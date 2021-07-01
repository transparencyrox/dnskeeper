#!/usr/bin/env python3

#  Title       : testdata
#  Creator     : Harsha Baste
#  Created     : 29.05.2021 10:30:47 PM
#  Description : Generate test data
"""

Generate test data 

"""

__author__ = "Harsh Baste"
__version__ = "0.1.0"
__license__ = "Proprietary"

import argparse
import logging
import logging.handlers
import os
import yaml
import sys
import time
from string import Template

import psycopg2
import boto3
from faker import Faker
import random

# ----[ Globals ]----

logger = logging.getLogger(os.path.splitext(os.path.basename(sys.argv[0]))[0])
config = None
conn = None
client = None
r53 = None

# ----[ Supporting Functions ]----


def read_config(config_file):
    global config
    logger.debug("Reading configuration from {}".format(config_file))
    with open(config_file, 'r') as ymlfile:
        root = yaml.load(ymlfile, Loader=yaml.FullLoader)
    config = root["application"]


class CustomFormatter(argparse.RawDescriptionHelpFormatter,
                      argparse.ArgumentDefaultsHelpFormatter):
    pass


def parse_args(args=sys.argv[1:]):
    parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__,
                                     formatter_class=CustomFormatter)

    g = parser.add_mutually_exclusive_group()
    g.add_argument("--debug",
                   "-d",
                   action="store_true",
                   default=False,
                   help="enable debugging")
    g.add_argument("--silent",
                   "-s",
                   action="store_true",
                   default=False,
                   help="don't log")
    g = parser.add_argument_group("testdata settings")
    g.add_argument('-c',
                   '--config',
                   dest="configfile",
                   required=False,
                   default="heroku.yml",
                   help='File to read the project specification from')
    g.add_argument('-a',
                   '--aws',
                   action="store_true",
                   default=False,
                   help='Create Route53 entries')

    return parser.parse_args(args)


def setup_logging(options):
    """Configure logging."""
    root = logging.getLogger("")
    root.setLevel(logging.WARNING)
    logger.setLevel(options.debug and logging.DEBUG or logging.INFO)
    if not options.silent:
        if not sys.stderr.isatty():
            facility = logging.handlers.SysLogHandler.LOG_DAEMON
            sh = logging.handlers.SysLogHandler(address='/dev/log',
                                                facility=facility)
            sh.setFormatter(
                logging.Formatter("{0}[{1}]: %(message)s".format(
                    logger.name, os.getpid())))
            root.addHandler(sh)
        else:
            ch = logging.StreamHandler()
            ch.setFormatter(
                logging.Formatter(
                    "%(asctime)-17s %(levelname)-7s | %(module)s.%(funcName)s.%(lineno)d | %(message)s",
                    datefmt="%d%m%Y:%H:%M:%S"))
            root.addHandler(ch)


# ----[ Application Logic ]----


def get_zone(domain):
    logger.debug("Listing Hosted Zones")
    zones = r53.list_hosted_zones_by_name(DNSName=domain)
    if not zones or len(zones['HostedZones']) == 0:
        raise Exception(
            "[Route53] Could not find hosted zone associated with domain")
    zone_id = zones['HostedZones'][0]['Id']
    return zone_id


def add_record(zone_id, ips, domain, subdomain, ttl, comment="Automated"):
    logger.debug("Adding A records")
    response = client.change_resource_record_sets(
        ChangeBatch={
            'Changes': [
                {
                    'Action': 'CREATE',
                    'ResourceRecordSet': {
                        'Name': subdomain + '.' + domain,
                        'ResourceRecords': ips,
                        'TTL': ttl,
                        'Type': 'A',
                    },
                },
            ],
            'Comment':
            comment,
        },
        HostedZoneId=zone_id,
    )


def main(options):

    global conn
    global client
    global r53
    logger.info("Starting {} ...".format(config["name"]))
    lines = open('names.txt').read().splitlines()

    try:
        db = config["database"]
        conn = psycopg2.connect(host=db["host"],
                                database=db["database"],
                                user=db["user"],
                                password=db["password"])

        logger.info('Connecting to PostgreSQL...')
        cur = conn.cursor()
        cur.execute('SELECT version()')
        db_version = cur.fetchone()
        logger.debug('PostgreSQL database version: {}'.format(db_version))

        aws = config["aws"]
        domain = aws["hosted_zone"]

        if options.aws:
            logger.info('Connecting to Route53...')
            client = boto3.client(
                'route53',
                aws_access_key_id=aws["aws_access_key_id"],
                aws_secret_access_key=aws["aws_secret_access_key"])
            r53 = boto3.client('route53')
            zone_id = get_zone(domain)

        faker = Faker()
        Faker.seed(time.time())

        # Create cluster table
        # id | name        | subdomain
        # 1  | Los Angeles | la
        # 2  | New York    | nyc
        table = """
            DROP TABLE IF EXISTS cluster;

            CREATE TABLE IF NOT EXISTS cluster (
            id SERIAL PRIMARY KEY,
            name VARCHAR(20) NOT NULL,
            subdomain VARCHAR(5) NOT NULL)
        """
        cur.execute(table)
        # Create server table
        # id | friendly_name | cluster_id | ip_string
        # 1  | something-1   | 1          | 123.123.123.123
        table = """
            DROP TABLE IF EXISTS server;

            CREATE TABLE IF NOT EXISTS server (
            id SERIAL PRIMARY KEY,
            friendly_name VARCHAR(30) NOT NULL,
            cluster_id INTEGER NOT NULL,
            ip_string VARCHAR(16) NOT NULL)
        """
        cur.execute(table)
        logger.info("Creating random cluster data")
        ips = []
        for x in range(1, 10):
            id = x
            name = faker.city()
            subdomain = faker.country_code().lower()
            cls_sql = """INSERT INTO cluster(name, subdomain)
                    VALUES(%s, %s) RETURNING id;"""
            cur.execute(cls_sql, (name, subdomain))
            cluster_id = cur.fetchone()[0]
            logger.debug("Created cluster {}".format(cluster_id))
            for y in range(1, random.randrange(2, 5)):
                server_name = random.choice(lines).lower() + "-" + str(y)
                ip = faker.ipv4()
                srv_sql = """INSERT INTO server(friendly_name, cluster_id, ip_string)
                        VALUES(%s, %s, %s) RETURNING id;"""
                cur.execute(srv_sql, (server_name, cluster_id, ip))
                server_id = cur.fetchone()[0]
                logger.debug("  - Added server {}/{} [{}]".format(
                    server_id, server_name, ip))
                ips.append({"Value": ip})
            if options.aws:
                add_record(zone_id, ips, domain, subdomain, 60)
            ips.clear()

            # Adding these records for our unit tests
        logger.info("Creating predictable cluster data for unit tests")
        unit_test_clusters = [
            {
                "name":
                "Test Cluster 1",
                "subdomain":
                "test1",
                "servers": [
                    {
                        "name": "srv1",
                        "ip": "192.168.42.1"
                    },
                    {
                        "name": "srv2",
                        "ip": "192.168.42.2"
                    },
                    {
                        "name": "srv3",
                        "ip": "192.168.42.3"
                    },
                ]
            },
            {
                "name":
                "Test Cluster 2",
                "subdomain":
                "test2",
                "servers": [
                    {
                        "name": "tsrv1",
                        "ip": "192.16.42.1"
                    },
                    {
                        "name": "tsrv2",
                        "ip": "192.16.42.2"
                    },
                ]
            },
        ]

        test_ips = []
        for cluster in unit_test_clusters:
            name = cluster["name"]
            subd = cluster["subdomain"]
            logger.debug("Adding cluster {} : {}".format(name, subd))
            cur.execute(cls_sql, (name, subd))
            cluster_id = cur.fetchone()[0]
            for srv in cluster["servers"]:
                logger.debug("  - Adding server {} : {}".format(
                    srv["name"], srv["ip"]))
                cur.execute(srv_sql, (srv["name"], cluster_id, srv["ip"]))
                test_ips.append({"Value": srv["ip"]})
            if options.aws:
                add_record(zone_id, test_ips, domain, cluster["subdomain"], 60)
            test_ips.clear()

        conn.commit()
        cur.close()

    except (Exception, psycopg2.DatabaseError) as error:
        logger.fatal(error)
    finally:
        if conn is not None:
            conn.close()
            logger.debug('Database connection closed')


# ----[ Entry Point ]----
if __name__ == "__main__":
    options = parse_args()

    # Config always over-rides the command line
    if options.configfile:
        if os.path.exists(options.configfile):
            read_config(options.configfile)

    if "loglevel" in config:
        lvl = config["loglevel"]
        logger.debug("Configuration over-ride for log level: {}".format(lvl))
        logger.setLevel(lvl)
        options.debug = logging.getLevelName(logger.level) == "DEBUG"
        if options.debug:
            options.silent = False
    setup_logging(options)

    try:
        main(options)
    except Exception as e:
        logger.exception("%s", e)
        sys.exit(1)
    sys.exit(0)
