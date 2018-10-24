#!/usr/bin/env bash

echo $APOLLO_URL


#use apollo API to create organism and expected groups
curl -i -H 'Content-type: application/json' \
  -X POST ${APOLLO_URL}/organism/addOrganism -d  \
  "{'username' : 'admin@local.host', 'password' : 'password', 'directory' : '/data/yeast',\
  'commonName' : 'yeast', 'genus' : 'saccharomyces',\
  'species' : 'cerevisiae'}"

curl -i -H 'Content-type: application/json' \
  -X POST ${APOLLO_URL}/group/createGroup -d  \
  "{'username' : 'admin@local.host', 'password' : 'password', 'name' : 'saccharomyces_cerevisiae_USER'}"
curl -i -H 'Content-type: application/json' \
  -X POST ${APOLLO_URL}/group/createGroup -d  \
  "{'username' : 'admin@local.host', 'password' : 'password', 'name' : 'saccharomyces_cerevisiae_WRITE'}"
