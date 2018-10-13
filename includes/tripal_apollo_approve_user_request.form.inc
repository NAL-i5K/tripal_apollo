<?php


/**
 * @param $form
 * @param $form_state
 *
 * @return mixed
 */
function tripal_apollo_approve_user_request_form($form, &$form_state, $submission_id) {

  $header = [];

  //note: UID is not user id.  uid is the webapollo_users.id
  $data = db_select('webapollo_users', 'w')
    ->fields('w', ['uid', 'name', 'pass', 'email', 'organisms', 'status'])
    ->condition('w.uid', $submission_id, '=')
    ->execute()->fetchObject();

  //TODO: on schema update, include the base table.

  $form['web_apollo_table'] = [
    '#tree' => TRUE, // Need this for easy submit processing
    '#header' => $header, // We trick Drupal to pass our $header to theme here
    '#theme' => 'tripal_apollo_approve_user_request_form',
  ];
  $form['web_apollo_table']['name'] = [
    '#type' => 'value',
    '#title' => t('Name'),
    '#value' => $data->name,
  ];

  $form['web_apollo_table']['email'] = [
    '#type' => 'value',
    '#value' => $data->email,
  ];


  //it says organisms here but in reality it will be only a single organism since we retrieved one record only.

  //todo: genercize to be any content type.
  $organism_object = chado_get_organism(['organism_id' => $data->organisms]);

  $organism_value = $organism_object->abbreviation . ' (' . $organism_object->common_name . ')';

  $form['web_apollo_table']['organism_key'] = [
    '#type' => 'value',
    '#value' => $data->organisms,
  ];

  $form['web_apollo_table']['organism'] = [
    '#type' => 'value',
    '#value' => $organism_value,
  ];

  //Look up instance for this organism
  //TODO: support non-organism base tables.



  $query = db_select('apollo_instance_record', 'aio');
  $query->join('apollo_instance', 'ai', 'ai.id  = aio.instance_id');
  $query->fields('ai', ['name', 'id']);
  $query->condition('aio.record_id', $organism_object->organism_id);
  $result  = $query->execute()
    ->fetchObject();

  $instance_name = $result->name;
  $instance_id = $result->id;


  $form['web_apollo_table']['instance_id'] = [
    '#type' => 'value',
    '#value' => $instance_id,
  ];

  $form['web_apollo_table']['instance'] = [
    '#type' => 'value',
    '#value' => $instance_name,
  ];

  $status = '';

  if ($data->status == 1) {
    $status = 'Approved';
  }
  elseif ($data->status == 0) {
    $status = 'Pending';
  }
  else {
    $status = 'Rejected';
  }

  $form['web_apollo_table']['status'] = [
    '#type' => 'radios',
    '#default_value' => $data->status,
    '#options' => [t('Reject'), t('Approve')],
  ];

  $form['web_apollo_table']['uid'] = [
    '#type' => 'value',
    '#value' => $data->uid,
  ];

  $form['web_apollo_table']['pass'] = [
    '#type' => 'value',
    '#value' => $data->pass,
  ];

  // Submit function
  $form['web_apollo_table']['submit'] = [
    '#type' => 'submit',
    '#name' => 'submit',
    '#value' => t('Save'),
  ];

  return $form;

}


/**
 * @hook_form_submit()
 * Runs the add_user.pl and set_track_permissions.pl to create user accounts
 *   and permissions for respective organisms.
 *
 * @sends an email to user regarding login credentials to the web apollo site.
 */

function tripal_apollo_approve_user_request_form_submit($form, &$form_state) {
  $values = $form_state['values']['web_apollo_table'];
  $values['type'] = 1;
  $status = $form_state['values']['web_apollo_table']['status'];//reject or approve
  $uid = $form_state['values']['web_apollo_table']['uid'];
  //TODO: not actually uid.  this is the submission id
  $to = $values['email'];
  $from = variable_get('site_mail');
  $status_res = db_select('webapollo_users', 'w')
    ->fields('w', ['status'])
    ->condition('w.uid', $uid, '=')
    ->execute()->fetchField();

  $instance_id = $form_state['values']['instance_id'];
  $record_id = $values['organism_key'];

  $query = db_select('apollo_instance_record', 'air')
    ->condition('air.record_id', $record_id);
  $query->join('apollo_instance', 'ai', 'ai.id = air.instance_id');
  $query->fields('ai');

  //one record = one instance, so only 1 result
  $instance_info = $query->execute()->fetchObject();

  if (($status_res != $status) && ($status == 1)) {//User is approved.


    $host = $instance_info->url;
    $db_name = $instance_info->db_name;
    $db_user = $instance_info->db_user_name;
    //TODO:  why doesnt it need the db password?

    $db_pass = $instance_info->db_password;

    /****
     * The goal is to make the password same for all organism for same emailid.
     * First check for the email already registered for any organism with status=1, if yes then take the most recent password and update in webapollo_users and apollo tables.
     * IF not then check for email with status=2, then take most recent password and update in webapollo_uses and apollo tables.
     */

    //TODO:  we will replace this with the new schema.
    $email = $to;
    $check_email_status1_exists = db_select('webapollo_users', 'w')
      ->fields('w', ['uid', 'name', 'pass', 'organisms'])
      ->condition('w.email', $email, '=')
      ->condition('w.status', 1, '=')
      ->orderby('w.created', 'desc')
      ->range(0, 1)
      ->execute()->fetchObject();

    $check_email_status2_exists = db_select('webapollo_users', 'w')
      ->fields('w', ['uid', 'name', 'pass', 'organisms'])
      ->condition('w.email', $email, '=')
      ->condition('w.status', 2, '=')
      ->orderby('w.created', 'desc')
      ->range(0, 1)
      ->execute()->fetchObject();


    $user = $values['email']; //Note: same as $to and $email.

    if (isset($check_email_status1_exists) && !empty($check_email_status1_exists)) {
      //user has an apollo 1 record: update THIS entry's password.
      $pass = $check_email_status1_exists->pass;
      db_update('webapollo_users')
        ->fields(['pass' => $pass])
        ->condition('uid', $uid, '=')
        ->execute();

    }
    elseif (isset($check_email_status2_exists) && !empty($check_email_status2_exists)) {
      //user has an apollo 2 record: update THIS entry's password with it.

      $pass = $check_email_status2_exists->pass;
      db_update('webapollo_users')
        ->fields(['pass' => $pass])
        ->condition('uid', $uid, '=')
        ->execute();
    }
    else {
      $pass = $values['pass'];
    }

    // If the values['pass'] should be the updated password from recent password.
    if ($values['pass'] != $pass) {
      $values['pass'] = $pass;
    }

    //TODO: get this from sitewide variable in admin area
    $python_path = variable_get('tripal_apollo_python_path');
    $script_path = drupal_get_path('module', 'tripal_apollo') . '/bin/adduser.py';

    //TODO:  drupal best practices running scripts?
    //TODO: get python path via admin setting.

    $exec_user = exec($python_path . " " . $script_path . " -dbuser " . $db_user . " -dbname " . $db_name . " -user " . $user . " -pwd " . $pass . " -host " . $host);


    //TODO:  sounds like apollo 1 support feature-level permissions.  How do we suppor this?

    // if apollo 1, do something

    //Only on prod we have data for species seqids.txt files, but not on dev. change the seqids.txt file while testing on gmod-dev server
    /*($organism_short_name == 'diacit')
      $seqids = '/app/data/other_species/'.$organism_short_name.'/seqids/seqids.txt';
    else
      $seqids = '/app/data/BCM_i5k_pilot/'.$organism_short_name.'/seqids/seqids.txt';

    $set_permissions =  shell_exec("perl set_track_permissions.pl -D ".$db_name." -U ".$db_user." -P ".$db_pass." -u ".$user." -t ".$seqids." -rw");
    */

    //DEVELOPMENT
    // $set_permissions =  shell_exec("perl set_track_permissions.pl -D ".$db_name." -U ".$db_user." -P ".$db_pass." -u ".$user." -t trackseq.txt -rw");

    //Send email to the user about the login credentials to the web apollo site
    $user_email_sent = drupal_mail('web_apollo', 'webapollo_user_approve_email', $to, language_default(), $values, $from, TRUE);

    if (!$user_email_sent) {
      drupal_set_message(t('Failed to send the email due to some technical problems'));
    }
    else {
      db_update('webapollo_users')// Table name no longer needs {}
      ->fields(['status' => $status])
        ->condition('uid', $uid, '=')
        ->execute();
      drupal_set_message(t('Successfully an email was sent to %email regarding the status approved.', ['%email' => $values['email']]), $type = 'status');
    }

    //Displaying status message on the site
    $status = ($status == 1) ? 'Approved' : 'Rejected';
    drupal_set_message(t("%name 's status was changed to %status", [
      '%name' => $form_state['values']['web_apollo_table']['name'],
      '%status' => $status,
    ]), $type = 'status');
  } // Status approved condition ends here

  elseif ($status == 0) {
    db_update('webapollo_users')
    ->fields(['status' => $status])
      ->condition('uid', $uid, '=')
      ->execute();
    drupal_set_message(t('The apollo request for %name has been successfuly rejected.', ['%name' => $values['name']]), $type = 'status');
  }
  $form_state['redirect'] = 'admin/tripal/apollo/users';
  return;
}


