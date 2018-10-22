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
    $host =  tripal_apollo_one_build_url($host, $record, $instance_info->id);

    $exec_user = exec($python_path . " " . $script_path .
      " -dbuser " . $admin_user .
      " -dbname " . $db_name .
      "-dbpass " . $admin_pass .
      " -user " . $user .
      " -pwd " . $user_pass .
      " -host " . $host);
  }
  else {

    $script_path .= 'add_user2.py';

    $command_base = $python_path . " " . $script_path .
      " -dbuser " . $admin_user .
      " -dbpass " . $admin_pass .
      " -user " . $user .
      " -pwd " . $user_pass .
      " -host " . $host .
      " -fname " . $fname .
      " -lname " . $lname;


    //TODO: Make this configurable?
    $exec_user = exec($command_base .
      " -group " . $group_base . "_USER");
    $exec_user = exec($command_base .
      " -group " . $group_base . "_WRITE");
    //Parameters not supplied:
    //metadata, token
  }


  return TRUE;
}

/**
 * Given an instance_id, return all users for that instance as an array of objects using the loadUsers API.
 * Apollo 2 only.
 * @param $instance_id
 * @param null $user_id
 *
 * @return array|bool|mixed
 */
function tripal_apollo_get_users($instance_id, $user_id = NULL){

  $instance = tripal_apollo_get_instance($instance_id);
  $path = '/user/loadUsers';
  $address = $instance->url . $path;
  $password = tripal_apollo_decrypt($instance->admin_password);
  $method = 'POST';
  $data = ['username' => $instance->admin_name, 'password' => $password];
  if ($user_id){
    //TODO: convert to apollo user id.
    $data['userId']  = $user_id;
  }

  $headers = ['Content-Type' => 'application/json'];

  $options = ['headers' => $headers,
    'method' => $method,
    'data' => json_encode($data)];

  $response = drupal_http_request($address, $options);


  //TODO: error check

  if (!isset($response->data)){
    return FALSE;
  }

  $data = $response->data;

  $data = json_decode($data);

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
 * builds the apollo 1 URL.  If its a multi-instance, we assume its url/genspec where genspec is first three letters of the genus and species.
 * @param $url
 * @param $record_id
 * @param $instance_id
 *
 * @return mixed
 */
function tripal_apollo_one_build_url($url, $record_id, $instance_id){

  $records = db_select('apollo_instance_record', 't')
    ->fields('t')
    ->condition('instance_id', $instance_id)
    ->execute()
    ->fetchAll();

  if (count($records) < 2){
    return $url;
  }

  $organism = chado_select_record('organism', ['genus', 'species'], ['organism_id' => $record_id]);

  $organism = $organism[0];

  $url .= '/' . substr($organism->genus, 0, 2) . substr($organism-species, 0, 2);

  return $url;

}