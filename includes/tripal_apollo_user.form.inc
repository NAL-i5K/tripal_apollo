<?php

/**
 * Implements hook_form().
 *
 * @param $auid: apollo user id.

 */
function tripal_apollo_user_form($form, &$form_state, $auid) {


  $auser = db_select('apollo_user', 'au')
    ->fields('au')
    ->execute()
    ->fetchObject();

  if ($auser->uid){
    //load user id
  }

  $form['user_info'] = [
    '#markup' => 'user info here.'
  ];


  $form['delete_user'] = [
    '#type' => 'submit',
    '#submit' => ['tripal_apollo_user_delete'],
    '#value' => t('Delete Apollo User'),
    '#description' => 'Please note this will not delete the associated Drupal user.'
  ];

  return $form;
}

function tripal_apollo_user_delete($form, &$form_state){

  //API CALLs: delete user in all attached instances.

  //Now: delete all apollo_user based records.

}

