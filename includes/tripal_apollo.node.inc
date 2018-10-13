<?php


/**
 * @file
 * This file defines the Tripal Apollo content type.
 * Each instance is a single server that the Drupal site can access in order to
 *   create users, etc.
 */


/**
 * hook node_info
 */
function tripal_apollo_node_info() {

  return [
    'apollo_instance' => [
      'name' => t('Apollo Instance'),
      'base' => 'apollo_instance',
      'description' => t('Registers an external Apollo web server. For use with administrative and user management tools provided by Tripal Apollo.'),
    ],
  ];

}


/**
 * Implements hook node_access()
 */
function tripal_apollo_node_access($node, $op, $account) {
  $node_type = $node;
  if (is_object($node)) {
    $node_type = $node->type;
  }

  if ($node_type == 'apollo_instance') {
    if ($op == 'create') {

      if (!user_access('create Apollo Instance', $account)) {
        return NODE_ACCESS_DENY;
      }
      return NODE_ACCESS_ALLOW;
    }
    if ($op == 'update') {
      if (!user_access('edit Apollo Instance', $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    if ($op == 'delete') {
      if (!user_access('delete Apollo Instance', $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    if ($op == 'view') {
      if (!user_access('access Apollo Instance', $account)) {
        return NODE_ACCESS_DENY;
      }
    }
    return NODE_ACCESS_IGNORE;
  }
}

/**
 *
 * Node creation form for apollo server.
 */
function apollo_instance_form($node, &$form_state) {

  $form = [];
  $form['#validate'] = ['apollo_instance_form_validate'];
  $form['core'] = [
    '#type' => 'fieldset',
    '#title' => 'General',
  ];

  $form['core']['instance_name'] = [
    '#type' => 'textfield',
    '#title' => t("Human-readable Name"),
    '#required' => TRUE,
    '#default_value' => property_exists($node, 'instance_name') ? $node->instance_name : '',
  ];

  $form['core']['url'] = [
    '#type' => 'textfield',
    '#title' => t("URL of Apollo Server"),
    '#required' => TRUE,
    '#default_value' => property_exists($node, 'url') ? $node->url : '',

  ];


  $table_choices = [
    'organism' => 'Organism',
    'analysis' => 'Analysis',
    'project ' => 'Project',
  ];

  $form['core']['base_table'] = [
    '#type' => 'select',
    '#options' => $table_choices,
    '#title' => t("Chado Base Table"),
    '#description' => 'Each record can only be associated with one instance.  We suggest associating instances with organisms.  However, if you would like to support different assemblies for the same organism, you can use the chado project or analysis table instead.',
    '#default_value' => property_exists($node, 'base_table') ? $node->base_table : 'organism',
    '#ajax' => [
      'callback' => 'tripal_apollo_record_callback',
      'wrapper' => 'tripal_apollo_record_wrapper',
    ],
  ];

  $select = [1 => 'Apollo 1', 2 => 'Apollo 2'];

  $form['core']['apollo_version'] = [
    '#type' => 'select',
    '#title' => t('Apollo Instance Type'),
    '#required' => TRUE,
    '#empty_option' => 'Select an Apollo version',
    '#options' => $select,
    '#default_value' => property_exists($node, 'apollo_version') ? $node->apollo_version : NULL,
  ];


  $form['core']['admin_name'] = [
    '#type' => 'textfield',
    '#title' => t("Admin name"),
    '#required' => TRUE,
    '#default_value' => property_exists($node, 'admin_name') ? $node->admin_name : '',
  ];

  $form['core']['admin_password'] = [
    '#type' => 'textfield',
    '#title' => t("Admin password"),
    '#required' => TRUE,
    '#default_value' => NULL,
  ];


  $selected = isset($form_state['values']['base_table']) ? $form_state['values']['base_table'] : 'organism';


  $record_list = tripal_apollo_build_record_select($selected);


  $form['core']['records'] = [
    '#type' => 'select',
    '#title' => 'Associated Records',
    '#multiple' => TRUE,
    '#description' => 'Select which records are associated with this instance.  A record can only be associated with one instance: setting this instance will overwrite others.',
    '#prefix' => '<div id="tripal_apollo_record_wrapper">',
    '#suffix' => '</div>',
    '#options' => $record_list,
  ];

  return $form;
}


/**
 * Implements hook_form_validate().
 * Ensures the Apollo name and url are unique.
 */
function apollo_instance_form_validate($form, $form_state) {

  $values = $form_state['values'];
  $name = $values['instance_name'];
  $url = $values['url'];

  $editing = isset($values['nid']) ? TRUE : FALSE;

  if ($editing) {//TODO: we should check if we have changed the name/url but for now we will just return.
    return;
  }

  $result = db_select('public.apollo_instance', 't')
    ->fields('t')
    ->condition('name', $name)
    ->execute()
    ->fetchObject();

  if ($result) {
    form_set_error('core[instance_name]', t('An instance with that name already exists.  Please supply a unique name.'));
  }
  $result = db_select('public.apollo_instance', 't')
    ->fields('t')
    ->condition('name', $name)
    ->execute()
    ->fetchObject();

  if ($result) {
    form_set_error('core[instance_name]', t('An instance with that URL already exists.  Please supply a unique URL.'));
  }

}

/**
 * Implements hook_insert().
 *
 * @param $node
 */

function apollo_instance_insert($node) {

  $password = drupal_hash_base64($node->admin_password);

  $id = db_insert('apollo_instance')->fields([
    'nid' => $node->nid,
    'name' => $node->instance_name,
    'url' => $node->url,
    'apollo_version' => $node->apollo_version,
    'admin_name' => $node->admin_name,
    'admin_password' => $password,
    'base_table' => $node->base_table,
  ])->execute();

  //associate records with this instance.  Clear a pre-exisiting record then insert.

  foreach ($node->records as $record) {
    db_delete('apollo_instance_record')
      ->condition('base_table', $node->base_table)
      ->condition('record_id', $record)
      ->execute();

    $result = db_insert('apollo_instance_record')
      ->fields([
        'instance_id' => $id,
        'record_id' => $record,
        'base_table' => $node->base_table,
      ])
      ->execute();


  }

}

/**
 * Implements hook_update().
 *
 * @param $node
 */
function apollo_instance_update($node) {


  $fields = [
    'name' => $node->instance_name,
    'url' => $node->url,
    'apollo_version' => $node->apollo_version,
    'admin_name' => $node->admin_name,
    'base_table' => $node->base_table,
  ];


  if ($node->admin_password) {
    $password = drupal_hash_base64($node->admin_password);
    $fields['admin_password'] = $password;
  }
  $id = db_update('apollo_instance')->fields($fields)
    ->condition('nid', $node->nid)
    ->execute();

  foreach ($node->records as $record) {

    db_delete('apollo_instance_record')
      ->condition('base_table', $node->base_table)
      ->condition('record_id', $record)
      ->execute();

    db_insert('apollo_instance_record')
      ->fields([
        'instance_id' => $id,
        'record_id' => $record,
        'base_table' => $node->base_table,
      ])
      ->execute();
  }

}

/**
 * Implements hook_delete().
 *
 * @param $node
 */
function apollo_instance_delete($node) {
  db_delete('apollo_instance')->condition('nid', $node->nid)->execute();

}

/**
 * Implements hook_load().
 *
 * @param $node
 */
function apollo_instance_load($nodes) {
  $sql = "
    SELECT *
    FROM {apollo_instance}
    WHERE nid IN (:nids)";
  $result = db_query($sql, [':nids' => array_keys($nodes)]);

  foreach ($result as $record) {
    // Add basic blast node information.
    $nodes[$record->nid]->instance_name = $record->name;
    $nodes[$record->nid]->url = $record->url;
    $nodes[$record->nid]->apollo_version = $record->apollo_version;
    $nodes[$record->nid]->base_table = $record->base_table;
    $nodes[$record->nid]->title = $record->name;
  }

  return $nodes;
}

/**
 * Implements hook_node_insert().
 * This function acts on ALL NODES
 */
function tripal_apollo_node_insert($node) {
  if ($node->type == 'apollo_instance') {
    $node->title = $node->instance_name;

    dpm($node);
  }
}

/**
 * Implements hook_node_insert().
 * This function acts on ALL NODES
 */
function tripal_apollo_node_update($node) {
  if ($node->type == 'apollo_instance') {
    $node->title = $node->instance_name;
  }
}

function tripal_apollo_record_callback($form, &$form_state) {
  return $form['core']['records'];
}


/**
 *
 * Build select options for all records of a given table.
 *
 * @param $base_table | expects organism, analysis, or project
 *
 * @return array | select option array of key -> label
 */
function tripal_apollo_build_record_select($base_table) {

  $table_key = $base_table . '_id';
  $select = [];

  $fields = [$table_key];

  switch ($base_table) {
    case 'organism':
      $fields[] = 'genus';
      $fields[] = 'species';
      break;
    case 'project':
      $fields[] = 'name';
      break;
    case 'analysis':
      $fields[] = 'name';
      break;
  }


  $query = db_select('chado.' . $base_table, 't')
    ->fields('t', $fields);

  $results = $query->execute()
    ->fetchAll();

  foreach ($results as $result) {
    $string = '';
    switch ($base_table) {
      case 'organism':
        $string = $result->genus . ' ' . $result->species;
        break;
      case 'project':
        $string = $result->name;
        break;
      case 'analysis':
        $string = $result->name;
        break;
    }

    $select[$result->$table_key] = $string;

  }

  return $select;
}