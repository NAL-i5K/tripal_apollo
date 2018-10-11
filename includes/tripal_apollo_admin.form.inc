<?php

/**
 * Builds the user requet table for admin to approve/reject.
 * Implements hook_form().
 */
function tripal_apollo_admin_form($form, &$form_state) {
  //TODO: this should be its own form.  After that,ideally, a site should use default drupal users and permissions instead.
  $user_info = webapollo_register_users();

  $form['users'] = ['#markup' => $user_info];

  return $form;

}

/**
 * List the web apollo registered users with pagination and
 * with Approve/Reject options
 **/
function webapollo_register_users() {
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
      case 'Organism':
        $order = 'organisms';
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

  $query = db_select("webapollo_users", "w");
  $query->fields("w", [
    "uid",
    "name",
    "email",
    "organisms",
    "status",
    "created",
  ]);
  $query->orderBy($order, $sort);
  $query = $query->extend('TableSort')->extend('PagerDefault')->limit(25);

  $result = $query->execute();
  $header = [
    ["data" => t('Name'), "field" => "name"],
    ["data" => t('Email'), "field" => "email"],
    ["data" => t('Organism'), "field" => "Organisms"],
    ["data" => t('Status'), "field" => "status"],
    ["data" => t('Created'), "field" => "created", 'sort' => 'desc'],
    [],
  ];

  // Now build the form array
  $rows = [];
  $i = 0;

  // Looping for filling the table rows
  while ($data = $result->fetchObject()) {

    //convert organism_id to organism name

    $organism = chado_get_organism(['organism_id' => $data->organisms]);

    $organism_string = $organism->genus . ' ' . $organism->species;

    // Fill the table rows
    $rows[$i][] = $data->name;
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
    $rows[$i][] = l(t('Edit'), 'admin/tripal/apollo/users/' . $data->uid);
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
        "empty" => t("Table has no row!")
        // The message to be displayed if table is empty
      ]
    ) . theme("pager");


  return $output;
}


/**
 * Search key by value from an array
 **/
function searchkey($organism_data, $needle) {
  foreach ($organism_data as $key => $data) {
    if (strcmp($key, $needle) == 0) {
      return $key;
    }
  }
  return FALSE;
}
