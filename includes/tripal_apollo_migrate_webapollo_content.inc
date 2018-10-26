<?php


function tripal_apollo_migrate_webapollo() {

  //migrate webapollo_users to new schema

  //first build organism lookup.


  $organisms = db_select('chado.organism', 't')
    ->fields('t')
    ->execute()
    ->fetchAll();
  $organism_lookup = [];

  foreach ($organisms as $organism) {

    /**
     * Counting on the organism in webapollo being first 3 letters of genus first 3 letters of species.
     */

    $code = substr($organism->genus, 0, 2) . substr($organism->species, 0, 2);
    $organism_lookup[$code] = $organism->organism_id;
  }

  $wa_users = db_select('public.webapollo_users', 't')
    ->fields('t')
    ->distinct()
    ->select()
    ->execute();

  foreach ($wa_users as $wa_user) {

    $name = $wa_user->name;
    $pass = $wa_user->pass;
    $organism_code = $wa_user->organisms;
    $institution = $wa_user->institution;
    $comment = $wa_user->comment;
    $created = $wa_user->created;
    $status = $wa_user->status;
    $email = $wa_user->email;

    $id = db_select('public.apollo_user', 't')
      ->fields('id')
      //   ->condition('name', $name)
      //   ->condition('pass', tripal_apollo_encrypt($pass))
      //   ->condition('institution', $institution)
      ->condition('email', $email)
      ->execute()
      ->fetchField();

    if (!$id) {
      $id = db_insert('public.apollo_user', 't')
        ->fields([
          'uid' => NULL,
          'name' => $name,
          'pass' => tripal_apollo_encrypt($pass),
          'email' => $email,
          'institution' => $institution,
          'comment' => $comment,
          'created' => $created,
        ])
        ->execute();
    }

    if (!isset($organism_lookup[$organism_code])) {
      tripal_log(t("Unable to find chado organism for !organism.  Skipping this connection.", ['!organism' => $organism_code]), TRIPAL_ERROR);
      continue;
    }

    $organism_id = $organism_lookup[$organism_code];

    //add to apollo user record
    db_insert('public.apollo_user_record')
      ->fields([
        'record_id' => $organism_id,
        'apollo_user_id' => $id,
        'status' => $status,
        'created' => $created,
      ])
      ->execute();
  }
}
