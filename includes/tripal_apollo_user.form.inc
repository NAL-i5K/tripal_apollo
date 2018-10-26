<?php

/**
 * Implements hook_form().
 *
 * @param $auid : apollo user id.
 */
function tripal_apollo_user_form($form, &$form_state, $auid) {


  $auser = db_select('apollo_user', 'au')
    ->fields('au')
    ->execute()
    ->fetchObject();

  if ($auser->uid) {
    //load user id
  }

  $form['name'] = [
    "#type" => 'item',
    '#markup' => $auser->name,
    '#title' => "Name",
  ];

  $form['email'] = [
    "#type" => 'item',
    '#markup' => $auser->email,
    '#title' => "Email",
  ];


  $base = variable_get('tripal_apollo_base_table');
  $base_key = $base . '_id';


  $query = db_select('apollo_user_record', 'aur')
    ->condition('apollo_user_id', $auser->id);
  $query->fields('aur', ['status']);
  $query->join('chado.' . $base, 'ct', 'ct.' . $base_key . ' = aur.record_id');

  if ($base == 'organism') {
    $query->fields('ct', ['genus', 'species']);
  }
  $requests = $query->execute();
  //$requests = $query->fetchAll();

  $headers = [$base, 'status'];
  $rows = [];

  foreach ($requests as $request) {

    $status = $request->status;

    switch ($status) {
      case '0':
        $status_string = 'Pending';
        break;
      case '1':
        $status_string = 'Approved';
        break;
      case '2':
        $status_string = 'Denied';
        break;
      default:
        $status_string = 'Unknown';
        break;
    }
    $rows[] = [$request->genus . ' ' . $request->species, $status_string];

  }


  $table = theme('table', ['header' => $headers, 'rows' => $rows]);
  $form['request_table'] = [
    '#markup' => $table,
  ];


  $form['instructions'] = ['#markup' => '<p><b>Warning!</b>  Deleting this user will delete all their requests and all accounts across all instances.</p><p>Please note this will not delete the associated Drupal user.</p>

<p>For now the delete user functionality is under development and disabled.</p>'];



  $form['auid'] = [
    '#type' => 'value',
    '#value' => $auid
  ];


  $form['delete_user'] = [
    '#type' => 'submit',
    '#submit' => ['tripal_apollo_user_delete'],
    '#value' => t('Delete Apollo User'),
    '#disabled' => TRUE
  ];

  return $form;
}

function tripal_apollo_user_delete($form, &$form_state) {

  $auid = $form_state['values']['auid'];

  $result = tripal_apollo_purge_user($auid);

  //API CALLs: delete user in all attached instances.

  if (!$result) {
    tripal_set_message('Error: could not delete user from Apollo instances', TRIPAL_ERROR);
  }
  //Now: delete all apollo_user based records.
  db_delete('apollo_user')
  ->condition('id', $auid)
  ->execute();

  $form_state['redirect'] = variable_get('site_frontpage');

}

