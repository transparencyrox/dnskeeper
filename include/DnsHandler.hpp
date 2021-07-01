#pragma once
#include <dnskeeper.h>


#include <aws/core/Aws.h>
#include <aws/route53/Route53Client.h>
#include <aws/route53/model/ChangeResourceRecordSetsRequest.h>
#include <aws/core/utils/logging/LogLevel.h>

using namespace Aws::Route53;

class DnsHandler
{
public: // Types
    enum rowspec
    {
        DOMAIN = 0,
        IP_LIST,
        TTL,
        TYPE,
        MAX_ROW
    };
    using iplist_t = std::vector<std::string>;
    using row_t = std::tuple<std::string, iplist_t, long, char>;
    using rrset_t = Model::ResourceRecordSet;
    using records_t = std::vector<row_t>;

private:
    DnsHandler(const DnsHandler &) = delete;
    DnsHandler operator=(const DnsHandler &) = delete;
    DnsHandler() = delete;

    Aws::SDKOptions m_options = {};
    Aws::Http::URI m_domain;
    std::string m_zone_id = {};
    std::shared_ptr<Route53Client> m_client;
    const Model::RRType m_dnstype = Model::RRType::A;
    bool update(const rrset_t &rrs, bool = false);

public:
    DnsHandler(const std::string& domain);

    std::string get_hosted_zone();
    bool list_records(records_t &dnsdata);
    bool get_record(const std::string &name, rrset_t &);
    bool add_record(const std::string &name, const std::string &ip);
    bool delete_record(const std::string &name, const std::string &ip);
};