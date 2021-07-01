#include <catch2/catch.hpp>

#include <SrvCache.hpp>

TEST_CASE("Check if we can connect to the database", "[Database]")
{
    // Test connection to the Heroku DB
    std::string con_str = "";
    CHECK(secure_config("TEST_DATABASE", con_str, 200));
    SrvCache sc(con_str);
    REQUIRE(sc.test_connection() == true);
}

TEST_CASE("Return list of servers", "[Database]")
{
    std::string con_str = "";
    CHECK(secure_config("TEST_DATABASE", con_str, 200));
    SrvCache sc(con_str);
    SrvCache::records_t servers;
    REQUIRE(sc.get_servers(servers) == true);
    REQUIRE(servers.size() > 0);
}

TEST_CASE("Return list of clusters", "[Database]")
{
    std::string con_str = "";
    CHECK(secure_config("TEST_DATABASE", con_str, 200));
    SrvCache sc(con_str);
    SrvCache::records_t clusters;
    REQUIRE(sc.get_clusters(clusters) == true);
    REQUIRE(clusters.size() > 0);
}

TEST_CASE("Return all servers for a subset of subdomains", "[Database]")
{
    std::string con_str = "";
    CHECK(secure_config("TEST_DATABASE", con_str, 200));
    SrvCache sc(con_str);
    SrvCache::row_t subdomains;
    SrvCache::records_t servers;
    SECTION("Confirm empty subdomains returns all records")
    {
        REQUIRE(sc.get_subdomains(subdomains, servers) == false);
    }
    SECTION("Confirm column content")
    {
        subdomains.push_back("test1");
        REQUIRE(sc.get_subdomains(subdomains, servers) == true);
        REQUIRE(servers.size() == 3);
        REQUIRE(servers[0][5] == "test1");
    }
    SECTION("Confirm multiple subdomains")
    {
        subdomains.push_back("test1");
        subdomains.push_back("test2");
        REQUIRE(sc.get_subdomains(subdomains, servers) == true);
        REQUIRE(servers.size() == 5);
    }
}

TEST_CASE("Return server for domain and ip", "[Single entry]")
{
    std::string con_str = "";
    CHECK(secure_config("TEST_DATABASE", con_str, 200));
    SrvCache sc(con_str);
    SrvCache::records_t servers;
    REQUIRE(sc.get_servers(servers, "test2.pyrotechnics.io", "192.16.42.2") == true);
    REQUIRE(servers.size() == 1);
    REQUIRE(servers[0][SrvCache::NAME] == "tsrv2");
    REQUIRE(servers[0][SrvCache::IP_ADDR] == "192.16.42.2");
}
