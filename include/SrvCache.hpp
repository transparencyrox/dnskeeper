#pragma once
#include <dnskeeper.h>

#include <pqxx/pqxx>
class SrvCache
{
private:
    pqxx::connection m_conn;
    SrvCache(const SrvCache &) = delete;
    SrvCache operator=(const SrvCache &) = delete;
    SrvCache() = delete;

public:
    enum rowspec
    {
        SERVER_ID = 0,
        IP_ADDR,
        NAME,
        CLUSTER_ID,
        CLUSTER_NAME,
        SUBDOMAIN,
        MAX_COLS
    };
    using server_t = std::tuple<std::string /*subdomain*/,
                                std::string /*IP*/>;
    using serverlist_t = std::vector<server_t>;
    using row_t = std::vector<std::string>;
    using records_t = std::vector<row_t>;

    SrvCache(const std::string&);
    bool test_connection();
    bool get_clusters(records_t &data);
    bool get_servers(records_t &data, const std::string domain = "", const std::string ip = "");
    bool get_subdomains(const row_t &subdomains, records_t &data);
};