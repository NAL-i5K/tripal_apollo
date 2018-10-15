<?php

namespace Tests;

use phpDocumentor\Reflection\Types\Object_;
use StatonLab\TripalTestSuite\DBTransaction;
use StatonLab\TripalTestSuite\TripalTestCase;

class apollo_instance_nodeTest extends TripalTestCase {

  // Uncomment to auto start and rollback db transactions per test method.
  use DBTransaction;

  public function test_instance_node_create() {

    $url = 'localhost:8888';

    $this->configureApollo($url);
    //run it again make sure it deletes existing records

    $result = db_select('apollo_instance', 't')
      ->fields('t', ['id'])
      ->condition('url', $url)
      ->execute()
      ->fetchField();

    $this->assertNotFalse($result);

    $this->configureApollo($url);

    $result = db_select('apollo_instance', 't')
      ->fields('t', ['id'])
      ->condition('url', $url)
      ->execute()
      ->fetchField();

    $this->assertNotFalse($result);

  }

  private function configureApollo($url) {

    //First delete an existing instance since the url is unique.
    $check = db_select('apollo_instance', 't')
      ->fields('t', ['id'])
      ->condition('url', $url)
      ->execute()
      ->fetchField();

    if ($check) {
      db_delete('apollo_instance')
        ->condition('id', $check)
        ->execute();
    }

    $values = [
      'title' => 'title',
      'instance_name' => 'tripal_apollo_test_instance',
      'url' => $url,
      'apollo_version' => 2,
      'database_name' => 'chado',
      'admin_name' => 'admin@local.host',
      'admin_password' => 'password',
    ];
    //    $info = entity_save('node', $values);

    $node = new \stdClass();

    $node->type = 'apollo_instance';
    node_object_prepare($node);
    $node->status = 1;
    $node->language = LANGUAGE_NONE;

    foreach ($values as $key => $val) {
      $node->$key = $val;
    }
    $node = node_submit($node);
    node_save($node);
  }
}
