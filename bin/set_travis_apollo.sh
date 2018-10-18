#!/usr/bin/env bash

#use apollo API to create organism and expected groups
curl -i -H 'Content-type: application/json' \
  -X POST http://127.0.0.1:8888/organism/addOrganism -d  \
  "{'username' : 'admin@local.host', 'password' : 'password', 'directory' : '/data/yeast',\
  'commonName' : 'yeast', 'genus' : 'saccharomyces',\
  'species' : 'cerevisiae'}"

curl -i -H 'Content-type: application/json' \
  -X POST http://127.0.0.1:8888/group/createGroup -d  \
  "{'username' : 'admin@local.host', 'password' : 'password', 'name' : 'saccharomyces_cerevisiae_READ'}"
curl -i -H 'Content-type: application/json' \
  -X POST http://127.0.0.1:8888/group/createGroup -d  \
  "{'username' : 'admin@local.host', 'password' : 'password', 'name' : 'saccharomyces_cerevisiae_WRITE'}"
