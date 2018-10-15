<?php


/**
 * @param $form
 * @param $form_state
 *
 * @return mixed
 */
function tripal_apollo_approve_user_request_form($form, &$form_state, $submission_id) {

  $header = [];

  $query = db_select('apollo_user_record', 'aur')
    ->fields('aur', ['record_id', 'status'])
    ->condition('aur.id', $submission_id);
  $query->join('apollo_user', 'au', 'aur.apollo_user_id = au.id');
  $query->fields('au', ['name', 'pass', 'email', 'institution', 'comment']);
  $data = $query->execute()->fetchObject();


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


  if (variable_get('tripal_apollo_base_table') == 'organism') {
    $pkey = "organism_id";
    $record_object = chado_get_organism(['organism_id' => $data->record_id]);

    $organism_value = $record_object->abbreviation . ' (' . $record_object->common_name . ')';

    $form['web_apollo_table']['organism_key'] = [
      '#type' => 'value',
      '#value' => $data->record_id,
    ];

    $form['web_apollo_table']['organism'] = [
      '#type' => 'value',
      '#value' => $organism_value,
    ];
  }
  else {
    //todo: genercize to be any content type.
  }

  //Look up instance for this record

  $query = db_select('apollo_instance_record', 'aio');
  $query->join('apollo_instance', 'ai', 'ai.id  = aio.instance_id');
  $query->fields('ai', ['name', 'id']);
  $query->condition('aio.record_id', $record_object->$pkey);
  $result = $query->execute()
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

  //TODO:  this is not the drupal uid.  Rename the value (here and in theme file)

  $form['web_apollo_table']['uid'] = [
    '#type' => 'value',
    '#value' => $submission_id,
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
 * Implements hook_form_submit().
 * Runs the add_user.pl and set_track_permissions.pl to create user accounts
 *   and permissions for respective organisms.
 *
 * @sends an email to user regarding login credentials to the web apollo site.
 */

function tripal_apollo_approve_user_request_form_submit($form, &$form_state) {
  $values = $form_state['values']['web_apollo_table'];
  $values['type'] = 1;
  $status = $form_state['values']['web_apollo_table']['status'];//reject or approve
  $submission_id = $form_state['values']['web_apollo_table']['uid'];
  //TODO: not actually uid.  this is the submission id
  $to = $values['email'];
  $from = variable_get('site_mail');
  $status_res = db_select('apollo_user_record', 'aur')
    ->fields('aur', ['status'])
    ->condition('aur.id', $submission_id)
    ->execute()->fetchField();

  //$instance_id = $form_state['values']['instance_id'];

  $record_id = $values['organism_key'];

  $query = db_select('apollo_instance_record', 'air')
    ->condition('air.record_id', $record_id);
  $query->join('apollo_instance', 'ai', 'ai.id = air.instance_id');
  $query->fields('ai');

  //one record = one instance, so only 1 result
  $instance_info = $query->execute()->fetchObject();

  if (($status_res != $status) && ($status == 1)) {//User is approved.

    $host = $instance_info->url;
    $db_name = $instance_info->database_name;
    $db_user = $instance_info->admin_name;
    //TODO:  why doesnt it need the db password?

    $db_pass = $instance_info->admin_password;

    $email = $to;

    $user = $values['email']; //Note: same as $to and $email.
    $user_pass = $values['pass'];

    $python_path = variable_get('tripal_apollo_python_path');
    $script_path = drupal_get_path('module', 'tripal_apollo') . '/bin/adduser.py';

    //TODO:  drupal best practices running scripts?
    //TODO: get python path via admin setting.

    $exec_user = exec($python_path . " " . $script_path . " -dbuser " . $db_user .
      " -dbname " . $db_name .
      "-dbpass " . $db_pass .
      " -user " . $user .
      " -pwd " . $user_pass .
      " -host " . $host);


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
      db_update('apollo_user_record')
        ->fields(['status' => $status])
        ->condition('id', $submission_id)
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
    db_update('apollo_user_record')
      ->fields(['status' => $status])
      ->condition('id', $submission_id)
      ->execute();
    drupal_set_message(t('The apollo request for %name has been successfuly rejected.', ['%name' => $values['name']]), $type = 'status');
  }
  $form_state['redirect'] = 'admin/tripal/apollo/users';
  return;
}
