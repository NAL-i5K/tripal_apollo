<?php

/**
 * Implements hook_form().
 *
 * @param $auid : apollo user id.
 */
function tripal_apollo_user_form($form, &$form_state, $auid) {


  $auser = db_select('apollo_user', 'au')
    ->fields('au')
    ->condition('id', $auid)
    ->execute()
    ->fetchObject();

  if ($auser->uid) {
    //load user id
  }

  $form['name'] = [
    "#type" => 'item',
    '#markup' => $auser->name,
    '#title' => "Name",
  ];

  $form['email'] = [
    "#type" => 'item',
    '#markup' => $auser->email,
    '#title' => "Email",
  ];


  $base = variable_get('tripal_apollo_base_table');
  $base_key = $base . '_id';


  $query = db_select('apollo_user_record', 'aur')
    ->condition('apollo_user_id', $auser->id);
  $query->fields('aur', ['status']);
  $query->join('chado.' . $base, 'ct', 'ct.' . $base_key . ' = aur.record_id');

  if ($base == 'organism') {
    $query->fields('ct', ['genus', 'species']);
  }
  $requests = $query->execute();
  //$requests = $query->fetchAll();

  $headers = [$base, 'status'];
  $rows = [];

  foreach ($requests as $request) {

    $status = $request->status;

    switch ($status) {
      case '0':
        $status_string = 'Pending';
        break;
      case '1':
        $status_string = 'Approved';
        break;
      case '2':
        $status_string = 'Denied';
        break;
      default:
        $status_string = 'Unknown';
        break;
    }
    $rows[] = [$request->genus . ' ' . $request->species, $status_string];

  }


  $table = theme('table', ['header' => $headers, 'rows' => $rows]);
  $form['request_table'] = [
    '#markup' => $table,
  ];


  $form['instructions'] = [
    '#markup' => '<p><b>Warning!</b>  Deleting this user will delete all their requests and all accounts across all instances.</p><p>Please note this will not delete the associated Drupal user.</p>

<p>For now the delete user functionality is under development and disabled.</p>',
  ];


  $form['auid'] = [
    '#type' => 'value',
    '#value' => $auid,
  ];


  $form['delete_user'] = [
    '#type' => 'submit',
    '#submit' => ['tripal_apollo_user_delete'],
    '#value' => t('Delete Apollo User'),
    '#disabled' => TRUE,
  ];

  return $form;
}

function tripal_apollo_user_delete($form, &$form_state) {

  $auid = $form_state['values']['auid'];

  $result = tripal_apollo_purge_user($auid);

  //API CALLs: delete user in all attached instances.

  if (!$result) {
    tripal_set_message('Error: could not delete user from Apollo instances', TRIPAL_ERROR);
  }
  //Now: delete all apollo_user based records.
  db_delete('apollo_user')
    ->condition('id', $auid)
    ->execute();

  $form_state['redirect'] = variable_get('site_frontpage');

}


/**
 * Implements hook_form().
 * Provides a tab to the user page area so that users can see what apollo stuff
 * they have access to, and let them request more access from here as well.
 */

function tripal_apollo_user_permissions_page($form, &$form_state, $uid) {

  $form['title'] = ['#markup' => "<h2>You can use the below form to request access to Apollo, or view the status of your access.</h2>"];

  global $user;

  $email = $user->mail;

  $au = db_select('apollo_user', 't')
    ->fields('t')
    ->condition('email', $email)
    ->execute()
    ->fetchObject();

  if (!$au) {
    //User might have an account but with a different email.
    $au = db_select('apollo_user', 't')
      ->fields('t')
      ->condition('uid', $user->uid)
      ->execute()
      ->fetchObject();
  }

  $form['au'] = ['#type' => 'value', '#value' => $au];

  if (!$au) {
    //User hasn't registered. Go right to offering registration.
    $form['register'] = ['#markup' => 'Register!  coming soon'];
    return $form;
  }


  $base = variable_get('tripal_apollo_base_table');

  $pkey = $base . '_id';


  $header = [
    'Record',
    'Status',
    'Date Requested',
  ];

  $query = db_select('apollo_user_record', 'aur')
    ->fields('aur', ['status', 'created', 'record_id'])
    ->condition('apollo_user_id', $au->id);
  $query->join('chado.' . $base, 'bt', 'bt.' . $pkey . ' = aur.record_id');
  $query = $query->extend('TableSort')
    // ->orderByHeader($header)
    ->extend('PagerDefault')
    ->limit(25);

  if ($base === 'organism') {
    $query->fields('bt', ['genus', 'species']);
  }
  else {
    $query->fields('bt', ['name']);
  }

  $requests = $query->execute();

  $rows = [];
  $existing = [];
  //build table of statuses.
  foreach ($requests as $request) {
    $existing[$request->record_id] = $request->record_id;
    if ($base == 'organism') {
      $name = $request->genus . ' ' . $request->species;
    }
    else {
      $name = $request->name;
    }
    $date = date('M d Y h:i:s A', $request->created);

    $num_status = $request->status;

    $status = tripal_apollo_convert_status_for_display($num_status);
    $rows[] = [$name, $status, $date];
  }

  $table = theme('table', ['header' => $header, 'rows' => $rows]);

  $form['existing_request'] = ['#markup' => $table];
  //get records the user hasnt requested


  $all_records = tripal_apollo_get_eligible_records();

  foreach ($existing as $key) {

    unset($all_records[$key]);
  }
  //provide multiselect tool.  Don't let people withdraw requests!

  $form['new_request'] = [
    '#title' => 'Request Access',
    '#description' => 'Select one or more of the ' . $base . 's below',
    '#type' => 'select',
    '#multiple' => TRUE,
    '#options' => $all_records,
  ];

  $form['submit'] = [

    '#type' => 'submit',
    '#value' => 'Request Access',
  ];

  return $form;

}

