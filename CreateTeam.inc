<?php
/**
 * @file
 * CreateTeam module file for the 'bbl' module
 *
 * This module hold the code for creating a new roster within the bbl module 
 */
/**
 * A multistep form for Creating a Blood Bowl Team
 *
 * Derived from: http://drupal.org/node/717750.
 *
 * This function will build a form for creating a Blood Bowl Team.
 * Page One asks for 
 *  Team name
 *  Select team race
 *  Starting gold, assume 1,000,000 but can change
 * 
 * Page Two will display available players and allow the user to buy players
 * 
 * Page Three allows the user to name the players and organise the roster.
 *
 * @see bbl_create_team_1_submit()
 * @see bbl_create_team_1_validate()
 * @see bbl_create_team_2()
 * @see bbl_create_team_2_submit()
 * @see bbl_create_team_2_validate()
 * @see bbl_create_team_3()
 * @see bbl_create_team_3_submit()
 * @see bbl_create_team_3_validate()
 
 * @ingroup testA
 */
function bbl_create_team_1($form, &$form_state) {

  // Display page 2 if $form_state['page_num'] == 2
  if (!empty($form_state['page_num']) && $form_state['page_num'] == 2) {
    return bbl_create_team_2($form, $form_state);
  }
  if (!empty($form_state['page_num']) && $form_state['page_num'] == 3) {
    return bbl_create_team_3($form, $form_state);
  }
  if (!empty($form_state['page_values'][1])) {
    $form_state['values'] = $form_state['page_values'][1];
  }
  
  $teams = fetch_team_list();
  
  // Otherwise we build page 1.
  $form_state['page_num'] = 1;

  $form['description'] = array(
    '#type' => 'item',
    '#title' => t('Build Team Roster'),
  );

  $form['teamname'] = array(
    '#type' => 'textfield',
    '#title' => t('Team name'),
    '#description' => "Please enter your new teams name.",
    '#size' => 20,
    '#maxlength' => 60,
    '#required' => TRUE,
    '#default_value' => !empty($form_state['values']['teamname']) ? $form_state['values']['teamname'] : '',
  );
  $form['race'] = array(
    '#type' => 'select',
    '#title' => t('Select race'),
    '#options' => $teams,
    '#default_value' => !empty($form_state['values']['race']) ? $form_state['values']['race'] : '',
    '#description' => t('Select the race for your team.'),
  );
  $form['startgold'] = array(
    '#type' => 'textfield',
    '#title' => "Starting gold",
    //'#description' => '',
    '#default_value' => !empty($form_state['values']['startgold']) ? $form_state['values']['startgold'] : '1000000',
  );
  $form['teamfluff'] = array(
    '#type' => 'textarea',
    '#title' => "Team background",
    '#description' => 'Add some team background here',
    '#default_value' => !empty($form_state['values']['teamfluff']) ? $form_state['values']['teamfluff'] : '',
  );
  $form['next'] = array(
    '#type' => 'submit',
    '#value' => 'Next >>',
    '#submit' => array('bbl_create_team_1_submit'),
    '#validate' => array('bbl_create_team_1_validate'),
  );
  return $form;
}

/*
 * Validation function. Check that teamname is long enough. Check that startgold 
 * is positive numeric.
 */
function bbl_create_team_1_validate($form, &$form_state) {
  $teamname = $form_state['values']['teamname'];
  
  if (!validate_unique_team_name($teamname)) {
    form_set_error('teamname', t('Team name already exists in the database.'));
  }
  if (strlen($teamname) < 3) {
    form_set_error('teamname', t('Team name must be more than 3 characters long.'));
  }
  if (!ctype_digit($form_state['values']['startgold'])) {
    form_set_error('startgold', t('Starting gold must be a number.'));
  }
  if ($form_state['values']['startgold'] < 20000) {
    form_set_error('startgold', t('You can not even buy a Snotling with that much! Need more gold!'));
  }
}

/*
 *  Values for page one are saved in $form_state['page_values'][1]
 *  while if page two values exist they become $form_state['values']
 */
function bbl_create_team_1_submit($form, &$form_state) {
  // Values are saved for each page.
  // to carry forward to subsequent pages in the form.
  // and we tell FAPI to rebuild the form.
  $form_state['values']['teamname'] = check_plain($form_state['values']['teamname']);
  $form_state['values']['teamfluff'] = check_plain($form_state['values']['teamfluff']);
  $form_state['page_values'][1] = $form_state['values'];

  if (!empty($form_state['page_values'][2])) {
    $form_state['values'] = $form_state['page_values'][2];
  }

  // When form rebuilds, it will look at this to figure which page to build.
  $form_state['page_num'] = 2;
  $form_state['rebuild'] = TRUE;
}

/*
 * display the players available for this team.
 * 
 * Need to change this function into another form. Add selection boxes to the 
 * table to allow coaches to buy players. This will require us to also write a 
 * theme function to build the form table theme_bbl_create_team_2.
 * 
 * It would be nice to add selection boxes for reroll, cheerleaders, assistant 
 * coaches and apothacary to this form. Ultimately we need a bit of javascript 
 * to calculate the cost and compare to startgold.
 */
function bbl_create_team_2($form, &$form_state) {
  $page_one_values = $form_state['page_values'][1];
  $form_state['page_num'] = 2;
  if (!empty($form_state['page_values'][2])) {
    $form_state['values'] = $form_state['page_values'][2];
  }
  
  $source_table = 'bbl_rs_players';
  $team = fetch_team_data($page_one_values['race']);
  $race = $team->teamname;
  
  $form['players']['#tree'] = TRUE;

  $form['description'] = array(
    '#type' => 'item',
    '#title' => t('You have ' . number_format($page_one_values['startgold']) . 'gold to purchase players for ' . $page_one_values['teamname'] . ' your ' . $race . ' team'),
  );
  
  // add all the standard team gubbins rerolls, fanfactor, cheerleaders, etc,
  $player_roster = array();
  $player_roster['#tree'] = 'TRUE';
  roster_purchasing_table($page_one_values['race'], -1, $player_roster);
  standard_team_stuff($page_one_values['race'], 0, $player_roster);

  $form['players'] = $player_roster;
  
  // Now we add our submit button, for submitting the form results.
  $form['back'] = array(
    '#type' => 'submit',
    '#value' => t('<< Back'),
    '#submit' => array('bbl_create_team_2_back'),
    '#validate' => array('bbl_create_team_2_validate'),
  );
  
  // Now we add our submit button, for submitting the form results.
  $form['next'] = array(
    '#type' => 'submit',
    '#value' => t('Next >>'),
    '#submit' => array('bbl_create_team_2_submit'),
    '#validate' => array('bbl_create_team_2_validate'),
  );
  return $form;
}

/*
 * Store the page 2 values!
 */
function bbl_create_team_2_submit($form, &$form_state) {
  
  $form_state['page_values'][2] = $form_state['values'];

  if (!empty($form_state['page_values'][3])) {
    $form_state['values'] = $form_state['page_values'][3];
  }

  // When form rebuilds, it will look at this to figure which page to build.
  $form_state['page_num'] = 3;
  $form_state['rebuild'] = TRUE;
}
/*
 * Return to previous page
 */
function bbl_create_team_2_back($form, &$form_state) {
  
  $form_state['page_values'][2] = $form_state['values'];

  if (!empty($form_state['page_values'][1])) {
    $form_state['values'] = $form_state['page_values'][1];
  }

  // When form rebuilds, it will look at this to figure which page to build.
  $form_state['page_num'] = 1;
  $form_state['rebuild'] = TRUE;
}

/*
 * Check that player numbers are between 11 and 16. Also check that team value
 * is below the specified starting gold. can do other validations here as they
 * become apparent.
 */
function bbl_create_team_2_validate($form, &$form_state) {
  
  $totqty = 0;
  $totcost = 0;
  $page_one_values = $form_state['page_values'][1];
  // Validate not more the starting gold
  foreach (element_children($form_state['values']['players']) as $id) {
    $qty = $form_state['values']['players'][$id]['quantity'];
    if (is_integer($id)) {
      $totqty += $qty;
    }
    $totcost += $form['players'][$id]['cost']['#value'] * $qty;
  }
  $form_state['values']['treasury'] = $page_one_values['startgold'] - $totcost;
  
  // Validate not more than 16 players
  if ($totqty > 16) {
    form_set_error('teamname', t('Team can not have more than 16 players.'));
  }
  if ($totqty < 11) {
    //TODO - Tournaments can let you buy star players on roster so may not be as valid
    form_set_error('teamname', t('Team needs a minium of 11 players.'));
  }
  if ($totcost > $page_one_values['startgold']) {
    form_set_error('teamname', t('Team value is ' . ($totcost - $page_one_values['startgold']) . ' more than starting gold.'));
  }
}

/*
 * AJAX callbacks to modify the costs displayed on the team builder form. AJAX
 * isn't really necessary here as simple javascript will suffice if I could figure 
 * it out within the Drupal context.
 */
function ajax_bbl_select_callback($form, $form_state) {
  //drupal_set_message(t('values <pre>@values</pre> ', array('@values' => print_r($form_state['triggering_element'], TRUE))));
  if (isset($form_state['input']['_triggering_element_name'])) {
    // get id here, appears in the format 'players[4][quantity]'
    $trigger = $form_state['input']['_triggering_element_name'];
 
    // very old school code, would prefer regular expressions but was having 
    // problems with square brackets in the needle
    $start = strpos($trigger,'[');
    $stop = strpos($trigger,']');
    $id = substr($trigger, $start + 1, $stop - $start - 1);
  }
  $selector = '#total-player-' . $id;
  
  $cost = $form['players'][$id]['cost']['#value'];  
  $quantity = $form_state['values']['players'][$id]['quantity'];
  $form['players'][$id]['total']['#markup'] = number_format($cost * $quantity);
  $totcost = 0;
  $totnumber = 0;
  
  // Loop through the players values to build the total cost and total number of players
  foreach (element_children($form_state['values']['players']) as $pid) {
    $qty = $form_state['values']['players'][$pid]['quantity'];
    $totcost += $form['players'][$pid]['cost']['#value'] * $qty;
    if (is_integer($pid)) {
      $totnumber += $qty;
    }
  }
  
  // List of AJAX commands to replace total player type cost, total cost and player
  // count. Also higlight overspend in red. TODO - hightlight > 16 players.
  $commands = array();
  $commands[] = ajax_command_replace($selector, "<div id='total-player-" . $id . "'>" . number_format($cost * $quantity) . "</div>");
  $commands[] = ajax_command_replace("#player-quantity", "<div id='player-quantity'>" . $totnumber . "</div>");
  $commands[] = ajax_command_replace("#total-player-total", "<div id='total-player-total'>" . number_format($totcost) . "</div>");
  if ($totcost > $form_state['page_values'][1]['startgold']) {
    $commands[] = ajax_command_css("#total-player-total", array('color' => 'red'));
  } 
  else {
    $commands[] = ajax_command_css("#total-player-total", array('color' => 'black'));
  }
  
  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * Theme callback for the bbl_create_team_1 form
 *
 * Because the callback is associated with the bbl_create_team_1 function it
 * will not find the theme_bbl_create_team_2 theme hook. Therefore we need to 
 * check to the existance of the 'players' array and if so render the form correctly.
 * 
 * The theme callback will format the $form data structure into a table.
 *
 * @return
 *   The rendered form
 */
function theme_bbl_create_team_1($variables) {
  // Display page 2 if $form_state['page_num'] == 2
  $form = $variables['form'];
  
  // If the $form['players'] array exists then we need to theme with the 
  // theme_bbl_create_team_2 function. Not sure where $form_state['pag_num'] == 2 is
  if (!empty($form['players'])) {
    return theme_bbl_create_team_2($variables);
  }
  if (!empty($form['player_items'])) {
    return theme_bbl_create_team_3($variables);
  }
  
}

/*
 * Rendering function for the second page of the wizard.
 */
function theme_bbl_create_team_2($variables) {
  $form = $variables['form'];

  // Initialize the variable which will store our table rows
  $rows = array();
  $output = drupal_render($form['description']);
          
  // Iterate over each element in our $form['example_items'] array
  foreach (element_children($form['players']) as $id) {
    // We are now ready to add each element of our $form data to the $rows
    // array, so that they end up as individual table cells when rendered
    // in the final table.  We run each element through the drupal_render()
    // function to generate the final html markup for that element.
    $rows[] = array(
      'data' => array(
        // Add our 'name' column
        drupal_render($form['players'][$id]['name']),
        drupal_render($form['players'][$id]['ma']),
        drupal_render($form['players'][$id]['st']),
        drupal_render($form['players'][$id]['ag']),
        drupal_render($form['players'][$id]['av']),
        drupal_render($form['players'][$id]['baseskills']),
        drupal_render($form['players'][$id]['cost']),
        drupal_render($form['players'][$id]['quantity']),
        drupal_render($form['players'][$id]['total']),
      ),
    );
  }
  
  // We now define the table header values.  Ensure that the 'header' count
  // matches the final column count for your table.
  $header = array(t('Position'), t('MA'), t('ST'), t('AG'), t('AV'), t('Skills'), t('Cost'), t('Quantity'),t('Total'));

  // We also need to pass the drupal_add_tabledrag() function an id which will
  // be used to identify the <table> element containing our tabledrag form.
  // Because an element's 'id' should be unique on a page, make sure the value
  // you select is NOT the same as the form ID used in your form declaration.
  $table_id = 'players-table';

  // We can render our tabledrag table for output.
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => $table_id)));

  // And then render any remaining form elements (such as our submit button)
  $output .= drupal_render_children($form);

  return $output;
}

/*
 * Generate page 3 of the Create Team form. Here we display the purchased players
 * and allows the user to enter their names and reorder the roster as they wish.
 * 
 */
function bbl_create_team_3($form, &$form_state) {
  $page_one_values = $form_state['page_values'][1];
  $page_two_values = $form_state['page_values'][2];
  //drupal_set_message(t('players <pre>@item</pre>', array('@item' => print_r($page_two_values['players'], TRUE))));

  $form['player_items']['#tree'] = TRUE;
  $form['description'] = array(
    '#type' => 'item',
    '#title' => t('Name your players and reorganise your roster by dragging players to the place in the roster you would like them to appear.'),
  );
  
  $player_items = array();
  
  // first process the results from page two into a list of actual players. So 
  // a row for each lineman, blitzer, etc and then blank rows up to 16.
  foreach (element_children($page_two_values['players']) as $id) {
    // player uid's are all numbers. named ids are available to all teams
    if (is_integer($id)) {
      $position = fetch_position_name($id);
      $qty = $page_two_values['players'][$id]['quantity'];
      for ($i = 0; $i < $qty; $i++) {
        $player_items[] = array(
            'type' => $id, 
            'position' => $position, 
            'def_name' => $position . ($i + 1),
            ); 
      }
    }
  }
  // Pad out roster to 16 
  for ($i = count($player_items); $i < 16; $i++) {
    $player_items[] = array('type' => 0, 'position' => 'empty', 'def_name' => ''); 
  }
  
  // Iterate through each database result
  foreach ($player_items as $key => $value) {

    $form['player_items'][$key] = array(
      
      // store player type
      'playertype' => array(
        '#type' => 'hidden',
        '#default_value' => $value['type'],
        '#value' => $value['type'],
        '#title-display' => 'invisible',
      ),
      // We'll use a form element of type '#markup' to display the item name.
      'position' => array(
        '#markup' => $value['position'],
      ),
      'playername' => array(
        '#type' => 'textfield',
        '#default_value' => $value['def_name'],
        '#size' => 30,
        '#maxlength' => 64,
      ),
      'weight' => array(
        '#type' => 'weight',
        '#title' => t('Weight'),
        '#default_value' => $key,// * 10,
        '#delta' => 20,
        '#title-display' => 'invisible',
      ),
    );
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
      '#type' => 'submit', 
      '#value' => t('Save Changes'),
      '#submit' => array('bbl_create_team_3_submit'),
      '#validate' => array('bbl_create_team_3_validate'),
      );
  return $form;
}

function bbl_create_team_3_validate($form, &$formstate) {
  
  
}

/*
 * Save the completed team to the database
 * 
 * First need a team ID for this team before we can save players. Next we need 
 * to save team items: rerolls, cheerleaders, apoth, etc with the team.
 * 
 * Finally add the players to the bbl_user_players table.
 */
function bbl_create_team_3_submit($form, &$form_state) {
  
  $form_state['page_values'][3] = $form_state['values'];
  $page_one_values = $form_state['page_values'][1];
  $page_two_values = $form_state['page_values'][2];
  $page_three_values = $form_state['page_values'][3];
  
  // if fluff empty add a default description
  if (strlen($page_one_values['teamfluff']) < 1) {
    $page_one_values['teamfluff'] = $page_one_values['teamname'] . ' created by ' . $GLOBALS['user']->name . ' on ' . format_date($_SERVER['REQUEST_TIME'], 'medium');
  }
  
  // First create the team_node for this team
  // Next create team in teamtable and retrieve tid.
  // Rerolls and other team items can not be team node fields as
  // they would be editable.
  $team = array();
  $team['team_node_id'] = CreateTeamNode($page_one_values['teamname'], $page_one_values['teamfluff']);
  
  $target_table = 'bbl_user_roster';
  $columns = array('rerolls','cheerleaders','assistantcoaches','apothecary','fanfactor');
  $team_id = 0;   // TODO - Consider if we should use the team_node_id here
  $team['teamname'] = $page_one_values['teamname'];
  $team['tid'] = $page_one_values['race'];
  $team['owner'] = $GLOBALS['user']->uid;
  $team['treasury'] = $page_two_values['treasury'];
  
  foreach ($columns as $value) {
    // Check index exists as not all players have an apothecary
    if (isset($page_two_values['players'][$value])) {
      $team[$value] = $page_two_values['players'][$value]['quantity'];
    }
  }
  
  if (db_table_exists($target_table)) {
    // TODO  - error trapping if this fails
    $team_id = db_insert($target_table)->fields($team)->execute();
  }
  
  $players = array();
  
  foreach (element_children($page_three_values['player_items']) as $pid) {
    // player uid's are all numbers. named ids are available to all teams
    
    //if ($page_three_values['player_items'][$pid]['playertype'] > 0) {
      $players[] = array(
        'teamid' => $team_id,
        'playertype' => $page_three_values['player_items'][$pid]['playertype'],
        'playername' => check_plain($page_three_values['player_items'][$pid]['playername']),
        'weight' => $page_three_values['player_items'][$pid]['weight'],
      );
    //}
  }
  // Need to sort the players by weight before adding the playernumbers and must
  // include the empty roster positions to get the correct numbers. Having empty 
  // roster positions within the table is probably actually a good thing.
  uasort($players, 'sortPlayersOnWeight');
  $i = 1;
  foreach ($players as $key => $value) {
    $players[$key]['playernumber'] = $i++;
  }
  //drupal_set_message(t('players array <pre>@players</pre>', array('@players' => print_r($players, TRUE))));
  $query = db_insert('bbl_user_players')->fields(array('teamid', 'playertype', 'playername','weight','playernumber'));
  foreach ($players as $record) {
    $query->values($record);
  }
  $query->execute();
  
  $url = url('node/' . $team['team_node_id']); 
  drupal_goto($url);
}

/*
 * Comparison function for use when sorting the $players array
 */
function sortPlayersOnWeight($a, $b) {
  if ($a['weight'] == $b['weight']) {
        return 0;
  }
  return ($a['weight'] < $b['weight']) ? -1 : 1;
}

/*
 * Custom theme creates a dragable table form which also allows users to enter 
 * player names and sort the roster to their own preference.
 * TODO - restructure this as it own form so that we can re-order and re-name players at other times
 */
function theme_bbl_create_team_3($variables) {
  $form = $variables['form'];
  $output = drupal_render($form['description']);
  
  // Initialize the variable which will store our table rows
  $rows = array();

  // Iterate over each element in our $form['example_items'] array
  foreach (element_children($form['player_items']) as $id) {

    $form['player_items'][$id]['weight']['#attributes']['class'] = array('example-item-weight');

    $rows[] = array(
      'data' => array(
        // Add our 'name' column
        //drupal_render($form['player_items'][$id]['playertype']),
        // Add our 'name' column
        drupal_render($form['player_items'][$id]['position']),
        // Add our 'description' column
        drupal_render($form['player_items'][$id]['playername']),
        // Add our 'weight' column
        drupal_render($form['player_items'][$id]['weight']),
      ),
      // To support the tabledrag behaviour, we need to assign each row of the
      // table a class attribute of 'draggable'. This will add the 'draggable'
      // class to the <tr> element for that row when the final table is
      // rendered.
      'class' => array('draggable'),
    );
  }

  // We now define the table header values.  Ensure that the 'header' count
  // matches the final column count for your table.
  $header = array(t('Position'), t('Player Name'), t('Weight'));

  $table_id = 'player-items-table';

  // We can render our tabledrag table for output.
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => $table_id)));

  // And then render any remaining form elements (such as our submit button)
  $output .= drupal_render_children($form);
  drupal_add_tabledrag($table_id, 'order', 'sibling', 'example-item-weight');

  return $output;
}

/*
 * Quick function for retreiving the team names. Need to rework the table to include 
 * a unique ID for future.
 */
function fetch_team_list($ruleset = 0) {
  // Get array of team names from DB table Teams
  
  $result = db_query('SELECT tid, teamname FROM bbl_rs_teams 
    WHERE ruleset_id = :ruleset_id', array(':ruleset_id' => 0));
  // If we wish to return the results in a single value index array we use FETCH::COLUMN
  $teams = $result->fetchAll();
  $teamData = array();
  
  foreach($teams as $team) {
    $teamData[$team->tid] = $team->teamname;
  }
  
  //drupal_set_message(t('team data by fetchAllAssoc <pre>@team</pre>', array('@team' => print_r($teamData, TRUE))));
  
  return $teamData;
}

function fetch_team_data($tid, $ruleset = 0) {
  
  // Get array of team names from DB table Teams
  $result = db_query('SELECT teamname, reroll, allowapothecary FROM bbl_rs_teams 
    WHERE ruleset_id = :ruleset_id AND tid = :tid', array(':ruleset_id' => 0, ':tid' => $tid));
  // Return the results as objects
  $teams = $result->fetchAll();
  
  return $teams[0];
}

function fetch_roster_data($roster_id) {
  
  // Get array of team names from DB table Teams
  $result = db_query('SELECT * FROM bbl_user_roster WHERE team_id = :team_id', array(':team_id' => $roster_id));
  // Return the results as objects
  $teams = $result->fetchAll();
  
  return $teams[0];
}

function fetch_position_name($value) {
  
  // Get array of team names from DB table Teams
  $result = db_query('SELECT position FROM bbl_rs_players 
    WHERE dpid = :dpid', array(':dpid' => $value));
  // If we wish to return the results in a single value index array we use FETCH::COLUMN
  $position = $result->fetchAll(PDO::FETCH_COLUMN, 0);
  
  return $position[0];
}

/*
 * Creates a simple array of numbers starting at $min up to $max
 */
function number_array($min, $max) {
  $i = $min < $max ? $min : $max;
  $result = array();
  while ($i <= $max) {
    array_push($result, $i++);
  }
  return $result;
}

/*
 * Check the database to ensure team names are not duplicated.
 */
function validate_unique_team_name($teamname) {
  
  $result = db_query('SELECT teamname FROM bbl_user_roster WHERE teamname = :teamname', array(':teamname' => $teamname));
  //$query = db_select('bbl_user_roster', 't');
  //$query->fields('t', array('teamname'))->condition('teamname', $teamname);
  //$result = $query->execute();
  
  $names = $result->fetchAll(PDO::FETCH_COLUMN, 0);
  
  //drupal_set_message(t('team name validation result <pre>@result</pre>', array('@result' => print_r($names, TRUE))));
  return count($names) > 0 ? FALSE : TRUE;
}

/*
 * build the correct arrays to add the general team items to the roster building
 * table.
 */
function standard_team_stuff($tid, $roster_id = 0, &$players) {
  
  $team = fetch_team_data($tid);
  if ($roster_id > 0) {
	  $roster_id = fetch_roster_data();
  }
  
  $standardstuff = array(
      'rerolls' => array(
          'name' => t('Rerolls'),
          'cost' => $roster_id == 0 ? $team->reroll : $team->reroll * 2,
          'max' => 8,
		  'min' => $roster_id > 0 ? $team->rerolls : 0,
      ),
      'cheerleaders' => array(
          'name' => t('Cheerleaders'),
          'cost' => 10000,
          'max' => 8,
		  'min' => $roster_id > 0 ? $team->cheerleaders : 0,
      ),
      'assistantcoaches' => array(
          'name' => t('Assistant Coaches'),
          'cost' => 10000,
          'max' => 8,
		  'min' => $roster_id > 0 ? $team->assistantcoaches : 0,
      ),
  );
  if ($roster_id == 0) {
	  $standardstuff['fanfactor'] = array(
          'name' => t('Fan Factor'),
          'cost' => 10000,
          'max' => 18,
          'min' => 0,
      );
  }
  if ($team->allowapothecary > 0) {
    $standardstuff['apothecary'] = array(
          'name' => t('Apothecary'),
          'cost' => 50000,
          'max' => $team->allowapothecary,
		  'min' => $roster_id > 0 ? $team->apothecary : 0,
      );
  }
  
  foreach ($standardstuff as $item) {
    
    $players[$item['name']] = array(
        // We'll use a form element of type '#markup' to display the player position.
        'name' => array(
          '#markup' => $item['name'],
          '#attributes' => Array('colspan' => 6),
        ), 
        'cost' => array(
          '#markup' => number_format($item['cost']),
          '#value' => $item['cost'],
          '#attributes' => array(
              'id' => $item['name'] . '-cost',
              'class' => 'money',
              ),
          '#prefix' => '<div id="' . $item['name'] . '-cost">',
          '#suffix' => '</div>',
        ),
        // Use a selection box to choose how many of that type to buy
        'quantity' => array(
          '#type' => 'select',
          '#options' => number_array($item['min'], $item['max']),
          '#default_value' => $item['min'],
//            '#attributes' => array(
//              'id' => $item['name'] . '-quantity',
//              ),
          '#ajax' => array(
            'callback' => 'ajax_bbl_select_callback',
            'wrapper' => 'total-player-' . $item['name'],
            'effect' => 'fade',
            'progress' => array(
              'message' => '',
            ),
          ), 
        ),
        'total' => array(
          '#markup' => number_format($item['cost'] * $item['min']),
          '#prefix' => '<div id="total-player-' . $item['name'] . '">',
          '#suffix' => '</div>',
        ),
      );
  }
  
  $players['TOTAL'] = array(
        // total team cost
        'name' => array(
          '#markup' => 'TOTAL',
          '#attributes' => Array('colspan' => 7, 'align' => 'right'),
        ), 
        'quantity' => array(
          '#markup' => 0,
          '#prefix' => '<div id="player-quantity">',
          '#suffix' => '</div>',
        ),
        'total' => array(
          '#markup' => 0,
          '#prefix' => '<div id="total-player-total">',
          '#suffix' => '</div>',
        ),
      );
  return $players;
}

/*
*  Get team player array for use in the purchasing table
*/
function roster_purchasing_table($tid, $team_id = -1, &$players) {
  
  // build the purchasing table for both new and existing teams 
    
  // If team_id is provided then existing team needs to be queried
  if ($team_id > -1) {
  
    $query = db_select('bbl_rs_players', 'rp');
    $query->join('bbl_user_players','up','rp.dpid = up.playertype');
    $query->condition('teamid', $team_id, '=');
    $query->groupby('dpid');
    $query->fields('rp', array('position', 'ma', 'st', 'ag', 'av', 'skills', 'cost', 'max'));
    $query->addExpression('COUNT(dpid)', 'player_count');
    //  $select = 'SELECT rp.position AS position, rp.ma AS ma, rp.st AS st,rp.ag AS ag, rp.av AS av, rp.skills AS skills, rp.cost AS cost, rp.max AS max, COUNT(dpid) AS player_count 
  //FROM bbl_rs_players as rp INNER JOIN bbl_user_players as up ON rp.dpid = up.playertype WHERE (teamid = 1) 
  //GROUP BY rp.position, rp.ma, rp.st, rp.ag, rp.av, rp.skills, rp.cost, rp.max';
    //drupal_set_message(t('Query. <pre>@roster</pre>', array('@roster' => print($query))));

    $result = $query->execute();
    $teamdata = $result->fetchAll();
  }
  else
  {
	  $query = db_select('bbl_rs_players', 't')->condition('tid', $tid, '=');   
	  $query->fields('t');
	  $result = $query->execute();
    $teamdata = $result->fetchAll();
  }
  
  // loop through the $results to build data array for form table
  foreach ($teamdata as $item) {
    // build the data array for each row
    
    $min = isset($item->player_count) ? $item->player_count : 0;
    $cost = $item->cost;
    
    $players[$item->dpid] = array(
      // We'll use a form element of type '#markup' to display the player position.
      'name' => array(
        '#markup' => check_plain($item->position),
      ), 
      'cost' => array(
        '#markup' => number_format($item->cost),
        '#value' => $item->cost,
        '#attributes' => array(
            'id' => $item->dpid . '-cost',
            'class' => 'money',
            ),
        '#prefix' => '<div id="'.$item->dpid. '-cost">',
        '#suffix' => '</div>',
      ),
      'ma' => array(
        '#markup' => check_plain($item->ma),
      ),
      'st' => array(
        '#markup' => check_plain($item->st),
      ),
      'ag' => array(
        '#markup' => check_plain($item->ag),
      ),
      'av' => array(
        '#markup' => check_plain($item->av),
      ),
      'baseskills' => array(
        '#markup' => check_plain($item->skills),
      ),
      // Use a selection box to choose how many of that type to buy
      'quantity' => array(
        '#type' => 'select',
        '#options' => number_array($min, $item->max),
        '#default_value' => $min,
//          '#attributes' => array(
//              'id' => $item->dpid . '-quantity',
//              ),
        '#ajax' => array(
          'callback' => 'ajax_bbl_select_callback',
          'effect' => 'fade',
          'progress' => array(
            'message' => '',
          ),
        ),
      ),
      'total' => array(
        '#markup' => number_format($item->cost * $min),
        '#prefix' => '<div id="total-player-' . $item->dpid . '">',
        '#suffix' => '</div>',
      ),
    );
  };
  
  return $players;
}

/*
 * Create a new team node when creating a new team.
 */
function CreateTeamNode($teamname, $summary) {
    // TODO - currently very much a test of page creation
  $newTeamNode = new stdClass();

  $newTeamNode->type = 'team_node';
  node_object_prepare($newTeamNode);
  $newTeamNode->uid = $GLOBALS['user']->uid;
  $newTeamNode->name = $GLOBALS['user']->name;

  $newTeamNode->title = $teamname;
  $newTeamNode->language = LANGUAGE_NONE;
  $newTeamNode->body[$newTeamNode->language][0]['value'] = $summary;
  $newTeamNode->body[$newTeamNode->language][0]['summary'] = text_summary($summary);
  $newTeamNode->body[$newTeamNode->language][0]['format'] = 'filtered_html';

  $newTeamNode->comment = 2;

  $newTeamNode->status = 1;        // 1 means published
  $newTeamNode->promote = 0;
  $newTeamNode->revision = 0;

  $newTeamNode->changed = $_SERVER['REQUEST_TIME'];
  $newTeamNode->created = $_SERVER['REQUEST_TIME'];
  node_submit($newTeamNode);
  node_save($newTeamNode);
  
  return $newTeamNode->nid;
}
