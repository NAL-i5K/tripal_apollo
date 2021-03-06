<?php


/**
 *  Generate Random password contains alphabets and numbers
 */
function tripal_apollo_gen_xkcd_password($num_of_words = 1) {
  $dict_lines = file('/usr/share/dict/words', FILE_IGNORE_NEW_LINES);
  $length = count($dict_lines);
  $password = '';
  $rand_num = rand(1, 9);
  for ($i = 1; $i <= $num_of_words; $i++) {
    $plain = FALSE;

    while (!$plain) {
      // Get random word from $lines
      $key = mt_rand(0, $length);
      if ((preg_match("/^[a-z]+$/", $dict_lines[$key]) == 1) && (strlen($dict_lines[$key]) < 9)) {
        // String only contains a to z characters
        $plain = TRUE;
        $password = $password . ucwords($dict_lines[$key]);
        if ($i != $num_of_words) {
          $password = $password . $rand_num;
        }
      }
    }
  }
  return $password;
}

/**
 * Fetches all records associated with an Apollo instance.
 */
function tripal_apollo_get_eligible_records() {

  $base = variable_get('tripal_apollo_base_table');
  $base_key = $base . '_id';

  $records = db_select('apollo_instance_record', 'tair')
    ->fields('tair', ['record_id'])
    ->execute();

  $out = [];

  foreach ($records as $record) {
    $record_id = $record->record_id;
    switch ($base) {
      case 'organism':

        $result = chado_select_record($base, [
          'genus',
          'species',
        ], [$base_key => $record_id]);

        if (!$result) {
          //todo:  should we instead delete the record?
          continue;
        }

        $result = $result[0];

        $out[$record_id] = $result->genus . ' ' . $result->species;
        break;
      /**
       * By default we can just use name.
       */
      case 'default':
        $result = chado_select_record($base, ['name'], [$base_key => $record_id]);
        $result = $result[0];
        $out[$record_id] = $result->name;
        break;
    }
  }

  return $out;
}

/**
 * Helper function for using the encrypt module.  Since installing some of the
 * prereq libraries can be a pain (particularly on OSX) we allow disabling it
 * via variable.
 *
 * @param $string
 *
 * @return : encrypted string, or unencrypted string
 */
function tripal_apollo_encrypt($string) {
  $encrypt = variable_get('tripal_apollo_encrypt');

  if ($encrypt) {
    $string = encrypt($string);
  }
  return $string;
}

/**
 * @see tripal_apollo_encrypt
 */
function tripal_apollo_decrypt($string) {
  $encrypt = variable_get('tripal_apollo_encrypt');

  if ($encrypt) {
    $string = decrypt($string);
  }
  return $string;
}

/**
 * @param $values | Form values from tripal_apollo_approve_user_request_form.
 *   Contains the information about the user and the record.
 * @param $instance_info | An apollo_instance record.
 *
 * @return bool | TRUE if successful, FALSE otherwise
 */
function tripal_apollo_create_user_permissions($values, $instance_info) {

  //Note that for apollo 1 this is the db credentials.  For apollo 2 its the site admin credentials.
  $admin_pass = tripal_apollo_decrypt($instance_info->admin_password);
  $admin_user = $instance_info->admin_name;
  $host = $instance_info->url;

  $apollo_user_id = $values['apollo_user_id'];

  $user = $values['email'];
  $names = explode(" ", $values['name']);

  $fname = $names[0];
  unset($names[0]);
  $lname = implode(" ", $names);
  $lname = isset($lname) ? $lname : $fname;

  //For now we assume the group base is genus_species
  //see @ticket 41
  $record = $values['organism_key'];

  $group_base = tripal_apollo_build_group_string_base($record);

  $user_pass = tripal_apollo_decrypt($values['pass']);


  $python_path = variable_get('tripal_apollo_python_path');
  $script_path = drupal_get_path('module', 'tripal_apollo') . '/bin/';

  if ($instance_info->apollo_version === 1) {
    //Apollo 1 registration

    $script_path .= 'adduser.py';

    $db_name = $instance_info->database_name;
    $host = tripal_apollo_one_build_url($host, $record, $instance_info->id);

    $exec_user = exec($python_path . " " . $script_path .
      " -dbuser " . $admin_user .
      " -dbname " . $db_name .
      "-dbpass " . $admin_pass .
      " -user " . $user .
      " -pwd " . $user_pass .
      " -host " . $host);
  }
  else {

    //create/get user


    $user = tripal_apollo_get_users($instance_info->id, $apollo_user_id);

    if (!$user) {
      $user = tripal_apollo_add_user($instance_info->id, $apollo_user_id);
    }

    //add user to group 1

    //create the groups in case they dont exist

    //TODO:  it looks like this is a bad idea.  do it in tests instead?
    //
    //    tripal_apollo_create_group($instance_info->id, $group_base . '_USER');
    //    tripal_apollo_create_group($instance_info->id, $group_base . '_WRITE');

    tripal_apollo_add_user_to_group($instance_info, $apollo_user_id, $group_base . '_USER');
    tripal_apollo_add_user_to_group($instance_info, $apollo_user_id, $group_base . '_WRITE');

  }
  return TRUE;
}


/**
 * Build the group base string.  General assumption is for genus_species of
 * organism.
 *
 * @ticket 41
 *
 * @param $record_id
 *
 * @return string
 */
function tripal_apollo_build_group_string_base($record_id) {
  //For now we assume the group base is genus_species
  //see @ticket 41
  // $record_id = $values['organism_key'];
  $base = variable_get('tripal_apollo_base_table');

  if ($base == 'organism') {
    $organism = chado_select_record('organism', [
      'genus',
      'species',
    ], ['organism_id' => $record_id]);

    if (empty($organism)) {
      tripal_set_message(t("Unable to find organism for !record", ['!record' => $record_id]), TRIPAL_ERROR);
      return FALSE;
    }
    $organism = $organism[0];
    $group_base = $organism->genus . '_' . $organism->species;
  }
  else {//todo: how would one want analysis or project supported?
    $object = chado_select_record($base, ['name'], [$base . '_id' => $record_id]);
    $object = $object[0];
    $group_base = $object->name;
  }

  return $group_base;
}


function tripal_apollo_add_user($instance_id, $apollo_user_id, $role = 'USER') {


  $user = db_select('apollo_user', 't')
    ->fields('t')
    ->condition('id', $apollo_user_id)
    ->execute()
    ->fetchObject();

  $names = explode(" ", $user->name);

  $fname = $names[0];
  unset($names[0]);
  $lname = implode(" ", $names);
  $lname = isset($lname) ? $lname : $fname;
  
  $instance_info = tripal_apollo_get_instance($instance_id);

  $path = '/user/createUser';
  $address = $instance_info->url . $path;
  $password_admin = tripal_apollo_decrypt($instance_info->admin_password);
  $password_user = tripal_apollo_decrypt($user->pass);
  $method = 'POST';
  $data = [
    'username' => $instance_info->admin_name,
    'password' => $password_admin,
    'firstName' => $fname,
    'lastName' => $lname,
    'role' => $role,
    'email' => $user->email,
    'newPassword' => $password_user,
  ];

  $headers = ['Content-Type' => 'application/json'];

  $options = [
    'headers' => $headers,
    'method' => $method,
    'data' => json_encode($data),
  ];


  $response = drupal_http_request($address, $options);
  return $response;

}

/**
 * Given an instance_id, return all users for that instance as an array of
 * objects using the loadUsers API. Apollo 2 only.
 *
 * @param $instance_id
 * @param null $user_id .  If a user_id is given, return that user only.
 * this is the apollo_user.id field.
 *
 * @return array|bool|mixed
 */
function tripal_apollo_get_users($instance_id, $user_id = NULL) {

  $instance = tripal_apollo_get_instance($instance_id);
  $path = '/user/loadUsers';
  $address = $instance->url . $path;
  $password = tripal_apollo_decrypt($instance->admin_password);
  $method = 'POST';
  $data = ['username' => $instance->admin_name, 'password' => $password];
  if ($user_id) {
    //TODO: convert to apollo user id.

    $user_mail = db_select('apollo_user', 't')->fields('t', ['email'])
      ->condition('id', $user_id)->execute()
      ->fetchField();
    if (!$user_mail) {
      tripal_set_message('User ID not found', TRIPAL_ERROR);
      return FALSE;
    }
    $data['userId'] = $user_mail;
  }

  $headers = ['Content-Type' => 'application/json'];

  $options = [
    'headers' => $headers,
    'method' => $method,
    'data' => json_encode($data),
  ];

  $response = drupal_http_request($address, $options);

  if (!isset($response->data)) {
    tripal_set_message(t('Connection error for Apollo instance: !name', ['!name' => $instance->name]), TRIPAL_ERROR);
    return FALSE;
  }

  $data = json_decode($response->data);

  if (!$data) {
    return FALSE;
  }

  foreach ($data as $user_number => $user) {
    if ($user->role === 'ADMIN') {
      unset($data[$user_number]);
    }
  }
  return $data;
}

/**
 * fetch an apollo object with the instance id
 *
 * @param $instance_id
 *
 * @return mixed | apollo instance object, or FALSE
 */
function tripal_apollo_get_instance($instance_id) {

  $result = db_select('apollo_instance', 't')
    ->fields('t')
    ->condition('id', $instance_id)
    ->execute()
    ->fetchObject();

  return $result;
}


//
//function tripal_apollo_get_instance_groups($instance_id) {
//
//  $base = tripal_apollo_build_base_request($instance_id);
//  $instance = tripal_apollo_get_instance($instance_id);
//  $url = $instance->url . "/groups/loadGroups";
//  $base . " {$url}";
//
//}
//
//function tripal_apollo_build_base_request($instance_id) {
//  $instance = tripal_apollo_get_instance($instance_id);
//  $base_curl = "curl -i -s -X POST -H 'Content-Type: application/json' -d";
//
//  return $base_curl;
//}


/**
 * builds the apollo 1 URL.  If its a multi-instance, we assume its url/genspec
 * where genspec is first three letters of the genus and species.
 *
 * @param $url
 * @param $record_id
 * @param $instance_id
 *
 * @return mixed
 */
function tripal_apollo_one_build_url($url, $record_id, $instance_id) {

  $records = db_select('apollo_instance_record', 't')
    ->fields('t')
    ->condition('instance_id', $instance_id)
    ->execute()
    ->fetchAll();

  if (count($records) < 2) {
    return $url;
  }

  $organism = chado_select_record('organism', [
    'genus',
    'species',
  ], ['organism_id' => $record_id]);

  $organism = $organism[0];

  $url .= '/' . substr($organism->genus, 0, 2) . substr($organism - species, 0, 2);

  return $url;

}

/**
 * Removes a user from all groups.
 *
 * @param $auid apollo user id
 */
function tripal_apollo_purge_user($auid) {

  $query = db_select('apollo_user_record', 'aur');
  $query->condition('aur.apollo_user_id', $auid);
  $query->fields('aur', ['id']);
  $query->join('apollo_user', 'au', 'aur.apollo_user_id = au.id');
  $query->fields('au', ['email']);
  $query->join('apollo_instance_record', 'air', 'air.record_id = aur.record_id');
  $query->fields('air', ['instance_id']);
  $results = $query->execute();

  //Track what was successful so we can update the status for those if we fail along the way.
  $successes = [];

  foreach ($results as $result) {

    $success = tripal_apollo_rescind_user_permissions($result->id, $result->instance_id);

    if (!$success) {

      tripal_set_message(t('Unable to rescind permissions for request !id', ['!id' => $result->id]), TRIPAL_ERROR);

      //update any succesful removals: if no successes just exit.
      if (empty($successes)){
        return FALSE;
      }
      db_update('apollo_user_record')
        ->fields(['status' => 0])
        ->condition('apollo_user_id', $auid)
        ->condition('id', $successes, "IN")
        ->execute();

      return FALSE;
    }

    $successes[] = $result->id;
  }

  //no failures, so update all for this user to rejected.
  db_update('apollo_user_record')
    ->fields(['status' => 0])
    ->condition('apollo_user_id', $auid)
    ->execute();

  return TRUE;
}

function tripal_apollo_add_user_to_group($instance_info, $apollo_user_id, $group_name) {

  $user = db_select('apollo_user', 't')
    ->fields('t')
    ->condition('id', $apollo_user_id)
    ->execute()
    ->fetchObject();

  $password = tripal_apollo_decrypt($instance_info->admin_password);
  $address = $instance_info->url . '/user/addUserToGroup';

  $headers = ['Content-Type' => 'application/json'];
  $method = 'POST';

  $data = [
    'username' => $instance_info->admin_name,
    'password' => $password,
    'group' => $group_name,
    'user' => $user->email,
  ];


  $options = [
    'headers' => $headers,
    'method' => $method,
    'data' => json_encode($data),
  ];

  $response = drupal_http_request($address, $options);


  if ($response->code != "200") {


    tripal_log(t('Error: Bad response adding User to Group: !group. code: !code.  Message: !message', [
      '!group' => $group_name,
      '!code' => $response->code,
      '!message' => $response->error,
    ]), TRIPAL_ERROR);
    return FALSE;
  }

  return TRUE;

}


/**
 * Creates a group in the apollo instance.
 *
 * @param $instance_id
 * @param $group_name
 *
 * @return mixed
 */
function tripal_apollo_create_group($instance_id, $group_name) {

  $instance = tripal_apollo_get_instance($instance_id);

  $address = $instance->url . '/group/createGroup';
  $password = tripal_apollo_decrypt($instance->admin_password);

  $headers = ['Content-Type' => 'application/json'];
  $method = 'POST';

  $data = [
    'username' => $instance->admin_name,
    'password' => $password,
    'name' => $group_name,
  ];


  $options = [
    'headers' => $headers,
    'method' => $method,
    'data' => json_encode($data),
  ];

  $response = drupal_http_request($address, $options);

  if ($response->code != '200') {
    tripal_log('Error creating group');
  }

  return $response;
}

function tripal_apollo_convert_status_for_display($num_status) {

  switch ($num_status) {
    case 0:
      $val = 'Rejected';
      break;
    case 1:
      $val = 'Approved';
      break;
    case 2:
      $val = 'Pending';
      break;
    default:
      $val = 'Unknown';
      break;
  }

  return $val;
}


function tripal_apollo_user_existing_requests_table($au, $header) {


  $base = variable_get('tripal_apollo_base_table');

  $pkey = $base . '_id';


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

}

function tripal_apollo_notify_admins_of_request($type, $record_id, $form_values) {

  $admin_email = variable_get('site_mail');;
  if (function_exists("tripal_hq_permissions_get_admins")) {

    $admins = tripal_hq_permissions_get_admins($record_id, 'organism');

    foreach ($admins as $mail) {
      $admin_email .= ', ' . $mail;
    }
  }

  $from = $form_values['name'] . "<" . $form_values['mail'] . ">";
  $sent = drupal_mail('tripal_apollo', $type, $admin_email, language_default(), $form_values, $from, TRUE);

  return $sent;
}


/**
 * Retroactively remove permissions from a user for a specific submission
 * request.
 *
 * @param $submission_id | Apollo user record submission ID
 * @param $instance_id | Instance ID.
 *
 * //TODO:  no reason instance id shouldnt be nullable
 *
 * @return bool
 */
function tripal_apollo_rescind_user_permissions($submission_id, $instance_id) {

  //get group info for this submission

  $query = db_select('apollo_user_record', 'aur');
  $query->fields('aur', ['record_id']);
  $query->join('apollo_user', 'au', 'au.id = aur.apollo_user_id');
  $query->addField('au', 'email');
  $query->condition('aur.id', $submission_id);

  $results = $query->execute()->fetchObject();


  $user_mail = $results->email;
  $record_id = $results->record_id;

  $base = tripal_apollo_build_group_string_base($record_id);
  //get instance

  $groups = [$base . '_USER', $base . '_WRITE'];
  $instance = tripal_apollo_get_instance($instance_id);


  if ($instance->apollo_version == 1) {
    tripal_set_message(t("Cannot change group permissions for Apollo 1 instance: !name", ['!name' => $instance->name]), TRIPAL_WARNING);
    return FALSE;
  }

  $address = $instance->url . '/user/removeUserFromGroup';
  $password = tripal_apollo_decrypt($instance->admin_password);

  $headers = ['Content-Type' => 'application/json'];
  $method = 'POST';

  foreach ($groups as $group) {

    $data = [
      'username' => $instance->admin_name,
      'password' => $password,
      'group' => $group,
      'user' => $user_mail,
    ];

    $options = [
      'headers' => $headers,
      'method' => $method,
      'data' => json_encode($data),
    ];

    $response = drupal_http_request($address, $options);

    if ($response->code != '200') {
      tripal_log('Error creating group', TRIPAL_ERROR);
      return FALSE;
    }

  }
  return TRUE;

}