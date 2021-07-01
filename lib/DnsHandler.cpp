#include <dnskeeper.h>

#include <DnsHandler.hpp>
#include <unistd.h>

#include <aws/route53/model/ListResourceRecordSetsRequest.h>
#include <aws/route53/model/ListHostedZonesByNameRequest.h>
#include <aws/route53/model/GetChangeRequest.h>

DnsHandler::DnsHandler(const std::string& domain)
    : m_domain(domain)
{
    Aws::InitAPI(m_options);
    Aws::Client::ClientConfiguration client_config("default");
    m_client = Aws::MakeShared<Aws::Route53::Route53Client>("RouteClient");
    get_hosted_zone();
}

std::string DnsHandler::get_hosted_zone()
{
    if (!m_zone_id.empty())
        return m_zone_id;
    auto hzr = Model::ListHostedZonesByNameRequest();
    hzr.AddQueryStringParameters(m_domain);
    auto outcome = m_client->ListHostedZonesByName(hzr);
    if (outcome.IsSuccess())
    {
        auto result = outcome.GetResult();
        auto zones = result.GetHostedZones();
        if (zones.size() > 0)
        {
            // Note: Right now we just use the first one for the test
            m_zone_id = zones[0].GetId();

            // AWS api replies with a zone_id
            // with a prefix that subsequent API
            // calls dont seem to want
            m_zone_id = m_zone_id.substr(m_zone_id.rfind("/") + 1);
            LOG(DEBUG) << "DNS Zone ID: " << m_zone_id << "\n";
        }
        else
        {
            LOG(FATAL) << "No zone found\n";
            exit(-1);
        }
    }
    else
    {
        LOG(ERROR) << "ListHostedZones: "
                   << outcome.GetError()
                   << std::endl;
    }
    return m_zone_id;
}

bool DnsHandler::list_records(records_t &dnsdata)
{
    auto lrrs = Model::ListResourceRecordSetsRequest()
                    .WithHostedZoneId(m_zone_id);
    auto outcome = m_client->ListResourceRecordSets(lrrs);
    if (outcome.IsSuccess())
    {
        auto result = outcome.GetResult().GetResourceRecordSets();
        for (auto &r : result)
        {
            if (r.GetType() == m_dnstype)
            {
                iplist_t ips;
                auto rr = r.GetResourceRecords();
                for (const auto &ip : rr)
                    ips.push_back(ip.GetValue());
                // AWS API oddity. There's a period
                // at the end of the returned domain
                std::string domain = r.GetName();
                domain.pop_back();
                auto row = std::make_tuple(domain,
                                           ips,
                                           r.GetTTL(),
                                           static_cast<char>(r.GetType()));
                dnsdata.push_back(row);
            }
        }
    }
    else
    {
        LOG(ERROR) << "ListResourceRecordSets failed: "
                   << static_cast<int>(outcome.GetError().GetErrorType())
                   << std::endl
                   << outcome.GetError()
                   << std::endl;
    }
    return dnsdata.size() > 0;
}

bool DnsHandler::get_record(const std::string &name,
                            rrset_t &rr)
{
    auto lrrs = Model::ListResourceRecordSetsRequest()
                    .WithStartRecordName(name)
                    .WithStartRecordType(m_dnstype)
                    .WithHostedZoneId(m_zone_id);
    auto outcome = m_client->ListResourceRecordSets(lrrs);
    if (outcome.IsSuccess())
    {
        auto result = outcome.GetResult().GetResourceRecordSets();
        for (auto &r : result)
            if (r.GetName() == (name + "."))
            {
                rr = r;
                return true;
            }
    }
    else
    {
        LOG(ERROR) << "ListResourceRecordSets failed: "
                   << static_cast<int>(outcome.GetError().GetErrorType())
                   << std::endl
                   << outcome.GetError()
                   << std::endl;
    }

    LOG(NOTICE) << "Did not locate an A type DNS record for [" << name << "]\n";
    return false;
}

bool DnsHandler::update(const rrset_t &rrs, bool remove)
{
    auto action = remove ? Model::ChangeAction::DELETE_ : Model::ChangeAction::UPSERT;
    LOG(DEBUG) << "DNS Update (" << (remove ? "DELETE" : "UPSERT") << ")\n";

    auto chg = Model::Change()
                   .WithAction(action)
                   .WithResourceRecordSet(rrs);
    auto crrs = Model::ChangeResourceRecordSetsRequest()
                    .WithHostedZoneId(m_zone_id)
                    .WithChangeBatch(
                        Model::ChangeBatch()
                            .WithComment("Automated")
                            .AddChanges(chg));

    auto outcome = m_client->ChangeResourceRecordSets(crrs);

    if (outcome.IsSuccess())
    {
        LOG(TRACE) << "DNS update outcome successful\n";
        unsigned sync_await = 120; // Wait for the sync
        do
        {
            sleep(1);
            auto id = outcome.GetResult().GetChangeInfo().GetId();
            id = id.substr(id.rfind("/") + 1);
            auto await_outcome = m_client->GetChange(
                Model::GetChangeRequest().WithId(id));
            if (await_outcome.IsSuccess())
            {
                auto ci = await_outcome.GetResult().GetChangeInfo();
                if (ci.GetStatus() == Model::ChangeStatus::INSYNC)
                    return true;
                LOG(TRACE) << "Awaiting record syncronization ...\n";
            }
            else
                LOG(ERROR) << "GetChange failed: "
                           << static_cast<int>(outcome.GetError().GetErrorType())
                           << std::endl
                           << outcome.GetError()
                           << std::endl;
            sync_await--;
        } while (sync_await);
    }
    else
    {
        LOG(ERROR) << "ChangeResourceRecordSets update failed: "
                   << static_cast<int>(outcome.GetError().GetErrorType())
                   << std::endl
                   << outcome.GetError()
                   << std::endl;
    }

    return false;
}

bool DnsHandler::add_record(const std::string &name, const std::string &ip)
{
    rrset_t rrs;
    Aws::Vector<Model::ResourceRecord> rrv;
    if (get_record(name, rrs))
    {
        // Check if the value is already present
        for (auto &rr : rrs.GetResourceRecords())
        {
            if (rr.GetValue() == ip)
            {
                LOG(NOTICE) << "Detected existing entry for ["
                            << name << " / " << ip << " ]"
                            << "\n";
                return true;
            }
        }
        rrs.AddResourceRecords(Model::ResourceRecord().WithValue(ip));
    }
    else
    {
        LOG(DEBUG) << "Adding a new record for ["
                   << name << " / " << ip << " ]"
                   << "\n";

        // Resource record set doesnt exist. Add one
        rrs = Model::ResourceRecordSet()
                  .WithName(name)
                  .WithType(m_dnstype)
                  .WithTTL(60);
        rrs.AddResourceRecords(Model::ResourceRecord().WithValue(ip));
    }

    return update(rrs);
}

bool DnsHandler::delete_record(const std::string &name, const std::string &ip)
{
    rrset_t rrs;
    bool found = false;
    Aws::Vector<Model::ResourceRecord> rrv;
    if (get_record(name, rrs))
    {
        // Check if the value is present
        for (auto &rec : rrs.GetResourceRecords())
        {
            if (rec.GetValue() == ip)
            {
                found = true;
            }
            else
                // Push everything else into
                // the update vector except the
                // found IP
                rrv.push_back(rec);
        }
    }

    if (found)
    {
        if (rrv.empty())
        {
            LOG(TRACE) << "No other records on ResourceRecordSet\n";
            rrv.push_back(Model::ResourceRecord().WithValue(ip));
            return update(rrs, true);
        }
        else
        {
            LOG(TRACE) << "Other records exist on ResourceRecordSet\n";
            rrs.SetResourceRecords(rrv);
            return update(rrs);
        }
    }
    LOG(NOTICE) << "Did not find IP for [" << name << " / " << ip << " ]"
                << "\n";
    return false;
}