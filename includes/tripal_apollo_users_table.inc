<?php


function tripal_apollo_users_table($form, &$form_state) {

  $users = db_select('apollo_user', 'au')
    ->fields('au')
    ->execute();

  //TODO: Paginate instead.

  $rows = [];
  $headers = ['User name', 'email', 'Access'];

  foreach ($users as $user) {

    $access = db_select('apollo_user_record', 'aur')
      ->fields('aur')
      ->execute()
      ->fetchAll();

    $id = $user->id;
    $uid = $user->uid;
    $name = $user->name;
    $email = $user->email;
    $institution = $user->institution;


  }

  return $form;


}