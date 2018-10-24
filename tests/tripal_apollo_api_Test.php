<?php

namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

require_once 'shared.php';

class tripal_apollo_api_Test extends TripalTestCase {

  // Uncomment to auto start and rollback db transactions per test method.
  use DBTransaction;

  /**
   * Ensure that the organism is linked to the entity and the
   * get_eligible_records api returns it.
   */
  public function test_tripal_apollo_get_eligible_records() {
    $url = 'http://127.0.0.1:8888';

    $info = tripal_apollo_configureApollo($url);
    $results = tripal_apollo_get_eligible_records();

    $this->assertNotEmpty($results);
    $this->assertArrayHasKey($info['organism'], $results);

  }

  public function test_tripal_apollo_create_user_permissions() {
    $this->add_user_with_permission();
    //TODO: write API call to fetch groups

    $this->assertTrue(TRUE);
  }


  /**
   * @group wip
   */
  public function test_tripal_apollo_get_users() {

    $url = 'http://127.0.0.1:8888';

    $this->add_user_with_permission();


    $instance = db_select('apollo_instance', 't')
      ->fields('t')
      ->condition('url', $url)
      ->execute()
      ->fetchObject();


    $users = tripal_apollo_get_users($instance->id);


    $this->assertNotEmpty($users);

    $target_user = NULL;

    //check returned users for our user.
    foreach ($users as $user) {
      if ($user->firstName == 'walrus') {
        $target_user = $user;
      }
    }

    $this->assertNotNull($target_user);
    $this->assertEquals('testaberger', $target_user->lastName);

    $user_info = db_select('apollo_user', 't')
      ->fields('t')
      ->condition('email', 'this_email_shouldnt_exist@never_gonna_happen.com')
      ->execute()
      ->fetchObject();

    $users = tripal_apollo_get_users($instance->id, $user_info->id);

    $this->assertNotEmpty($users);

    $user = array_shift($users);

    $this->assertEquals('testaberger', $user->lastName, "Error retrieving a specific user");


  }

//
//  /**
//   * Purge user is currently on hold.  See @ticket 58.
//   * @group wip
//   */
//  public function test_tripal_apollo_purge_user() {
//
//    $url = 'http://127.0.0.1:8888';
//
//    $info = $this->add_user_with_permission();
//
//    $auid = $info['apollo_user_id'];
//
//    $response = tripal_apollo_purge_user($auid);
//
//
//    var_dump($response);
//    $this->assertTrue($response, 'purge user API returned error');
//
//    $instance = db_select('apollo_instance', 't')
//      ->fields('t')
//      ->condition('url', $url)
//      ->execute()
//      ->fetchObject();
//
//    $users = tripal_apollo_get_users($instance->id);
//
//    $has_user = FALSE;
//
//    foreach ($users as $user) {
//      if ($user->firstName == 'walrus') {
//        $has_user = TRUE;
//      }
//    }
//
//    $this->assertFalse($has_user);
//
//  }

  private function add_user_with_permission() {

    $url = 'http://127.0.0.1:8888';

    $info = tripal_apollo_configureApollo($url);

    $instance_info = db_select('apollo_instance', 't')
      ->fields('t')
      ->condition('url', $url)
      ->execute()
      ->fetchObject();

    $user_info = [
      'uid' => NULL,
      'name' => "walrus testaberger",
      'pass' => 'some_unencrypted_password',
      'email' => 'this_email_shouldnt_exist@never_gonna_happen.com',
      'institution' => "U of Tripal",
    ];


    $user = db_insert('apollo_user')
      ->fields($user_info)
      ->execute();

    $aur = db_insert('apollo_user_record')
      ->fields([
        'record_id' => $info['organism'],
        'apollo_user_id' => $user,
        'status' => '2',
      ])
      ->execute();

    $user_info['apollo_user_id'] = $user;

    $user_info['organism_key'] = $info['organism'];

    $curr_dir = getcwd();


    chdir(DRUPAL_ROOT);

    tripal_apollo_create_user_permissions($user_info, $instance_info);

    chdir($curr_dir);

    return ['apollo_user_id' => $user];

  }
}
