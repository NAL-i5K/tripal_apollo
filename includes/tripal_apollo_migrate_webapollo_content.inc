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

      $code = strtolower(substr($organism->genus, 0, 3)) . substr($organism->species, 0, 3);

      if (isset($organism_lookup[$code])){
        print("$code is already in use!\n");
        return;
      }
      if ($code == "censcu"){
        $organism_lookup['cenexi'] = $organism->organism_id;
      }
      $organism_lookup[$code] = $organism->organism_id;
    }

  $wa_users = db_select('public.webapollo_users', 't')
    ->fields('t')
    ->distinct()
    ->execute();

    $count = 0;

  foreach ($wa_users as $wa_user) {
    $count++;

    if ($count % 100 == 0){
      print ("converting record " . $count);
    }

    $name = $wa_user->name;
    $pass = $wa_user->pass;
    $organism_code = $wa_user->organisms;
    $institution = $wa_user->institution;
    $comment = $wa_user->comment;
    $created = $wa_user->created;
    $status = $wa_user->status;
    $email = $wa_user->email;

    $id = db_select('public.apollo_user', 't')
      ->fields('t', ['id'])
      //   ->condition('name', $name)
      //   ->condition('pass', tripal_apollo_encrypt($pass))
      //   ->condition('institution', $institution)
      ->condition('email', $email)
      ->execute()
      ->fetchField();

    if (!$id) {
      $id = db_insert('public.apollo_user')
        ->fields([
          'uid' => NULL,
          'name' => $name,
          'pass' => tripal_apollo_encrypt($pass),
          'email' => $email,
          'institution' => $institution,
          'comment' => $comment,
        //  'created' => $created,
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
