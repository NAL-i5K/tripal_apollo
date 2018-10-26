<?php


function tripal_apollo_migrate_webapollo() {

  //migrate webapollo_users to new schema

  $wa_users = db_select('public.webapollo_users', 't')
    ->fields('t')
    ->distinct()
    ->select()
    ->execute();

  foreach ($wa_users as $wa_user){



    //Do not confuse this with webapollo_users.uid they are not the same.
    $uid = db_select('public.users', 't')
      ->addField('t', 'uid')
      ->condition('mail', $email)
      ->execute()
      ->fetchField();

    $credentials = db_select('public.webapollo_users', 't')
      ->fields('t')
      ->condition('email', $email)
      ->select()
      ->execute()
      ->fetchAll();

    $apollo_credentials = [];

    foreach ($credentials as $credential){
      $name = $credential->name;
      $pass = $credential->pass;
      $organism = $credential->organisms;
      $institution = $credential->institution;
      $comment = $credential->comment;
      $created = $credential->created;
      $status = $credential->status;

      $id  = db_select('public.apollo_user', 't')
        ->fields('id')
        ->condition('name', $name)
        ->condition('pass', $pass)
        ->condition('institution', $institution)
        ->condition('email', $email)
        ->execute()
        ->fetchField();

      if (!$id){
        $id = db_insert('public.apollo_user', 't')
        ->fields([
          'uid' => $uid,
          'name' => $name,
          'pass' => $pass,
          'email' => $email,
          'institution' => $institution,
          'comment' => $comment,
          'created' => $created
        ])
        ->execute();
      }

      //TODO: I need to use the web_apollo organism api to figure out which organism this is in chado unfortunately. Also chado_get_organism is tripal_get_organism in tripal 2 so if compatibility is an issue, use chado_get_record instead.


      $organism_id = chado_get_organism(['organism_id' => "shoot i dont know, i have a string"]);
      //Add this organism instance
      db_insert('public.apollo_user_organism_instance')
        ->fields([
          'organism_id' ,
          'instance_id' => 1,
          'apollo_user_id' =>$id
        ])
        ->execute();


    }

  }
  //TODO unfinished because of above issue.
  //Drop webapollo_users
}