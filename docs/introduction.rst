=========================
Introduction & Background
=========================


The Tripal Apollo module seeks to bridge Tripal and Apollo user and data management for both Apollo 1 and 2.


What is Apollo?
----------------------

Apollo is a plugin for the Genome Viewer JBrowse (http://jbrowse.org/)

Apollo provides:

* A user interface for editing gene annotation tracks
* GO term support
* Revision history

To learn more, visit:

http://genomearchitect.github.io/

 What does Tripal Apollo do?
-----------------------------

User account requests
~~~~~~~~~~~~~~~~~~~~~~~

Users visit ``/apollo-registration`` and select which organisms they would like access to.


.. image:: /_static/img/registration_page.png


An email is sent to the user and the site admin email notifying them of the request.

.. image:: /_static/img/admin_requests.png


Approving/denying requests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~


Registration requests appear at `admin/tripal/apollo/requests`.

Each row is for a single user - organism request pairing, so a single form submission may consist of several rows.  The admin can click the "Approve/Deny" button to view the request, which will list the user name, email, organism.  To approve or reject the request, check the appropriate box and click **Save**.
.

.. image:: /_static/img/approve_request.png

![approving a request](docs/_static/img/approve_request.png)
