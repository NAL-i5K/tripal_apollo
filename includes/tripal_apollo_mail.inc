<?php

/**
 * Implements @hook_mail
 */
function tripal_apollo_mail($key, &$message, $params) {
  $language = $message['language'];
  $annotation = [1, 2];

  $organism_data = [];

  $site_name = variable_get('site_name');
  $site_email = variable_get('site_mail');

  $organism_value = "TODO: set this with chado instead";
//
//  if (is_array($params['organism'])) {
//    $organism_value = '';
//    $num_of_organism = count($params['organism']);
//    $i = 1;
//
//    foreach ($params['organism'] as $keys => $organism_val) {
//      $separator = ($i == ($num_of_organism - 1)) ? ' and ' : ', ';
//      $organism_value .= trim($organism_data[$organism_val]) . $separator;
//      $i++;
//    }
//    $organism_value = rtrim($organism_value, ", ");
//  }
//  else {
//    $organism_value = isset($params['organism_key']) ? $organism_data[$params['organism_key']] : '';
//  }

  $variables = [
    '!organism_data' => $organism_data,
    '!organism' => $organism_value,
    '!user' => $params['name'],
    '!form' => url($_GET['q'], ['absolute' => TRUE]),
  ];

  switch ($key) {
    //switching on $key lets you create variations of the email based on the $key parameter
    case 'notify_admin_organism_request':
      $message['subject'] = t('Web Apollo Registration', $variables, ['langcode' => $language->language]); //$variabless required even if not used to get $language in there

      //the email body is here, inside the $message array
      $message['body'][] = t("!user sent a message using the web apollo registration form at !form.", $variables, ['langcode' => $language->language]);
      $message['body'][] = '<br><b>Organism(s):</b> ' . $organism_value . '<Br>
<b>Institution:</b> ' . $params['institution'] . '<br>
<b>Other Comments:</b> ' . $params['comments'];
      $message['body'][] = '<br><BR>Regards,<br><bR>The ' . $site_name . 'team';
      $message['headers']['Content-Type'] = 'text/html; charset=UTF-8; format=flowed';

      break;
    case 'notify_user_organism_request':
      $get_user_before_at = explode("@", $params['mail']);
      $message['subject'] = t('Web Apollo registration for !organism', $variables, ['langcode' => $language->language]);
      $message['body'][] = t("Dear !user, <br><br> Thank you for registering to annotate !organism.", $variables, ['langcode' => $language->language]);
      $message['body'][] = 'Once approved by the ' . $organism_value;

      if (isset($num_of_organism) && ($num_of_organism > 1)) {
        $community_contact = "community contacts";
      }
      else {
        $community_contact = "community contact";
      }

      $message['body'][] = trim($community_contact) . ", we will send your account information to the email provided. Contact us if you do not receive an email with your login credentials within one business day.";
      $message['body'][] = '<br><BR>Regards,<br><bR>The ' . $site_name . 'team';
      $message['headers']['Content-Type'] = 'text/html; charset=UTF-8; format=flowed';
      break;
    case 'webapollo_user_approve_email':
      $message['subject'] = t('Web Apollo user account', $variables, ['langcode' => $language->language]);
      $message['body'][] = "Dear " . $params['name'] . ", <br><br>We have approved your Web Apollo application for " . $params['organism'] . ". Please keep this email for future reference.";
      $message['body'][] = '<br><br><b>Username: </b>' . $params['email'];
      $message['body'][] = '<br><br><b>Password: </b>' . $params['pass'];

      //TODO:  Use the new web apollo instances to deal with this.
      //Apollo one: Link to site/organism_key
      //APollo two: link to apollo/annotator/index

      $message['body'][] = '<p>The Apollo software is collaborative - many annotators from different labs, universities, and even countries work together on improving the annotations of a genome assembly. We thank you for joining this community!</p>>';
      $message['body'][] = '<p>If you notice that your gene is already in the user-created annotations track, this means that another annotator is already working on it. You can hover over the model to see who the annotator is. Before you work on the model, please get in touch with this annotator, so you can improve the annotation together. If you don\'t know how to reach the annotator, get in touch with us ' . $site_email . ', and we\'ll help.</p>';
      $message['body'][] = '<br><br>Annotation guidelines:<bR> http://genomearchitect.org/users-guide';
      $message['body'][] = '<Br><br>You will receive a separate email containing your subscription information, including instructions on how to opt out of the mailing list if you choose.';
      $message['body'][] = '<br><BR>Regards,<br><bR>The ' . $site_name . 'team';
      $message['headers']['Content-Type'] = 'text/html; charset=UTF-8; format=flowed';
      break;
  }
}