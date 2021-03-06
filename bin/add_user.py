#! /usr/bin/python
# Copyright (C) 2014  Jun-Wei Lin <cs.castman [at] gmail [dot] com>
#
# Edited 2018 Bradford Condon (bradford.condon) at gmail
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

__version__ = '1.0'

import argparse, psycopg2, sys

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description='''
Insert a new WebApollo (1) user into PostgreSQL DB and add read/write permissions of all tracks for the user.
Example Usage:
\tadduser.py -dbuser apollo_db_admin_user -dbname apollo_db_name -dbpass mypass -user abc123 -pwd 123456 -host some.host.com
\t(When connect to remote host, make sure the host accepts remote connection by editing pg_hba.conf)
''')
parser.add_argument('-dbuser', help="Username used to connect database", required=True)
parser.add_argument('-dbname', help="Database name to be connected", required=True)
parser.add_argument('-dbpass', help="Username password for connecting to database", required=True)
parser.add_argument('-host', help="Host name or IP of database server (Default: localhost)", required=False, default='localhost')
parser.add_argument('-user', help="Username to be added", required=True)
parser.add_argument('-pwd', help="Password of the new user (Default: the same as -user)", required=False)
parser.add_argument('-perm', help="Permission of the new user (Default: 3)", required=False, default='3')
parser.add_argument('-v', '--version', action='version', version='%(prog)s ' + __version__)

args = parser.parse_args()

dbpass = args.dbpass
dbname = args.dbname
dbuser = args.dbuser
host = args.host
user = args.user
pwd = user
if args.pwd:
    pwd = args.pwd
perm = args.perm # read=1, write=2, publish=4, admin=8; ex. read+write = 1+2 = 3; system admin = 1+2+4+8 = 15

if '://' in host :
    host  = host.split('://', 1)
    host = host[1]


if ':' in host :
    split_host = host.split(':', 1)
    host =  split_host[0]
    port = split_host[1]

connection_string = 'dbname=' + dbname + ' user=' + dbuser + ' host=' + host + ' password=' + dbpass

if port:
    connection_string = connection_string + ' port=' + port

conn = psycopg2.connect(connection_string)
cur = conn.cursor()

# If the user already exists then exit
cur.execute("SELECT user_id FROM users WHERE username=%s;", (user, ))
rows = cur.fetchall()
if rows:
    print 'User \'%s\' already exists and can not be inserted.' % (user)
    sys.exit(0)

# add new user
cur.execute("INSERT INTO users (username, password) VALUES (%s, %s);", (user, pwd, ))

# if the user already exists, get the user id
cur.execute("SELECT user_id FROM users WHERE username=%s;", (user, ))
rows = cur.fetchall()
user_id = int(rows[0][0])

# select all tracks (with prefix 'Annotations-NW_') form tracks table
cur.execute("SELECT track_id  FROM tracks WHERE track_name LIKE 'Annotations-%'")
rows = cur.fetchall()
for row in rows:
    track_id = int(row[0])
    print track_id
    cur.execute("INSERT INTO permissions (track_id, user_id, permission) VALUES (%s, %s, %s);", (track_id, user_id, perm))

# commands to remove a user
#cur.execute("DELETE FROM permissions WHERE user_id=%s;", (str(user_id),))
#cur.execute("DELETE FROM users WHERE user_id=%s;", (str(user_id),))

conn.commit()
cur.close()
conn.close()
