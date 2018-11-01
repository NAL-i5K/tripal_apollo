<?php

/**
 * Builds the user request table for admin to approve/reject.
 * Implements hook_form().
 */
function tripal_apollo_requests_form($form, &$form_state, $type = NULL) {

  $user_info = tripal_apollo_user_table($type);
  $form['users'] = ['#markup' => $user_info];
  return $form;
}


/**
 * List the web apollo registered users with pagination and
 * with Approve/Reject options
 **/
function tripal_apollo_user_table($type) {

  $type_convert = ["approved" => 1, "rejected" => 0, "pending" => 2];


  $deputy = user_access('tripal_hq_permissions deputy');

  global $user;
  $uid = $user->uid;


  $base = variable_get('tripal_apollo_base_table');
  $pid = $base . '_id';
  $sort_field = "name";

  if ($base == "organism") {
    $sort_field = "genus";
  }
  $header = [

    'Name' => ["data" => t('Name'), "field" => "au.name"],
    'Email' => ["data" => t('Email'), "field" => "email"],
    'Record' => ["data" => t('Record'), "field" => 'bt.' . $sort_field],
    'Status' => ["data" => t('Status'), "field" => "status"],
    'Date Created' => [
      "data" => t('Created'),
      "field" => "created",
      'sort' => 'dsc',
    ],
    'Action',
  ];

  $query = db_select("apollo_user_record", "aur")
    ->extend('TableSort')
    ->orderByHeader($header)
    ->extend('PagerDefault');


  $query->fields('aur', [
    'id',
    'record_id',
    'status',
    'created',
    'apollo_user_id',
  ]);
  $query->join('chado.' . $base, 'bt', 'aur.record_id = bt.' . $pid);
  $query->fields('bt', [$sort_field]);


  if ($type != "all") {

    $check = $type_convert[$type];

    $query->condition('status', $check);
  }

  $base = variable_get('tripal_apollo_base_table');
  if ($deputy && $uid != 0 && $uid != 1) {
    $query->join('tripal_hq_permissions', 'thp', 'thp.record_id = aur.record_id');
    $query->condition('thp.uid', $uid);
    $query->condition('thp.base_table', $base);
  }

  $query->join('apollo_instance_record', 'air', 'air.record_id = aur.record_id');

  $query->fields('air', [
    'instance_id',
    'record_id',
  ]);
  $query->join('apollo_user', 'au', 'au.id = aur.apollo_user_id');
  $query->fields('au', ['name', 'email']);
  $query = $query->limit(25);
  $result = $query->execute();

  // Now build the form array
  $rows = [];
  $i = 0;


  // Looping for filling the table rows
  while ($data = $result->fetchObject()) {
    //convert organism_id to organism name
    //TODO:  support multiple base tables

    $record_object = chado_select_record('organism', [
      'genus',
      'species',
    ], ['organism_id' => $data->record_id]);
    $organism = $record_object[0];

    $organism_string = $organism->genus . ' ' . $organism->species;

    // Fill the table rows
    $user_apollo_id = $data->apollo_user_id;
    $path = '/admin/tripal/apollo/users/' . $user_apollo_id;
    $rows[$i][] = l($data->name, $path);
    $rows[$i][] = $data->email;
    $rows[$i][] = $organism_string;
    $rows[$i][] = tripal_apollo_convert_status_for_display($data->status);
    $rows[$i][] = date('M d Y h:i:s A', $data->created);
    $rows[$i][] = l(t('Approve/Deny Request'), 'admin/tripal/apollo/requests/' . $data->id);
    $i++;
  }

  // Output of table with the paging
  $output = theme('table',
    [
      "header" => $header,
      "rows" => $rows,
      "empty" => t("There are no requests.")
      // The message to be displayed if table is empty
    ]
  );

  return $output;
}
