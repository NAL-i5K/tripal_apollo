=======================
Installation and Setup
=======================

Admin setup
============

Download the module using git (``git clone https://github.com/NAL-i5K/tripal_apollo.git``).  Enable the module with drush: ``drush pm-enable tripal_apollo``.  Instructions are the same for both Tripal 2 and Tripal 3 sites.

User passwords are generated using the ``/usr/share/dict/words`` file.  If this file doesn't exist on your server, please create it and populate with words you would like your user passwords generated with (one word per line).

Site-wide settings
-----------------------

Site-wide settings can be set at ``/admin/tripal_apollo``.

.. image:: /_static/img/admin_area.png


.. csv-table:: Site-Wide Settings
  :header: "Field", "Description"

  "Python Path", "Full path to python executable on the server.  Only necessary for Apollo 1."
  "Chado Base Table", "Determine what content will link to Apollo.  Set to organism by default.  Please see warning below regarding changing the base table."
  "Encrypt Passwords", "Should user and admin passwords be encrypted when stored in the database?  If your site is hacked, this will make user passwords more secure."


.. note::

	 Python path is only necessary for Apollo 1.


.. warning::

  **Important notice!!!**  Switching base tables will **wipe all information linking instances to records and records to users**.  Be very mindful of changing this setting!

.. note::
  We encourage enabling password encryption.  However, disabling encryption is provided in case of issues setting up the encryption module.  https://www.drupal.org/project/encrypt


Adding an Apollo Instance to Tripal
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

First, you must tell the module about your Apollo server.  To do so, go to **Content --> add Content --> Apollo Instance**.

If your server is Apollo 1, you will need to provide the db name, db admin name and password.  Apollo 2 will instead require the admin username and password: the db username is not required.

The URL should be the full apollo server URL without a trailing slash, for example,  ``http://localhost:8888``.  The correct URL is listed in your web services API on your apollo server:

.. image:: /_static/img/apollo_url.png

Select all of the organisms you would like linked to this Apollo instance.  Note that Apollo 1 mappings for multiple organisms makes several assumptions: see  :ref:`ApolloConfig`.



.. image:: /_static/img/create_apollo_instance.png

If your instance is successfully linked, the "Users" field will display the number of non-admin users on your instance.

Permissions
=============

This module defines the following permissions:

* administer tripal apollo: Administer the module itself.  This permission is for site admins.
* administer apollo users.  Allows admins to approve/deny apollo access requests.  This permission is for site admins and/or community leaders.
 * access apollo: allows users to make apollo registration requests.  You can give this permission to anonymous users, allowing users to register for apollo accounts without a Drupal account.

To learn more about setting up permissions and roles, please see https://www.drupal.org/docs/_static/img/7/managing-users/user-roles

Chado specific permissions
---------------------------


If you have the ``tripal_hq`` and ``tripal_hq_permissions`` modules enabled, you can use Chado-specific permissions!  This means you can have a user role that can, for example, approve Apollo requests for a subset of organisms **only**.  Simply configure HQ permissions for curators based on chado **organisms**.

Please see the ``tripal_hq`` module for more information: https://github.com/statonlab/tripal_hq.


References
---------------

Dunn NA, Munoz-Torres MC, Unni D, Yao E, Rasche E, Bretaudeau A, Holmes IH, Elsik CG; Lewis SE (2017). GMOD/Apollo: Apollo2.0.6(JB#29795a1bbb)
