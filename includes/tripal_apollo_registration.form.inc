<?php

//TODO:  I would prefer to make this all happen in Vanilla drupal upon standard user account creation.


/**
 *  hook_form() registration form for the web apollo
 */
function tripal_apollo_registration_form($form, &$form_state) {
//  $form['#attached']['css'] = [
//    drupal_get_path('module', 'tripal_apollo') . '/css/registration.css',
//  ];


  $form['instructions'] = [
    '#markup' => '<div id=\"tripal_apollo_param\">Complete the form below and click \'Submit\' to register for an Apollo account.  Only registered users can view, create or change annotations.</div>'
  ];
  $form['name'] = [
    '#type' => 'textfield',
    '#title' => t('Full Name'),
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

  //TODO: functionality could allow you to associate organisms with web apollo if necessary?  Otherwise, just all chado organisms.

  $organisms = chado_get_organism_select_options(FALSE);

  unset($organisms[0]);

  $form['organism'] = [
    '#type' => 'select',
    '#title' => t('Organism'),
    '#options' => $organisms,
    '#multiple' => TRUE,
    '#required' => TRUE,
    '#empty_option' => "Please select an Organism",
    '#description' => 'The organisms you would like access to.',
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

  //Check if this email is already associated with this organism.

  $results = db_select('webapollo_users', 'w')
    ->fields('w', ['uid', 'organisms'])
    ->condition('w.email', $email)
    ->condition('w.organisms', $organisms, 'IN')
    ->execute()
    ->fetchAll();

  $cnt = 0;

  if ($results) {
    $message = "You are already registered for the following organisms:\n";

    foreach ($results as $key => $result) {

      //TODO: Previous entries store a special organism/genus key instead of organism_id currently.
      //see @ticket 11
      $organism = chado_get_organism(['organism_id' => $result->organisms]);
      $message .= $organism->abbreviation . "\n";
    }
    form_set_error('organism', $message);

  }
}

function tripal_apollo_registration_form_submit($form, &$form_state) {
  $admin_email = variable_get('site_mail');
  $values = $form_state['values'];

  /****
   * The goal is to make the password same for all organism for a given email.
   * First check for the email already registered for any organism with status=1, if yes then take the most recent password.
   * IF not then check for email with status=2, then take most recent password and update in webapollo_users table.
   */

  //TODO:  Switching to a different schema would be really helpful here.

  $email = $values['mail'];
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

  if (isset($check_email_status1_exists) && !empty($check_email_status1_exists)) {
    $pass = $check_email_status1_exists->pass;;
  }
  elseif (isset($check_email_status2_exists) && !empty($check_email_status2_exists)) {
    $pass = $check_email_status2_exists->pass;
  }
  else {
    $pass = tripal_apollo_gen_xkcd_password(2);
  }

  $created = time();

  foreach ($values['organism'] as $key => $organism_val) {
    $data = [
      'name' => $values['name'],
      'pass' => $pass,
      'email' => $values['mail'],
      'organisms' => $organism_val,
      'institution' => $values['institution'],
      'comment' => $values['comments'],
      'created' => $created,
    ];
    drupal_write_record('webapollo_users', $data);
  }

  // E-mail address of the sender: as the form field is a text field.
  $from = $values['name'] . "<" . $values['mail'] . ">";

  // Send the e-mail to the recipients using the site default language.
  $sent = drupal_mail('tripal_apollo', 'notify_admin_organism_request', $admin_email, language_default(), $values, $from, TRUE);

  if (empty($sent['result']) || ($sent['result'] != 1)) {
    /*handle send fail, $sent ===false when mail fails, but it won't always recognize a failure*/
    drupal_set_message(t('Failed to send e-mail.'));
  }
  else {
    drupal_set_message(t('Your message has been sent. You will receive an email confirmation shortly.'));
  }

  //Email to the registered user stating successfully registered and once approved from admin you can login to the site
  $user_email_sent = drupal_mail('tripal_apollo', 'notify_user_organism_request', $values['mail'], language_default(), $values, $admin_email, TRUE);

  if (empty($user_email_sent['result']) || ($user_email_sent['result'] != 1)) {
    drupal_set_message(t('Failed to send the email due to some technical problems'));
  }

  // Jump to home page rather than back to web apollo registration page to avoid
  // contradictory messages if flood control has been activated.
  $form_state['redirect'] = variable_get('site_frontpage');
}