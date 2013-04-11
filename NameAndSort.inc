<?php
/*
 * @file
 * NameAndSort form code for bbl module
 * 
 * Code to control the naming and reordering of player in a roster.
 * 
 * Accessed from menu callback bbl/%/sort
 * 
 * 
 */
function bbl_name_and_sort($form, &$form_state, $node_id) {
  
  // TODO - query for players on team. Remember to sort on weight.
  $query = db_select('bbl_user_players', 'st');
  $query->addField('st', 'tpid');
  $query->addField('pl', 'position');
  $query->addField('st', 'playername');
  $query->addField('st', 'weight');
  $query->join('bbl_rs_players', 'pl', 'st.playertype = pl.dpid');
  $query->join('bbl_user_roster', 'tt', 'tt.team_id = st.teamid');
  $query->condition('tt.team_node_id', $node_id, '=');
  $result = $query->execute();
  //drupal_set_message(t('roster to sort <pre>@roster</pre>', array('@roster' => print_r($result, TRUE))));
  $form['player_items']['#tree'] = TRUE;
  $form['description'] = array(
    '#type' => 'item',
    '#title' => t('Name your players and reorganise your roster by dragging players to the place in the roster you would like them to appear.'),
  );
  $form['team_id'] = array(
    '#type' => 'value',
    '#value' => $node_id,
  );
  $player_items = $result->fetchAll();
  $form['player_items']['#tree'] = 'true';
  // Iterate through each database result
  foreach ($player_items as $row) {
    
    $value = (array) $row;
    
    $form['player_items'][$value['tpid']] = array(
      // store player type
      'tpid' => array(
        '#type' => 'value',
        '#default_value' => $value['tpid'],
        '#value' => $value['tpid'],
        '#title-display' => 'invisible',
      ),
      // We'll use a form element of type '#markup' to display the item name.
      'position' => array(
        '#markup' => $value['position'],
      ),
      'playername' => array(
        '#type' => 'textfield',
        '#default_value' => $value['playername'],
        '#size' => 30,
        '#maxlength' => 64,
      ),
      'weight' => array(
        '#type' => 'weight',
        '#title' => t('Weight'),
        '#default_value' => $value['weight'],
        '#delta' => 20,
        '#title-display' => 'invisible',
      ),
    );
  }

  $form['actions']['submit'] = array(
      '#type' => 'submit', 
      '#value' => t('Save Changes'),
      '#submit' => array('bbl_name_and_sort_submit'),
      '#validate' => array('bbl_name_and_sort_validate'),
      );
  return $form;
}

function bbl_name_and_sort_validate($form, &$formstate) {
  
  
}

/*
 * Save the completed team to the database
 * 
 * First need a team ID for this team before we can save players. Next we need 
 * to save team items: rerolls, cheerleaders, apoth, etc with the team.
 * 
 * Finally add the players to the bbl_user_players table.
 */
function bbl_name_and_sort_submit($form, &$form_state) {
  
  $team_node_id = $form['team_id']['#value'];
  $data = array();
  $i = 1;
  $players = $form_state['input']['player_items'];
  // TODO sort data on weight before the update so that playernumbers easily assigned
  uasort($players, 'sortPlayersOnWeight2');
  //drupal_set_message(t('players array <pre>@players</pre>', array('@players' => print_r($players, TRUE))));
  $i = 1;
  
  foreach(element_children($players) as $tpid) {
    $row = $form_state['input']['player_items'][$tpid];
    
    $data['playername'] = check_plain($row['playername']);
    $data['weight'] = $row['weight'];
    $data['playernumber'] = $i++;
	
    db_update('bbl_user_players') 
        ->fields($data)
      ->condition('tpid', $tpid, '=')
      ->execute();
	
	//drupal_set_message(t('players array <pre>@players</pre>', array('@players' => print_r($players, TRUE))));
  
  }

  
  $url = url('node/' . $team_node_id); 
  drupal_goto($url);
}

/*
 * Comparison function for use when sorting the $players array
 */
function sortPlayersOnWeight2($a, $b) {
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
function theme_bbl_name_and_sort($variables) {
  $form = $variables['form'];
  $output = drupal_render($form['description']);
  
  // Initialize the variable which will store our table rows
  $rows = array();

  foreach (element_children($form['player_items']) as $id) {

    $form['player_items'][$id]['weight']['#attributes']['class'] = array('example-item-weight');

    $rows[] = array(
      'data' => array(
        
        drupal_render($form['player_items'][$id]['position']),
        drupal_render($form['player_items'][$id]['playername']),
        drupal_render($form['player_items'][$id]['weight']),
      ),
      
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
