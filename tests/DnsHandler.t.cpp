#include <catch2/catch.hpp>
#include <DnsHandler.hpp>

TEST_CASE("Confirm Hosted Zone setup", "[HostedZone]")
{
    REQUIRE(configured_properly());
    std::string domain = "";
    CHECK(secure_config("DOMAIN_NAME", domain));
    DnsHandler dns(domain);
    REQUIRE(dns.get_hosted_zone() != "");
}

TEST_CASE("Add to new subdomain", "[Records]")
{
    REQUIRE(configured_properly());
    std::string domain = "";
    CHECK(secure_config("DOMAIN_NAME", domain));
    DnsHandler dns(domain);
    std::string name = "unittest.pyrotechnics.io";
    INFO("Adding subdomain " << name);
    REQUIRE(dns.add_record(name, "10.9.8.5") == true);
}

TEST_CASE("Add to existing subdomain", "[Records]")
{
    REQUIRE(configured_properly());
    std::string domain = "";
    CHECK(secure_config("DOMAIN_NAME", domain));
    DnsHandler dns(domain);
    std::string name = "unittest.pyrotechnics.io";
    REQUIRE(dns.add_record(name, "10.9.8.6") == true);
}

TEST_CASE("Return all A records", "[Records]")
{
    REQUIRE(configured_properly());
    std::string domain = "";
    CHECK(secure_config("DOMAIN_NAME", domain));
    DnsHandler dns(domain);
    DnsHandler::records_t data;
    REQUIRE(dns.list_records(data) == true);
}

TEST_CASE("Return a single A record", "[Records]")
{
    REQUIRE(configured_properly());
    std::string domain = "";
    CHECK(secure_config("DOMAIN_NAME", domain));
    DnsHandler dns(domain);
    DnsHandler::records_t data;
    DnsHandler::rrset_t rrs;
    REQUIRE(dns.get_record("unittest.pyrotechnics.io", rrs) == true);
    REQUIRE(rrs.GetName() == "unittest.pyrotechnics.io.");
}

TEST_CASE("Delete record on subdomain", "[Records]")
{
    REQUIRE(configured_properly());
    std::string domain = "";
    CHECK(secure_config("DOMAIN_NAME", domain));
    DnsHandler dns(domain);
    std::string name = "unittest.pyrotechnics.io";
    std::string ip = "10.9.8.5";
    REQUIRE(dns.delete_record(name, ip) == true);
    ip = "10.9.8.6";
    REQUIRE(dns.delete_record(name, ip) == true);
}
