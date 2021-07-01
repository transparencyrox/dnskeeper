#include <dnskeeper.h>
#include <SrvCache.hpp>

#include <iostream>
#include <cassert>
#include <sstream>
#include <string>
#include <iomanip>

bool SrvCache::test_connection()
{
    return m_conn.is_open();
}

SrvCache::SrvCache(const std::string& url)
    : m_conn(url)
{
}

bool SrvCache::get_servers(records_t &data, const std::string domain, const std::string ip)
{
    const std::string prepared_simple = R"(
        SELECT A.id as server_id,
            A.ip_string as server_ip,
            A.friendly_name as friendly_name,
            A.cluster_id as cluster_id,
            B.name as cluster_name,
            B.subdomain as cluster_subdomain
        FROM
            server A
            JOIN cluster B ON A.cluster_id = B.id
        )";

    const std::string prepared_with_subdomain = prepared_simple + R"(
        WHERE
            B.subdomain = $1)";

    const std::string prepared_with_subdomain_and_ip = prepared_simple + R"(
        WHERE
            B.subdomain = $1 AND A.ip_string = $2)";

    // Note: prepared_with_ip is not useful (same ip could be in multiple subdomains)

    if (m_conn.is_open())
    {
        pqxx::work tx{m_conn};
        std::string stmt = {};
        pqxx::params p;
        if(!domain.empty()) {
            auto it = domain.find('.');
            if (it != std::string::npos)
                p.append(domain.substr(0, it));
            else
                p.append(domain);

            if(!ip.empty()) {
                stmt = prepared_with_subdomain_and_ip;
                p.append(ip);
            } else {
                stmt = prepared_with_subdomain;
            }
        } else {
            stmt = prepared_simple;
        }

        LOG(TRACE) << "Prepared statement with "
                   << " domain:" << domain << " ip:" << ip
                   << " [" << stmt << "]\n";

        auto r = tx.exec_params(stmt, p);

        for (auto const &row : r)
        {
            assert(row.size() == SrvCache::MAX_COLS);
            data.push_back({row[SrvCache::SERVER_ID].c_str(),
                            row[SrvCache::IP_ADDR].c_str(),
                            row[SrvCache::NAME].c_str(),
                            row[SrvCache::CLUSTER_ID].c_str(),
                            row[SrvCache::CLUSTER_NAME].c_str(),
                            row[SrvCache::SUBDOMAIN].c_str()});
        }

        return data.size() > 0;
    }

    return false;
}

bool SrvCache::get_clusters(records_t &data)
{
    if (m_conn.is_open())
    {
        pqxx::work tx{m_conn};
        auto r = tx.exec("SELECT * from cluster");

        for (auto const &row : r)
        {
            assert(row.size() == 3); // Detect schema changes
            data.push_back({row[0].c_str(),
                            row[1].c_str(),
                            row[2].c_str()});
        }

        return data.size() > 0;
    }

    return false;
}

bool SrvCache::get_subdomains(const row_t &subdomains, records_t &data)
{
    if (subdomains.empty())
        return false;

    auto prepared_sql = [](int count) -> auto {
        std::string sql = R"(
            SELECT A.id as server_id,
                A.ip_string as server_ip,
                A.friendly_name as friendly_name,
                A.cluster_id as cluster_id,
                B.name as cluster_name,
                B.subdomain as cluster_subdomain
            FROM
                server A
                JOIN cluster B ON A.cluster_id = B.id 
            WHERE
                B.subdomain IN ()";
        // Add positional parameters
        for(auto i = 1; i <= count; i++) {
            if(i != 1)
                sql.append(",");

            sql.append("$").append(std::to_string(i));
        }
        sql.append(")");
        return sql;
    };

    if (m_conn.is_open())
    {
        pqxx::work tx{m_conn};

        auto stmt = prepared_sql(subdomains.size());
        LOG(TRACE) << "Prepared statement with "
                   << subdomains.size() 
                   << " parameters [" << stmt << "]\n";

        pqxx::params p;
        p.append_multi(subdomains);
        auto r = tx.exec_params(stmt, p);

        for (auto const &row : r)
        {
            assert(row.size() == SrvCache::MAX_COLS);
            data.push_back({row[SrvCache::SERVER_ID].c_str(),
                            row[SrvCache::IP_ADDR].c_str(),
                            row[SrvCache::NAME].c_str(),
                            row[SrvCache::CLUSTER_ID].c_str(),
                            row[SrvCache::CLUSTER_NAME].c_str(),
                            row[SrvCache::SUBDOMAIN].c_str()});
        }
        tx.commit();

        return data.size() > 0;
    }

    return false;
}