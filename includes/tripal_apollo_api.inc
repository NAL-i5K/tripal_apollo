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
  $name = explode(" ", $values['name'], 2);
  $fname = $name[0];
  $lname = isset($name[1]) ? $name[1] : $name[0];

  //For now we assume the group base is genus_species
  //see @ticket 41
  $record = $values['organism_key'];
  $base = variable_get('tripal_apollo_base_table');

  if ($base == 'organism') {
    $organism = chado_select_record('organism', [
      'genus',
      'species',
    ], ['organism_id' => $record]);
    $organism = $organism[0];
    $group_base = $organism->genus . '_' . $organism->species;
  }
  else {//todo: how would one want analysis or project supported?
    $object = chado_select_record($base, ['name'], [$base . '_id' => $record]);
    $object = $object[0];
    $group_base = $object->name;
  }

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
      $user = array_shift($user);
    }


    //add user to group 1

    //create the groups in case they dont exist

    //TODO:  it looks like this is a bad idea.  do it in tests instead?

    tripal_apollo_create_group($instance_info->id, $group_base . '_USER');
    tripal_apollo_create_group($instance_info->id, $group_base . '_WRITE');

    tripal_apollo_add_user_to_group($instance_info, $apollo_user_id, $group_base . '_USER');


    tripal_apollo_add_user_to_group($instance_info, $apollo_user_id, $group_base . '_WRITE');

  }
  //
  //
  //    $script_path .= 'add_user2.py';
  //
  //    $command_base = $python_path . " " . $script_path .
  //      " -dbuser " . $admin_user .
  //      " -dbpass " . $admin_pass .
  //      " -user " . $user .
  //      " -pwd " . $user_pass .
  //      " -host " . $host .
  //      " -fname " . $fname .
  //      " -lname " . $lname;
  //
  //
  //    //TODO: Make this configurable?
  //    $exec_user = exec($command_base .
  //      " -group " . $group_base . "_USER");
  //    $exec_user = exec($command_base .
  //      " -group " . $group_base . "_WRITE");
  //    //Parameters not supplied:
  //    //metadata, token


  return TRUE;
}


function tripal_apollo_add_user($instance_id, $apollo_user_id, $role = 'USER') {


  $user = db_select('apollo_user', 't')
    ->fields('t')
    ->condition('id', $apollo_user_id)
    ->execute()
    ->fetchObject();

  $name = explode(" ", $user->name, 2);
  $fname = $name[0];
  $lname = isset($name[1]) ? $name[1] : $name[0];

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

  return array_shift($response);

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
      ->fetchObject();
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
    var_dump($options);
    tripal_set_message(t('Connection error for Apollo instance: !name', ['!name' => $instance->name]), TRIPAL_ERROR);
    return FALSE;
  }

  //remove admin user

  $data = json_decode($response->data);

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
 * @param $auid apollo user id
 */
function tripal_apollo_purge_user($auid) {

  $query = db_select('apollo_user_record', 'aur');
  $query->condition('aur.apollo_user_id', $auid);
  $query->join('apollo_user', 'au', 'aur.apollo_user_id = au.id');
  $query->fields('au', ['email']);
  $query->join('apollo_instance_record', 'air', 'air.record_id = aur.record_id');
  $query->fields('air', ['instance_id']);
  $results = $query->execute();


  $path = '/user/deleteUser';

  foreach ($results as $result) {

    $instance = tripal_apollo_get_instance($result->instance_id);
    $password = tripal_apollo_decrypt($instance->admin_password);
    $address = $instance->url . $path;

    $headers = ['Content-Type' => 'application/json'];
    $method = 'POST';
    $data = ['username' => $instance->admin_name, 'password' => $password];
    $data['userToDelete'] = $result->email;

    $options = [
      'headers' => $headers,
      'method' => $method,
      'data' => json_encode($data),
    ];

    $response = drupal_http_request($address, $options);

    //error check here
    if ($response->code != "200") {
      tripal_log(t('Error: Bad response from Apollo instance !id, code: !code.  Message: !message', [
        '!id' => $instance->id,
        '!code' => $response->code,
        '!message' => $response->error,
      ]), TRIPAL_ERROR);
      return FALSE;
    }

    $data = json_decode($response->data);

    if (isset($data->error)) {
      tripal_log(t('Error: Bad response from Apollo instance !id, code: !code.  Message: !message', [
        '!id' => $instance->id,
        '!code' => $response->code,
        '!message' => $data->error,
      ]), TRIPAL_ERROR);
      return FALSE;

    }
  }
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