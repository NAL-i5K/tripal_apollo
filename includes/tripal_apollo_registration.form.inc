<?php

/**
 *  hook_form() registration form for the web apollo
 */
function tripal_apollo_registration_form($form, &$form_state) {

  $form['instructions'] = [
    '#markup' => '<div id=\"tripal_apollo_param\">Complete the form below and click \'Submit\' to register for an Apollo account.  Only registered users can view, create or change annotations.</div>',
  ];


  $choices = tripal_apollo_get_eligible_records();
  if (!$choices) {
    $form['instructionsp2'] = [
      '#markup' => '<p>Apollo configuration is still underway.  <p>Please contact a site administrator</p></p></div>',
    ];
    return $form;
  }

  //remove records user is already associated with, and the prebuilt null selection.

  global $user;

  $associated_records = [];
  //if we're logged in, remove the records already associated.

  if ($user->uid > 0) {
    $form['uid'] = [
      '#type' => 'value',
      '#value' => $user->uid,
    ];

    $query = db_select('apollo_user_record', 'aur')
      ->fields('aur', ['record_id']);
    $query->join('apollo_user', 'au', 'aur.apollo_user_id = au.id');
    $query->condition('au.uid', $user->uid);

    $results = $query->execute()->fetchAll();

    $base = variable_get('tripal_apollo_base_table');
    $pkey = $base . '_id';

    foreach ($results as $result) {
      $record_id = $result->record_id;
      unset($choices[$record_id]);


      if ($base == 'organism') {
        $chado_record = chado_select_record($base, [
          'genus',
          'species',
        ], [$pkey => $record_id]);
        $chado_record = $chado_record[0];

        $label = $chado_record->genus . ' ' . $chado_record->species;
      }
      else {
        $chado_record = chado_select_record($base, ['name'], [$pkey => $record_id]);
        $chado_record = $chado_record[0];
        $label = $chado_record->name;

      }
      $associated_records[] = $label;
    }

    if (!empty($associated_records)) {
      $message = 'You have already requested access to the following  ' . $base . 's :<ul>';
      foreach ($associated_records as $label) {
        $message .= '<li> ' . $label . '</li>';
      }
      $message .= '</ul>';
      $form['existing'] = [
        '#type' => 'markup',
        '#markup' => $message,
      ];
    }
  }

  $form['name'] = [
    '#type' => 'textfield',
    '#title' => t('Full Name'),
    '#description' => t('Please provide your first and last name separated by a space.'),
    '#maxlength' => 255,
    '#required' => TRUE,
  ];

  $form['mail'] = [
    '#type' => 'textfield',
    '#title' => t('Email Address'),
    '#maxlength' => 255,
    '#required' => TRUE,
    '#description' => 'The email address you will use to log in to the Apollo server.',
    '#default_value' => NULL,
  ];


  if (count($choices) == 0) {

    $form['exit'] = [
      '#markup' => '<p>There are no additional ' . $base . 's with Apollo set up.</p>',
    ];
    return $form;

  }

  $form['organism'] = [
    '#type' => 'select',
    '#title' => t('!type Content', [
      '!type' => ucfirst($base),
    ]),
    '#options' => $choices,
    '#multiple' => TRUE,
    '#required' => TRUE,
    '#empty_option' => "Please select",
    '#description' => 'The ' . $base . '(s)  you would like access to.',
  ];

  $form['institution'] = [
    '#type' => 'textfield',
    '#title' => t('Institution'),
    '#required' => TRUE,
  ];

  $form['comments'] = [
    '#type' => 'textarea',
    '#title' => t('Genes or gene families that you intend to annotate'),
    '#required' => TRUE,
  ];

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Submit'),
  ];

  return $form;
}

/**
 * Implements hook_validate().
 *
 * Web apollo registration form validation
 * 1 - organism field is required
 * 2 - Email should be valid
 * 3 - Using same emailId cannot registered for the same organism
 */
function tripal_apollo_registration_form_validate($form, &$form_state) {


  //Character validation
  $name = $form_state['input']['name'];
  if (!preg_match('!^[\w @-]*$!', $name)) {
    form_set_error('name', t('Your Name can only contains numbers, letters @  - _.'));
  }
  $institution = $form_state['input']['institution'];
  if (!preg_match('!^[\w @-]*$!', $institution)) {
    form_set_error('institution', t('Your institution name can only contains numbers, letters @  - _.'));
  }
  if (!valid_email_address($form_state['values']['mail'])) {
    form_set_error('mail', t('You must enter a valid e-mail address.'));
  }
  $organisms = $form_state['values']['organism'];
  $email = $form_state['values']['mail'];

  //Check if this user is already associated with this organism.

  if (!$organisms || $email) {
    return;
  }
  $query = db_select('apollo_user_record', 'aur');
  $query->join('apollo_user', 'au', 'au.id = aur.apollo_user_id');
  $query->fields('aur', ['id'])
    ->condition('au.email', $email)
    ->condition('aur.record_id', $organisms, 'IN');
  $results = $query->execute()
    ->fetchAll();

  if ($results) {
    $message = "You are already registered for the following organisms: ";

    foreach ($results as $key => $result) {


      $record_object = chado_select_record('organism', [
        'abbreviation',
      ], ['organism_id' => $result->record_id]);
      $organism = $record_object[0];
      $message .= $organism->abbreviation . ", ";
    }
    form_set_error('organism', $message);

  }
}

/**
 * Implements hook_submit().
 * Creates requests for each selected record for the user.
 * Notifies user and admin via email.
 */
function tripal_apollo_registration_form_submit($form, &$form_state) {
  $admin_email = variable_get('site_mail');
  $values = $form_state['values'];

  //Is there a user record?
  $email = $values['mail'];
  $uid = isset($values['uid']) ? $values['uid'] : NULL;

  $query = db_select('apollo_user', 'au')
    ->fields('au', ['uid', 'pass', 'email', 'id'])
    ->condition('au.email', $email);
  $result = $query->execute()->fetchObject();

  //If not, create password and user record


  if ($result) {
    $password = tripal_apollo_decrypt($result->pass);
    $au_id = $result->id;

  }
  else {
    $password = tripal_apollo_gen_xkcd_password(2);

    $password = tripal_apollo_encrypt($password);

    $au_id = db_insert('apollo_user')
      ->fields([
        'uid' => $uid,
        'name' => $values['name'],
        'pass' => $password,
        'email' => $email,
        'institution' => $values['institution'],
        'comment' => $values['comments'],
      ])
      ->execute();
  }

  //does this email have records?

  $pre_check = db_select('apollo_user_record', 't')
    ->fields('t', ['record_id'])
    ->condition('apollo_user_id', $au_id)
    ->execute()
    ->fetchAll();

  $existing_organisms = [];

  foreach ($pre_check as $record) {
    $existing_organisms[$record->record_id] = $record->record_id;
  }

  //  //write link with status 0
  foreach ($values['organism'] as $record_id => $organism_val) {

    if (array_key_exists($record_id, $existing_organisms)) {
      //dont want an error if an anonymous user is submitting an email/organism combination that already exists.
      continue;
    }
    db_insert('apollo_user_record')
      ->fields([
        'record_id' => $record_id,
        'apollo_user_id' => $au_id,
        'status' => 2,//pending request
        'created' => time(),
      ])
      ->execute();

  }


  //send emails
  $from = $values['name'] . "<" . $values['mail'] . ">";
  $sent = drupal_mail('tripal_apollo', 'notify_admin_organism_request', $admin_email, language_default(), $values, $from, TRUE);


  if (empty($sent['result']) || ($sent['result'] != 1)) {
    /*handle send fail, $sent ===false when mail fails, but it won't always recognize a failure*/
    drupal_set_message(t('Error: Failed to send e-mail notification.'));
  }
  else {
    drupal_set_message(t('Thank you for requesting Apollo access. You will receive an email confirmation shortly.'));
  }

  //Email to the registered user stating successfully registered and once approved from admin you can login to the site
  $user_email_sent = drupal_mail('tripal_apollo', 'notify_user_organism_request', $values['mail'], language_default(), $values, $admin_email, TRUE);


  if (empty($user_email_sent['result']) || ($user_email_sent['result'] != 1)) {
    drupal_set_message(t('Error: Failed to send e-mail notification.'));

  }

  //return to homepage
  $form_state['redirect'] = variable_get('site_frontpage');
}
