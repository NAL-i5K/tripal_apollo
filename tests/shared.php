<?php


 function tripal_apollo_configureApollo($url) {

  //Ffirst delete an existing instance since the url is unique.
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

  $organism_id = db_select('chado.organism', 't')
    ->fields('t', ['organism_id'])
    ->condition('genus', 'saccharomyces')
    ->condition('species', 'cerevisiae')
    ->execute()
    ->fetchField();

  if (!$organism_id){
    $organism = factory('chado.organism')->create([
      'genus' => 'saccharomyces',
      'species' => 'cerevisiae',
      'abbreviation' => 's. cerevisiae',
      'common_name' => 'yeast'
    ]);
    $organism_id = $organism->organism_id;
  }

  $values = [
    'title' => 'title',
    'instance_name' => 'tripal_apollo_test_instance',
    'url' => $url,
    'apollo_version' => 2,
    'database_name' => 'chado',
    'admin_name' => 'admin@local.host',
    'admin_password' => 'password',
    'records' => [$organism_id]
  ];
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

   return ['organism' => $organism_id];
}
