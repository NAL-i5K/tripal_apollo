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

  $form['email_value'] = [
    '#type' => 'value',
    '#value' => $auser->email
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

    $status_string = tripal_apollo_convert_status_for_display($status);

    $rows[] = [$request->genus . ' ' . $request->species, $status_string];

  }


  $table = theme('table', ['header' => $headers, 'rows' => $rows]);
  $form['request_table'] = [
    '#markup' => $table,
  ];


  $form['instructions'] = [
    '#markup' => '<p><b>Warning!</b> 
               
                Rejecting this user will reject all their pending requests, and rescind all approved requests, for all organisms across all instances.</p>
            <p>Please note this will not delete the requests, or the associated Drupal user.</p>',
  ];


  $form['auid'] = [
    '#type' => 'value',
    '#value' => $auid,
  ];


  $form['delete_user'] = [
    '#type' => 'submit',
    '#submit' => ['tripal_apollo_user_delete'],
    '#value' => t('Reject Apollo User'),
  ];

  return $form;
}

function tripal_apollo_user_delete($form, &$form_state) {

  $auid = $form_state['values']['auid'];

  $email = $form_state['values']['email_value'];
  $result = tripal_apollo_purge_user($auid);

  //API CALLs: delete user in all attached instances.

  if (!$result) {
    tripal_set_message('Error: could not delete user from Apollo instances', TRIPAL_ERROR);
  }
  else {
    tripal_set_message(t('All user permissions removed for !user!', ['!user' => $email]), TRIPAL_NOTICE);

    $form_state['redirect'] = 'admin/tripal/apollo/requests';

    return;
  }
}


/**
 * Implements hook_form().
 * Provides a tab to the user page area so that users can see what apollo stuff
 * they have access to, and let them request more access from here as well.
 */

function tripal_apollo_user_permissions_page($form, &$form_state, $uid) {

  $form['title'] = ['#markup' => "<h2>You can use the below form to request access to Apollo, or view the status of your access.</h2>"];

  global $user;
  $base = variable_get('tripal_apollo_base_table');


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

  $header = [
    'Record',
    'Status',
    'Date Requested',
  ];

  $requests = tripal_apollo_user_existing_requests_table($au, $header);

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
    '#description' => 'Select one or more of the ' . $base . 's above.',
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


/**
 * Implements hook_validate()
 */
function tripal_apollo_user_permissions_page_validate($form, &$form_state) {

  $base = variable_get('tripal_apollo_base_table');

  $key = $base . 's';

  $requests = $form_state['values']['new_request'];
  if (empty($requests)) {
    form_set_error('new_request', t('Please select one or more !key to request Apollo access to.', ['!key' => $key]));
  }
}

/**
 * Implements hook_submit().
 * Submits requests for the records provided.
 */
function tripal_apollo_user_permissions_page_submit($form, &$form_state) {

  $requests = $form_state['values']['new_request'];

  $a_user = $form_state['values']['au'];

  $au_id = $a_user->id;

  if (!$a_user) {

    global $user;
    //create a user account for this person.

    $password = tripal_apollo_gen_xkcd_password(2);
    $password = tripal_apollo_encrypt($password);

    $au_id = db_insert('apollo_user')
      ->fields([
        'uid' => $user->uid,
        'name' => $user->name,
        'pass' => $password,
        'email' => $user->mail,
        //  'institution' => $values['institution'],
        //  'comment' => $values['comments'],
      ])
      ->execute();
  }

  foreach ($requests as $record_id) {
    db_insert('apollo_user_record')
      ->fields([
        'record_id' => $record_id,
        'apollo_user_id' => $au_id,
        'status' => 2,
        'created' => time(),
      ])
      ->execute();
    //send emails

  }
}
