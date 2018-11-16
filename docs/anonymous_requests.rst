.. _permissions_guide:

==================
Permissions Guide
==================



Anonymous vs registered requests
=================================

The **access apollo** permission determines if visitors can submit Apollo requests.  You can choose wether or not to give this permission to anonymous users.  If you do, users won't need an account to request Apollo access.  This can be convenient if creating an account on your Drupal site offers little else to end users; in such cases its silly to create two accounts (the Apollo account and the Drupal account).


Our general recommendation is to **require an account** for Apollo registration.  Doing this will greatly simplify your anti-spam efforts.  You can simply enable Drupal anti-spam modules, which will automatically provide barriers at the account registration step.  Allowing anonymous Apollo registration form submissions means you will have to extend these anti-spam modules for each form you need to protect.

Protecting your site with anonymous submissions
=================================================

If you do allow anonymous requests, how do you enable anti-spam measures on the form?


HoneyPot Module
----------------

Honeypot deters spam bots from completing forms on your site!

Find out more here: https://www.drupal.org/project/honeypot

To add Honeypot to the registration form, enable the Honeypot module, and simply add ``honeypot_add_form_protection($form, $form_state, array('honeypot', 'time_restriction'));`` to the form.  You can use ``hook_form_alter`` to do this from within your site's custom themeing module, without having to maintain a separate fork of Tripal Apollo.

.. code-block:: php

  function form_alter(&$form, &$form_state, $form_id){
      if ($form_id == "tripal_apollo_registration_form") {
          honeypot_add_form_protection($form, $form_state, array('honeypot', 'time_restriction'));
      }
    }


Captcha Module
---------------

There are multiple captcha modules available for Drupal.  The below instructions are for the Captcha module.


To add a captcha to our form:

* Enable the drupal captcha module: https://www.drupal.org/project/captcha

* go to ``admin/config/people/captcha``
* Add the registration form with this ID: ``tripal_apollo_registration_form``.  Use the default challenge type.
* add the **Skip CAPTCHA** permission to your registered users, so they don't have to fill out the captcha if they are registered.

.. image:: /_static/img/captcha_form.png

A CAPTCHA now appears on our registration page!

.. image:: /_static/img/captchad_registration.png

By default, the Captcha is also added to the user registration form, so even if you don't allow anonymous users to request Apollo access, you may be interested in this module.
