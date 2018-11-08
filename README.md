[![Build Status](https://travis-ci.org/NAL-i5K/tripal_apollo.svg?branch=master)](https://travis-ci.org/NAL-i5K/tripal_apollo)

The Tripal Apollo module seeks to bridge Tripal and Apollo user and data management for both Apollo 1 and 2.


# What is Apollo?

Apollo is a plugin for the Genome Viewer [JBrowse](http://jbrowse.org/)

Apollo provides:

* A user interface for editing gene annotation tracks
* GO term support
* Revision history

To learn more, visit:

http://genomearchitect.github.io/

# What does Tripal Apollo do?

### User account requests

Users visit `/apollo-registration` and select which organisms they would like access to.

![user regisrtation form](docs/_static/img/registration_page.png)

An email is sent to the user and the site admin email notifying them of the request.

![admin requests](docs/_static/img/admin_requests.png)

### Approving/denying requests

Registration requests appear at `admin/tripal/apollo/requests`.

Each row is for a single user - organism request pairing, so a single form submission may consist of several rows.  The admin can click the "Approve/Deny" button to view the request, which will list the user name, email, organism.  To approve or reject the request, check the appropriate box and click **Save**.
.
![approving a request](docs/_static/img/approve_request.png)


# Admin setup

Download the module using git (`git clone https://github.com/NAL-i5K/tripal_apollo.git`).  Enable the module with drush: `drush pm-enable tripal_apollo`.  Instructions are the same for both Tripal 2 and Tripal 3 sites.

User passwords are generated using the `/usr/share/dict/words` file.  If this file doesn't exist on your server, please create it and populate with words you would like your user passwords generated with (one word per line).

## Site-wide settings

Site-wide settings can be set at `/admin/tripal_apollo`.  

Python path is only necessary for Apollo 1.

**Important notice!!!**  Switching base tables will **wipe all information linking instances to records and records to users**.  Be very mindful of changing this setting!

![admin area](docs/_static/img/admin_area.png)

Note that we encourage enabling encryption of passwords.  However, disabling encryption is provided in case of issues setting up the encryption module.

## Creating an Apollo Instance

First, you must tell the module about your Apollo server.  To do so, go to **Content --> add Content --> Apollo Instance**.

If your server is Apollo 1, you will need to provide the db name, db admin name and password.  Apollo 2 will instead require the admin username and password: the db username is not required.

The URL should be the full apollo server URL without a trailing slash, for example,  `http://localhost:8888`.  The correct URL is listed in your web services API on your apollo server:

![apollo url](docs/_static/img/apollo_url.png)

Select all of the organisms you would like linked to this Apollo instance.  Note that Apollo 1 mappings for multiple organisms makes several assumptions: see below.

![create apolllo instance](docs/_static/img/create_apollo_instance.png)


If your instance is successfully linked, the "Users" field will display the number of non-admin users on your instance.  

## Permissions

This module defines the following permissions:

* administer tripal apollo: Administer the module itself.  This permission is for site admins.
* administer apollo users.  Allows admins to approve/deny apollo access requests.  This permission is for site admins and/or community leaders.
 * access apollo: allows users to make apollo registration requests.  You can give this permission to anonymous users, allowing users to register for apollo accounts without a Drupal account.

To learn more about setting up permissions and roles, please see https://www.drupal.org/docs/_static/img/7/managing-users/user-roles

## Chado specific permissions

If you have the `tripal_hq` and `tripal_hq_permissions` modules enabled, you can use Chado-specific permissions!  This means you can have a user role that can, for example, approve Apollo requests for a subset of organisms **only**.  Simply configure HQ permissions for curators based on chado **organisms**.

Please see the [`tripal_hq` module for more information](https://github.com/statonlab/tripal_hq).

## Apollo 1 setup

Apollo 1 does not support a REST API.  Your Apollo 1 server's database must therefore be setup to accept remote connections by editing `pg_hba.conf`.


## Testing and development

The travis CI environment uses the Docker compose file in this repository to launch a Tripal site and Apollo site. An example setup:

```bash
tar -xvf example_data/yeast.tar.gz -C example_data/
composer install
docker-compose up -d
## Set the APOLLO_URL variable.
APOLLO_URL=http://localhost:8888
export APOLLO_URL
/bin/bash setup/set_travis_apollo.sh
```

If you only need an Apollo container, it can be run via `docker run`:

```bash

tar -xvf example_data/yeast.tar.gz -C example_data/
docker run -it -v ${PWD}/example_data/:/data  -p 8888:8080 quay.io/gmod/docker-apollo:2.1.0

## Set the APOLLO_URL variable.
APOLLO_URL=http://localhost:8888
export APOLLO_URL
/bin/bash setup/set_travis_apollo.sh

```

Note the Apollo credentials for this container are:

username: admin@local.host
password: password

### Setting up Test Suite

Prior to running test suite, you must run `composer install` and copy `tests/example.env` to `tests/.env`.  Note we define an extra variable in `tests/example.env`: `APOLLO_URL=http://localhost:8888`.  This **MUST** include `http` and it must point at your Apollo instance for tests to work.

See https://tripaltestsuite.readthedocs.io/en/latest/environment.html?highlight=.env for general information on setting up Test Suite.

## References

Dunn NA, Munoz-Torres MC, Unni D, Yao E, Rasche E, Bretaudeau A, Holmes IH, Elsik CG; Lewis SE (2017). GMOD/Apollo: Apollo2.0.6(JB#29795a1bbb)
