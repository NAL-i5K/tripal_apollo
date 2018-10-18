<?php
namespace Tests;

use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;
require_once 'shared.php';

class tripal_apollo_api_Test extends TripalTestCase {
  // Uncomment to auto start and rollback db transactions per test method.
   use DBTransaction;

  public function test_tripal_apollo_get_elligible_records(){
    $url = 'localhost:8888';

    tripal_apollo_configureApollo($url);

    $results = tripal_apollo_get_elligible_records();

    $this->assertNotEmpty($results);


  }

}
