#!/usr/bin/python
from __future__ import absolute_import
from subprocess import Popen, PIPE, call
import json
import sys
import argparse

parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description='''
Insert a new WebApollo2 user into PostgreSQL DB and add read/write permissions of all tracks for the user.
Example Usage:
\tcreateUser.py -dbuser myadmin -host http://192.168.100.9:8085 -user abc123 -fname f -lname l -pwd 123456 -group CAT_ADMIN
''')
parser.add_argument('-dbuser', help="Username used to connect database", required=True)
parser.add_argument('-dbpass', help="Admin password used to connect database", required=True)
#parser.add_argument('-dbname', help="Database name to be connected", required=True)
#Needed for Apollo 1, not Apollo 2
parser.add_argument('-host', help="Host IP of Apollo2 server ", required=True)
parser.add_argument('-user', help="Username to be added", required=True)
parser.add_argument('-fname', help="Firstname to be added", required=True)
parser.add_argument('-lname', help="Lastname to be added", required=True)
parser.add_argument('-pwd', help="Password of the new user (Default: the same as -user)", required=False)
parser.add_argument('-group', help="Group name for which user to be added", required=True)
parser.add_argument('-metadata', help="", required=False, default='')
parser.add_argument('-token', help="", required=False, default='ignore')


#parser.add_argument('-perm', help="Permission of the new user (Default: 3)", required=False, default='3')
#parser.add_argument('-v', '--version', action='version', version='%(prog)s ' + __version__)

args = parser.parse_args()

dbuser = args.dbuser
dbpass = args.dbpass
host = args.host
user = args.user
fname = args.fname
lname = args.lname
pwd = user
#organism = args.organism
group = args.group
metadata = args.metadata
token = args.token
if args.pwd:
    pwd = args.pwd

# Usage:
# python createUserBatch.py add_users_groups.txt

ADMIN=dbuser
PASSWD=''

def create_insert_str(email, newPassword, firstName, lastName, metadata, token):
    curl_str = ["curl", "-i", '-s', "-X", "POST", "-H", "'Content-Type:","application/json'", "-d"]
    curl_cmd =  "'{\"username\":\"" + dbuser + "\",\"password\":\"" + dbpass
    curl_cmd += "\",\"email\":\"" + email + "\",\"newPassword\":\"" + newPassword
    curl_cmd += "\",\"firstName\":\"" + firstName + "\",\"lastName\":\"" + lastName + "\",\"metadata\":\"" + metadata + "\",\"client_token\":\"" + token +"\"}'"
    curl_str.append(curl_cmd)
    curl_str.append(host + "/user/createUser")
    return ' '.join(curl_str)

def add_user_permisson_str(group, user, token):
    curl_str = ["curl", "-i", '-s', "-X", "POST", "-H", "'Content-Type:","application/json'", "-d"]
    curl_cmd =  "'{\"username\":\"" + dbuser + "\",\"password\":\"" + dbpass
    curl_cmd += "\",\"group\":\"" + group
    curl_cmd += "\",\"user\":\"" + user + "\",\"client_token\":\"" + token + "\"}'"
    curl_str.append(curl_cmd)
    curl_str.append(host + "/user/addUserToGroup")
    return ' '.join(curl_str)

def createUser(email, newPassword, firstName, lastName, metadata, token):
    p = Popen(create_insert_str(email, newPassword, firstName, lastName, metadata, token), stdout=PIPE, stderr=None, shell=True)
    output, err = p.communicate()
    http_code = output.split('\n')[0]
    msg = output.split('\n')[-1]
    if msg == '{}':
       return "SUCCESS", http_code, 'user \'%s\' added to the grails_user table ' % (email)
    if msg == '':
       return "FAILURE", http_code, 'user \'%s\' createUser fails' % (email)
    d = json.loads(msg)
    return "SUCCESS", http_code, 'user \'%s\'' % (email) + ", " + d['error']

def addUserToGroup(group, user, token):
    p = Popen(add_user_permisson_str(group, user, token), stdout=PIPE, stderr=None, shell=True)
    output, err = p.communicate()
    http_code = output.split('\n')[0]
    msg = output.split('\n')[-1]
    if msg == '{}':
       return "SUCCESS", http_code, 'user \'%s\' n group \'%s\' inserted into user_group_users table.' %(user, group)
    if msg == '':
       return "FAILURE", http_code, 'user \'%s\' addUserToGroup fails' % (user)

    d = json.loads(msg)
    return "FAILURE", http_code, 'user \'%s\'' % (user) + ", " + d['error']

status, code, err = createUser(user, pwd, fname, lname, metadata, token)
print(code)
print(err)

if status == 'SUCCESS':
    status_g, code_g, err_g = addUserToGroup(group, user, token)
    print(code_g)
    print(err_g)


'''
input_file = open(sys.argv[1], 'r')
#skip header
input_file.readline()
for line in input_file:
    line = line.strip()
    data = line.split("\t")
    if len(data) == 1:
        break
    email = data[0]
    token = data[1]
    newpasswd  = data[2]
    fname = data[3]
    lname = data[4]
    metadata = data[5]
    groups = data[6].replace('"', '').split(',')

    print("Create user:", email, newpasswd, fname, lname, metadata, groups)
    status, code = createUser(email, newpasswd, fname, lname, metadata, token)
    if status == 'SUCCESS':
        print("Create user " + email + " succeed.")
        for group in groups:
            status_g, code_g = addUserToGroup(group, email, token)
            if status_g == 'SUCCESS':
                print("addUserToGroup: " + email + " to " + group + " succeed.")
            else:
                print("addUserToGroup: " + email + " to " + group + " failure : " + code + ".")
    else:
        print("Create user " + email + " failure : " + code + ".")
'''
