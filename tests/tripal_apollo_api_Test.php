<?php
namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;
require_once 'shared.php';

class tripal_apollo_api_Test extends TripalTestCase {
  // Uncomment to auto start and rollback db transactions per test method.
   use DBTransaction;

  /**
   * Ensure that the organism is linked to the entity and the get_eligible_records api returns it.
   */
  public function test_tripal_apollo_get_eligible_records(){
    $url = 'localhost:8888';

    $info = tripal_apollo_configureApollo($url);
    $results = tripal_apollo_get_eligible_records();

    $this->assertNotEmpty($results);
    $this->assertArrayHasKey($info['organism'], $results);

  }

  public function test_tripal_apollo_create_user_permissions(){
    $url = 'localhost:8888';

    $info = tripal_apollo_configureApollo($url);

   $instance_info =  db_select('apollo_instance', 't')
     ->fields('t')
     ->condition('url', $url)
     ->execute()
     ->fetchObject();

$user_info = ['uid' => NULL,
  'name' => "walrus testaberger",
  'pass' => 'some_unencrypted_password',
  'email' => 'this_email_shouldnt_exist@never_gonna_happen.com',
  'institution' => "U of Tripal"];

   db_insert('apollo_user')
     ->fields($user_info);

$user_info['organism_key'] = $info['organism'];
   //email
    //name
    //organism_key
    //pass


     tripal_apollo_create_user_permissions($user_info, $instance_info);

     //TODO: write API call to fetch groups

     $this->assertTrue(TRUE);
    }
}
