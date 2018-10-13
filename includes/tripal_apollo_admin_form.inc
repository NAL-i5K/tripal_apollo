<?php

/**
 * @file sitewide settings for this module.
 * Lets admins custom supply the python path.
 */

function tripal_apollo_admin_form($form, &$form_state){

variable_set('tripal_apollo_python_path',   "/usr/local/bin/python2.7");

  $path = variable_get('tripal_apollo_python_path');

  $form['python'] = [
    '#type' => 'text',
    '#value' => $path,
    '#title' => 'Python Path',
    '#description' => "Path to Python on the server."
  ];

  return $form;
}


function tripal_apollo_admin_form_submit($form, &$form_state){
  $path = $form_state['values']['python'];
  variable_set('tripal_apollo_python_path',   $path);
}