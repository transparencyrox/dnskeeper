#include <dnskeeper.h>
#include <iostream>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include <SrvCache.hpp>
#include <DnsHandler.hpp>
#include <Display.hpp>

int main(int argc, char **argv)
{
    init_log();
    if (argc < 2)
    {
        // Listening port port
        LOG(ERROR) << "No port specified\n";
        return -1;
    }

    if (!configured_properly())
    {
        LOG(ERROR) << "Check configuration\n";
        exit(-1);
    }

    std::string con_str = "";
    if(!secure_config("DATABASE_URL", con_str, 200)) {
        LOG(FATAL) << "DATABASE_URL invalid or not set\n";
        exit(-1);
    }
    SrvCache sc(con_str);
    if (sc.test_connection())
        LOG(DEBUG) << "Database connection succeeded\n";

    std::string domain_name = "";
    if(!secure_config("DOMAIN_NAME", domain_name)) {
        LOG(FATAL) << "DOMAIN_NAME invalid or not set\n";
        exit(-1);
    }
    DnsHandler dns(domain_name);

    httplib::Server svr;
    std::mutex render_mtx;  // protects g_i

    auto ret = svr.set_mount_point("/", "./www");
    if (!ret) {
        LOG(ERROR) << "Mount point www not found in current working directory " 
                   << std::filesystem::current_path() << "\n";
        exit(-1);
    } 
    svr.Get("/dns", [&](const httplib::Request &, httplib::Response &res)
            {
                DnsUI ui;
                DnsHandler::records_t data;
                LOG(TRACE) << "Requested DNS Page\n";

                if (dns.list_records(data))
                {
                    // DNS records 
                    // name|IP list|TTL|type
                    for (const auto &row : data)
                    {
                        auto domain = std::get<DnsHandler::DOMAIN>(row);
                        auto iplist = std::get<DnsHandler::IP_LIST>(row);

                        for (auto ip : iplist)
                        {
                            SrvCache::records_t rec;
                            // Server records
                            // id|ip|server_name|cluster_id|cluster_name|subdomain

                            if (sc.get_servers(rec, domain, ip) && rec.size())
                            {
                                for (const auto &s : rec)
                                    ui.add_row({domain, ip, s[SrvCache::NAME], s[SrvCache::CLUSTER_NAME]});
                            }
                            else
                                ui.add_row({domain, ip, "not found", "N/A"}, true);
                        }
                    }
                }

                res.set_content(ui.render(), "text/html");
            });
    svr.Get("/servers", [&](const httplib::Request &, httplib::Response &res)
            {
                const std::lock_guard<std::mutex> lock(render_mtx); // Avoid parallel page requests
                ServerUI ui;
                SrvCache::records_t rec;
                DnsHandler::records_t data;
                LOG(TRACE) << "Requested Servers Page\n";

                // map of domains --> set of ips per domain
                using dns_map = std::map<std::string, std::set<std::string>>;
                dns_map domain_map;
                if (dns.list_records(data))
                {
                    for (const auto &row : data)
                    {
                        auto domain = std::get<DnsHandler::DOMAIN>(row);
                        auto &ipset = domain_map[domain]; // implicit create

                        auto iplist = std::get<DnsHandler::IP_LIST>(row);
                        for (auto ip : iplist)
                            ipset.insert(ip);
                    }
                }
                if (sc.get_servers(rec) && rec.size())
                {
                    for (const auto &s : rec)
                    {
                        // Server records
                        // id|ip|server_name|cluster_id|cluster_name|subdomain
                        auto subdomain = s[SrvCache::SUBDOMAIN];
                        auto name = s[SrvCache::NAME];
                        auto ip = s[SrvCache::IP_ADDR];
                        auto domain = subdomain + "." + domain_name;
                        auto it = domain_map.find(domain);
                        if (it == domain_map.end())
                        {
                            ui.addition(name, domain, ip);
                        }
                        else
                        {
                            auto &ipset = it->second;
                            if (ipset.find(ip) != ipset.end())
                                ui.removal(name, domain, ip);
                            else
                                ui.addition(name, domain, ip);
                        }
                    }
                }
                res.set_content(ui.render(), "text/html");
            });
    svr.Get("/add", [&](const httplib::Request &req, httplib::Response &res)
            {
                std::string domain = req.get_param_value("domain");
                std::string ip = req.get_param_value("ip");
                LOG(TRACE) << "API call (add)\n";

                if (dns.add_record(domain, ip))
                    res.set_content("ADD_OK", "text/plain");
                else
                    res.set_content("ADD_ERROR", "text/plain");
            });
    svr.Get("/remove", [&](const httplib::Request &req, httplib::Response &res)
            {
                std::string domain = req.get_param_value("domain");
                std::string ip = req.get_param_value("ip");
                LOG(TRACE) << "API call (delete)\n";

                if (dns.delete_record(domain, ip))
                    res.set_content("DEL_OK", "text/plain");
                else
                    res.set_content("DEL_ERROR", "text/plain");
                res.set_content("Operation in process", "text/plain");
            });

    int port = std::stoi(argv[1]);
    LOG(INFO) << "Listening on port " << port << std::endl;
    svr.listen("0.0.0.0", port);
    return 0;
}