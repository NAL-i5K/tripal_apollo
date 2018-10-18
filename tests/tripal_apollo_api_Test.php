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

}
