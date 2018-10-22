<?php

/**
 * Builds the user request table for admin to approve/reject.
 * Implements hook_form().
 */
function tripal_apollo_requests_form($form, &$form_state) {
  $user_info = tripal_apollo_user_table();
  $form['users'] = ['#markup' => $user_info];
  return $form;
}

/**
 * List the web apollo registered users with pagination and
 * with Approve/Reject options
 **/
function tripal_apollo_user_table() {
  $output = "";

  if (isset($_GET['sort']) && isset($_GET['order'])) {
    // Sort it Ascending or Descending?
    if ($_GET['sort'] == 'asc') {
      $sort = 'ASC';
    }
    else {
      $sort = 'DESC';
    }
    // Which column will be sorted
    switch ($_GET['order']) {
      case 'Name':
        $order = 'name';
        break;
      case 'Email':
        $order = 'email';
        break;
      case 'Record':
        $order = 'record_id';
        break;
      case 'Status':
        $order = 'status';
        break;
      default:
        $order = 'created';
    }
  }
  else {
    // Default sort
    $sort = 'DESC';
    $order = 'created';
  }

  $query = db_select("apollo_user_record", "aur");

  $query->fields('aur', [
    'id',
    'record_id',
    'status',
    'created',
    'apollo_user_id'
  ]);
  $query->join('apollo_instance_record', 'air', 'air.record_id = aur.record_id');

  $query->fields('air', [
    'instance_id',
    'record_id',
  ]);
  $query->join('apollo_user', 'au', 'au.id = aur.apollo_user_id');
  $query->fields('au', ['name', 'email']);
  $query->orderBy($order, $sort);
  $query = $query->extend('TableSort')->extend('PagerDefault')->limit(25);
  $result = $query->execute();


  $header = [
    ["data" => t('Name'), "field" => "name"],
    ["data" => t('Email'), "field" => "email"],
    ["data" => t('Record'), "field" => "record_id"],
    ["data" => t('Status'), "field" => "status"],
    ["data" => t('Created'), "field" => "created", 'sort' => 'desc'],
    ["data" => t('Action'), "field" => 'action'],
  ];

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
    $status = '';
    if ($data->status == 1) {
      $status = 'Approved';
    }
    elseif ($data->status == 0) {
      $status = 'Rejected';
    }
    elseif ($data->status == 2) {
      $status = 'Pending';
    }
    $rows[$i][] = $status;
    $rows[$i][] = date('M d Y h:i:s A', $data->created);
    $rows[$i][] = l(t('Approve/Deny Request'), 'admin/tripal/apollo/requests/' . $data->id);
    $i++;
  }

  // Output of table with the paging
  $output = theme_table(
      [
        "header" => $header,
        "rows" => $rows,
        "attributes" => [],
        "sticky" => TRUE,
        // Table header will be sticky
        "caption" => "",
        "colgroups" => [],
        "empty" => t("There are no Apollo registration requests.")
        // The message to be displayed if table is empty
      ]
    ) . theme("pager");

  return $output;
}
