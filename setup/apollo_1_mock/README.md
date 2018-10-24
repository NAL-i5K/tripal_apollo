
Setting up an Apollo 1 mock:


```
docker run --name apollo1_mock -e POSTGRES_PASSWORD=secret -p 5433:5432 -d postgres
docker cp ./setup.sql apollo1_mock:/
sleep 10 #wait for the container to launch
docker exec -it apollo1_mock /bin/sh -c 'PGPASSWORD=secret psql -U postgres </setup.sql'
```


this will create an empty Apollo container with just the tables the script needs to set users and permissions.

Add an apollo instance with localhost:5433 , dbname postgres, user postgres, and password secret


python ~/tripal/sites/all/modules/custom/tripal_apollo/bin/add_user.py -dbuser postgres -dbname postgres -dbpass secret -user apollo1@ccom.com -pwd Enlief2Bellwind -host http://localhost:5433


```
docker exec -it apollo1_mock psql -U postgres -W secret apollo1_mock

```
