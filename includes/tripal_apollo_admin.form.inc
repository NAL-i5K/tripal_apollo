<?php

/**
 * Builds the user requet table for admin to approve/reject.
 * Implements hook_form().
 */
function tripal_apollo_admin_form($form, &$form_state) {
  //TODO: this should be its own form.  After that,ideally, a site should use default drupal users and permissions instead.
  $user_info = webapollo_register_users();

  $form['users'] = ['#markup' => $user_info];

  return $form;

}

/**
 * List the web apollo registered users with pagination and
 * with Approve/Reject options
 **/
function webapollo_register_users() {
  $output = "";

  if (isset($_GET['sort']) && isset($_GET['order'])) {
    // Sort it Ascending or Descending?
    if ($_GET['sort'] == 'asc') {
      $sort = 'ASC';
    }
    else {
      $sort = 'DESC';
    }

    // Which column will be sorted
    switch ($_GET['order']) {
      case 'Name':
        $order = 'name';
        break;
      case 'Email':
        $order = 'email';
        break;
      case 'Organism':
        $order = 'organisms';
        break;
      case 'Status':
        $order = 'status';
        break;
      default:
        $order = 'created';
    }
  }
  else {
    // Default sort
    $sort = 'DESC';
    $order = 'created';
  }

  $query = db_select("webapollo_users", "w");
  $query->fields("w", [
    "uid",
    "name",
    "email",
    "organisms",
    "status",
    "created",
  ]);
  $query->orderBy($order, $sort);
  $query = $query->extend('TableSort')->extend('PagerDefault')->limit(25);

  $result = $query->execute();
  $header = [
    ["data" => t('Name'), "field" => "name"],
    ["data" => t('Email'), "field" => "email"],
    ["data" => t('Organism'), "field" => "Organisms"],
    ["data" => t('Status'), "field" => "status"],
    ["data" => t('Created'), "field" => "created", 'sort' => 'desc'],
    [],
  ];

  // Now build the form array
  $rows = [];
  $i = 0;

  // Looping for filling the table rows
  while ($data = $result->fetchObject()) {
      // Fill the table rows
      $rows[$i][] = $data->name;
      $rows[$i][] = $data->email;
      $rows[$i][] = 'organism here';
      $status = '';
      if ($data->status == 1) {
        $status = 'Approved';
      }
      elseif ($data->status == 0) {
        $status = 'Rejected';
      }
      elseif ($data->status == 2) {
        $status = 'Pending';
      }
      $rows[$i][] = $status;
      $rows[$i][] = date('M d Y h:i:s A', $data->created);
      $rows[$i][] = l(t('Edit'), 'admin/tripal/apollo/users/' . $data->uid);
      $i++;
  }

  // Output of table with the paging
  $output = theme_table(
      [
        "header" => $header,
        "rows" => $rows,
        "attributes" => [],
        "sticky" => TRUE,
        // Table header will be sticky
        "caption" => "",
        "colgroups" => [],
        "empty" => t("Table has no row!")
        // The message to be displayed if table is empty
      ]
    ) . theme("pager");


  return $output;
}


/**
 * Search key by value from an array
 **/
function searchkey($organism_data, $needle) {
  foreach ($organism_data as $key => $data) {
    if (strcmp($key, $needle) == 0) {
      return $key;
    }
  }
  return FALSE;
}

/**
 * List the web apollo registered users with pagination and
 * with Approve/Reject options
 **/
function webapollo2_register_users() {
  $output = "";

  if (isset($_GET['sort']) && isset($_GET['order'])) {
    // Sort it Ascending or Descending?
    if ($_GET['sort'] == 'asc') {
      $sort = 'ASC';
    }
    else {
      $sort = 'DESC';
    }

    // Which column will be sorted
    switch ($_GET['order']) {
      case 'Name':
        $order = 'name';
        break;
      case 'Email':
        $order = 'email';
        break;
      case 'Organism':
        $order = 'organisms';
        break;
      case 'Status':
        $order = 'status';
        break;
      default:
        $order = 'created';
    }
  }
  else {
    // Default sort
    $sort = 'DESC';
    $order = 'created';
  }

  // Select table
  $query = db_select("webapollo_users", "w");
  // Select fields
  $query->fields("w", [
    "uid",
    "name",
    "email",
    "organisms",
    "status",
    "created",
  ]);
  // Set order by
  $query->orderBy($order, $sort);
  // For pagination
  $query = $query->extend('TableSort')->extend('PagerDefault')->limit(25);

  // Execute query
  $result = $query->execute();

  // Prepare table header
  // Prepare table header
  $header = [
    ["data" => t('Name'), "field" => "name"],
    ["data" => t('Email'), "field" => "email"],
    ["data" => t('Organism'), "field" => "Organisms"],
    ["data" => t('Status'), "field" => "status"],
    ["data" => t('Created'), "field" => "created", 'sort' => 'desc'],
    [],
  ];

  // Now build the form array
  $web_apollo_organism = variable_get('web_apollo_organism');
  $organism_data = get_webapollo_organisms($web_apollo_organism, 2);
  $rows = [];
  $i = 0;

  // Looping for filling the table rows
  while ($data = $result->fetchObject()) {
    if (array_key_exists($data->organisms, $organism_data)) {
      // Fill the table rows
      $rows[$i][] = $data->name;
      $rows[$i][] = $data->email;
      $rows[$i][] = (isset($organism_data[$data->organisms])) ? $organism_data[$data->organisms] : '';
      $status = '';
      if ($data->status == 1) {
        $status = 'Approved';
      }
      elseif ($data->status == 0) {
        $status = 'Rejected';
      }
      elseif ($data->status == 2) {
        $status = 'Pending';
      }
      $rows[$i][] = $status;
      $rows[$i][] = date('M d Y h:i:s A', $data->created);
      $rows[$i][] = l(t('Edit'), 'webapollo/user2/' . $data->uid . '/edit');
      $i++;
    }
  }

  // Output of table with the paging
  $output = theme_table(
      [
        "header" => $header,
        "rows" => $rows,
        "attributes" => [],
        "sticky" => TRUE,
        // Table header will be sticky
        "caption" => "",
        "colgroups" => [],
        "empty" => t("Table has no row!")
        // The message to be displayed if table is empty
      ]
    ) . theme("pager");


  return $output;
}

/**
 * @hook_form()
 * Registered user form with options Approve/Reject
 */
function webapollo2_user_approval($form, &$form_state, $uid) {
  $header = [];

  // sql here
  $data = db_select('webapollo_users', 'w')
    ->fields('w', ['uid', 'name', 'pass', 'email', 'organisms', 'status'])
    ->condition('w.uid', $uid, '=')
    ->execute()->fetchObject();


  // Now build the form array
  $form = [];

  $form['web_apollo2_table'] = [
    '#tree' => TRUE, // Need this for easy submit processing
    '#header' => $header, // We trick Drupal to pass our $header to theme here
    '#theme' => 'webapollo2_user_approval',
  ];
  $form['web_apollo2_table']['name'] = [
    '#type' => 'value',
    '#title' => t('Name'),
    '#value' => $data->name,
  ];
  $form['web_apollo2_table']['email'] = [
    '#type' => 'value',
    '#value' => $data->email,
  ];

  $web_apollo_organism = variable_get('web_apollo_organism');
  $organism_data = get_webapollo_organisms($web_apollo_organism, 2);
  $organism_value = $organism_data[$data->organisms];

  $form['web_apollo2_table']['organism_key'] = [
    '#type' => 'value',
    '#value' => $data->organisms,
  ];

  $form['web_apollo2_table']['organism'] = [
    '#type' => 'value',
    '#value' => $organism_value,
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

  $form['web_apollo2_table']['status'] = [
    '#type' => 'radios',
    '#default_value' => $data->status,
    '#options' => [t('Reject'), t('Approve')],
  ];

  $form['web_apollo2_table']['uid'] = [
    '#type' => 'value',
    '#value' => $data->uid,
  ];

  $form['web_apollo2_table']['pass'] = [
    '#type' => 'value',
    '#value' => $data->pass,
  ];

  // Submit function
  $form['web_apollo2_table']['submit'] = [
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
 * @sends an email to user regarding login credentails to the web apollo site.
 */
function webapollo2_user_approval_submit($form, &$form_state) {
  $values = $form_state['values']['web_apollo2_table'];
  $values['type'] = 2;
  $status = $form_state['values']['web_apollo2_table']['status'];
  $uid = $form_state['values']['web_apollo2_table']['uid'];
  $to = $values['email'];
  $from = FROM_ADDRESS;
  $status_res = db_select('webapollo_users', 'w')
    ->fields('w', ['status'])
    ->condition('w.uid', $uid, '=')
    ->execute()->fetchField();

  if (($status_res != $status) && ($status == 1)) {
    /***************web apollo automated login credentials*******************************/
    $web_apollo_organism = variable_get('web_apollo_organism');
    if (!empty($web_apollo_organism)) {
      $organism_data1 = get_webapollo_organisms($web_apollo_organism, 1);
      $organism_data2 = get_webapollo_organisms($web_apollo_organism, 2);
    }
    if (searchkey($organism_data1, $values['organism_key'])) {
      $organism_data = $organism_data1;
      $type = 1;
    }
    else {
      $organism_data = $organism_data2;
      $type = 2;
    }

    $organism_nameconv = [
      'anogla',
      'athros',
      'cimlec',
    ]; //Hard coded the values for db_users_admin
    $organism_short_name = searchkey($organism_data, $values['organism_key']);
    $organism_full_name = $organism_data[$organism_short_name];
    $organism_full_name = explode(' ', $organism_full_name);
    $organism_full_name = implode('_', $organism_full_name);

    if (in_array($organism_short_name, $organism_nameconv)) {
      $db_name = $organism_short_name . "_users_admin";
    }
    else {
      $db_name = $organism_short_name . "_users";
    }

    $db_user = "admin_user";
    $db_pass = "admin_password";

    /****
     * The goal is to make the password same for all organism for same emailid.
     * First check for the email already registered for any organism with status=1, if yes then take the most recent password and update in webapollo_users and apollo tables.
     * IF not then check for email with status=2, then take most recent password and update in webapollo_uses and apollo tables.
     */
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


    $user = $values['email'];
    $name = explode(" ", $values['name'], 2);
    $fname = $name[0];
    $lname = $name[1];

    if (isset($check_email_status1_exists) && !empty($check_email_status1_exists)) {
      $pass = $check_email_status1_exists->pass;;
      db_update('webapollo_users')
        ->fields(['pass' => $pass])
        ->condition('uid', $uid, '=')
        ->execute();

    }
    elseif (isset($check_email_status2_exists) && !empty($check_email_status2_exists)) {
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

    $user_pass_values['organism'] = isset($values['organism_key']) ? $organism_data[$values['organism_key']] : '';
    $user_pass_values['user'] = $user;
    $user_pass_values['pass'] = $pass;

    // $add_user = "perl add_user.pl -D ".$db_name." -U ".$db_user." -P ".$db_pass." -u ".$user." -p ".$pass;
    //ON PRODUCTION uncomment below line and comment the very next line for host apollo.nal.usda.gov otherwise it takes localhost
    // $exec_user = exec("/usr/local/bin/python2.7 /app/web/drupal7/sites/all/modules/custom/web_apollo/adduser.py -dbuser ".$db_user." -dbname ".$db_name." -user ".$user." -pwd ".$pass." -host apollo-stage.nal.usda.gov");

    // $exec_user = exec("/usr/local/bin/python2.7 /usr/local/drupal7/sites/all/modules/custom/web_apollo/adduser2.py -dbuser " . $db_user . " -dbname " . $db_name . " -user " . $user . " -fname " . $fname . " -lname " . $lname . " -pwd " . $pass . " -group " . $organism_full_name ."_WRITE -host apollo.nal.usda.gov");
    // $exec_user = exec("/usr/local/bin/python2.7 /usr/local/drupal7/sites/all/modules/custom/web_apollo/adduser2.py -dbuser " . $db_user . " -dbname " . $db_name . " -user " . $user . " -fname " . $fname . " -lname " . $lname . " -pwd " . $pass . " -group " . $organism_full_name ."_USER -host apollo.nal.usda.gov");
    $exec_user = exec("/usr/local/bin/python2.7 /usr/local/drupal7/sites/all/modules/custom/web_apollo/adduser2.py -dbpass " . $db_pass . " -dbuser " . $db_user . " -user " . $user . " -fname " . $fname . " -lname " . $lname . " -pwd " . $pass . " -group " . $organism_full_name . "_WRITE -host apollo.nal.usda.gov");
    $exec_user = exec("/usr/local/bin/python2.7 /usr/local/drupal7/sites/all/modules/custom/web_apollo/adduser2.py -dbpass " . $db_pass . " -dbuser " . $db_user . " -user " . $user . " -fname " . $fname . " -lname " . $lname . " -pwd " . $pass . " -group " . $organism_full_name . "_USER -host apollo.nal.usda.gov");


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
      drupal_set_message(t('Successfully an email was sent to %email regarding the status approved.'), ['%email' => $values['email']], $type = 'status');
    }

    //Displaying status message on the site
    $status = ($status == 1) ? 'Approved' : 'Rejected';
    drupal_set_message(t("%name 's status was changed to %status"), [
      '%name' => $form_state['values']['web_apollo2_table']['name'],
      '%status' => $status,
    ], $type = 'status');
  } // Status approved condition ends here
  elseif ($status == 0) {
    db_update('webapollo_users')// Table name no longer needs {}
    ->fields(['status' => $status])
      ->condition('uid', $uid, '=')
      ->execute();
    drupal_set_message(t('%name has been rejected by  the admin due to some reasons.'), ['%name' => $values['name']], $type = 'status');
  }
  /**********************************************/

  $form_state['redirect'] = 'admin/structure/webapollo/users2';
  return;
}
