<?php

/**
 * @file sitewide settings for this module.
 * Lets admins custom supply the python path.
 */

function tripal_apollo_admin_form($form, &$form_state)
{

    $path = variable_get('tripal_apollo_python_path');

    $form['python'] = [
        '#type' => 'textfield',
        '#default_value' => $path,
        '#title' => 'Python Path',
        '#description' => "Path to Python on the server."
    ];

    $table = variable_get('tripal_apollo_base_table');

    $tables = ['organism' => "organism",
        "analysis" => "analysis",
        "project" => "project"];

    $form['base_table'] = [
        '#type' => 'select',
        '#default_value' => $table,
        '#title' => 'Chado Base Table',
        '#description' => "The Chado base table associated with Apollo records.  This can be Organism, Analysis, and Project.",
        '#options' => $tables
    ];

    $encrypt = variable_get('tripal_apollo_encrypt');

    $form['encrypt'] = [
        '#type' => 'checkbox',
        '#default_value' => $encrypt,
        '#title' => "Encrypt Passwords",
        '#description' => "Enable/disable encryption.  Please see <a href = ' https://www.drupal.org/project/encrypt f'>https://www.drupal.org/project/encrypt</a> for more information."
    ];


    $form['submit'] = [
        '#type' => "submit",
        '#value' => "submit"
    ];
    return $form;
}


function tripal_apollo_admin_form_submit($form, &$form_state)
{
    $path = $form_state['values']['python'];
    variable_set('tripal_apollo_python_path', $path);
    $base = $form_state['values']['base_table'];
    variable_set('tripal_apollo_base_table', $base);
    $encrypt = $form_state['values']['encrypt'];
    variable_set('tripal_apollo_encrypt', $encrypt);

    tripal_set_message('Settings updated', TRIPAL_INFO);
}