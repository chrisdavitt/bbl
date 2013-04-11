<?php

/**
 * @file
 * Admin form code for 'bbl' module
 *
 * This file contains all the Admin forms for bbl. 
 */

/**
 * Page callback for our general info page.
 */
function bbl_admin_page() {
  $markup = t('League Administration pages.');
  return array('#markup' => $markup);
}



function bbl_admin_award_SPP_1($form, &$form_state) {
  
  if (!empty($form_state['page_num']) && $form_state['page_num'] == 2) {
    return bbl_admin_award_SPP_2($form, $form_state);
  }
  // Otherwise we build page 1.
  $form_state['page_num'] = 1;

  // Show a drop down box of current teams
  // Get array of team names from DB table Teams
  
  $query = db_select('bbl_user_roster', 't');
  $query->fields('t', array('team_node_id','teamname'))->orderBy('teamname');

  $result = $query->execute();
  // If we wish to return the results in a single value index array we use FETCH::COLUMN
  $teams = $result->fetchAll();
  $teamData = array();
  
  foreach($teams as $team) {
    $teamData[$team->team_node_id] = $team->teamname;
  }
  
  //drupal_set_message(t('team data by fetchAllAssoc <pre>@team</pre>', array('@team' => print_r($teamData, TRUE))));
  
  $form['team_selected'] = array(
    '#type' => 'select',
    '#title' => t('Select Team'),
    '#options' => $teamData,
    '#default_value' => '',
    '#description' => t('Select team for awarding SPP.'),
  );
  $form['next'] = array(
    '#type' => 'submit',
    '#value' => 'Next >>',
    '#submit' => array('bbl_admin_award_SPP_1_submit'),
    '#validate' => array('bbl_admin_award_SPP_1_validate'),
  );
  
  return $form;
}

function bbl_admin_award_SPP_1_validate($form, &$form_state) {
  
}

function bbl_admin_award_SPP_1_submit($form, &$form_state) {
  // forward the node_id on to the bbl_admin_award_spp pages
  $node_id = $form_state['values']['team_selected'];
  drupal_goto('bbl/' . $node_id . '/award');
  
}

function bbl_admin_award_spp($form, &$form_state, $node_id) {
  
  // TODO - verify $node_id before continuing
  if (is_numeric($node_id) == FALSE) { return; }
  // next check it is an actual team_node ID
  $node = node_load($node_id);
  if($node->type !== 'team_node') { return; }
  
  drupal_add_js(drupal_get_path('module', 'bbl') . '/my_test.js');
  $stat_columns = player_stat_columns(0);
  $form_state['stat_columns'] = $stat_columns;
  
  $field_names = array('st.playernumber', 'st.playername', 'pl.position', 'pl.skills', 'st.spp');
  //drupal_set_message(t('stat_columns by fetchAllAssoc <pre>@team</pre>', array('@team' => print_r($field_names, TRUE))));
    
  $query = db_select('bbl_user_players', 'st');
  $query->addField('st','tpid');
  foreach ($field_names as $field) {  // we use the $field_names array to build up the fields list
    $column = explode('.', $field);
    $query->addField($column[0], $column[1]);
  } 
  $query->join('bbl_rs_players', 'pl', 'st.playertype = pl.dpid');
  $query->join('bbl_user_roster', 'tt', 'tt.team_id = st.teamid');
  $query->condition('tt.team_node_id', $node_id, '=');
  $query->condition('st.playerstate', array(0,2), 'IN');  // do not display dead or retired players
  $query->orderBy('weight');
  $result = $query->execute();
  
  $players = $result->fetchAll();
  // TODO - do we pivot here or in the function?
  $players_and_stats = add_stats_to_roster($node_id, $players);
  
  // Check how many additional skills this player should have
  $skill_levels = get_skill_levels();
  
  $form['description'] = array(
    '#type' => 'item',
    '#title' => t('Edit players SPPs by entering there TD, CAS, MVP and other stats'),
  );
  $form['team_selected'] = array(
      '#type' => 'value',
      '#value' => $node_id,
  );
  $form['players']['#tree'] = TRUE;
  
  foreach ($players_and_stats as $row) {
    
    $tpid = $row['tpid'];
    $no_skills = player_level_check($skill_levels, $row['spp']);
  
    $form['players'][$tpid] = array(
      'tpid' => array(
        '#type' => 'hidden',
        '#value' => $tpid,
        '#title-display' => 'invisible',
      ),
      'playernumber' => array(
        '#markup' => $row['playernumber'],
      ),
      'playername' => array(
        '#markup' => $row['playername'],
      ), 
      'position' => array(
        '#markup' => $row['position'],
      ),
      'skills' => array(
        '#markup' => $row['skills'],
      ),
	    'no_skills' => array(
	      '#type' => 'value',
		    '#value' => $no_skills,
	    ),
    );
    
    foreach ($stat_columns as $item) {
      
      $form['players'][$tpid][$item->player_column] = array (
        '#type' => 'textfield',
        '#value' => $row[$item->player_column],
        '#size' => 3,
        '#maxlength' => 3,
        '#attributes' => array(
          'spp_value' => $item->spp_value,
          'header_text' => $item->short_name,
        ),
      );
    }
    
    $form['players'][$tpid]['spp'] = array(
      '#markup' => $row['spp'],
      '#prefix' => '<div id="player-spp-' . $tpid . '">',
      '#suffix' => '</div>',
    );
            
  }
  
  $form['next'] = array(
    '#type' => 'submit',
    '#value' => t('Save >>'),
    '#submit' => array('bbl_admin_award_spp_submit'),
    '#validate' => array('bbl_admin_award_spp_validate'),
  );
  
  return $form;
}

function bbl_admin_award_spp_submit($form, &$form_state) {

  $skill_levels = get_skill_levels();
  $stat_columns = $form_state['stat_columns'];
  $new_skills = array();
  
  // loop through the players
  foreach(element_children($form_state['input']['players']) as $dpid) {
    $row = $form_state['input']['players'][$dpid];
    
    //$data = array(); //$row;
    
    // The next bit is tricky as we log all the stat changes to the bbl_user_player_stats
    // table. We only wish to log when the stat value changes so need to compare with previous 
    // stat values.
    $previous_values = $form_state['complete form']['players'][$dpid];
    $spp_total = $previous_values['spp']['#markup'];
    
    foreach ($stat_columns as $stat) {
      
	  // TODO - verify that $statname is in the correct ps# format
      $new_value = $row[$stat->player_column];
      $old_value = $previous_values[$stat->player_column]['#value'];
      $delta = $new_value - $old_value;
      
      if ($delta != 0) {
        // add row to bbl_user_player_stats if the stat has changed

        //drupal_set_message(t('@pstat delta @delta', array('@pstat' => print_r($statname, TRUE), '@delta' => $delta)));
        $fields = array(
          'ps_scid' => $stat->scid, 
          'ps_tpid' => $dpid,
          'stat_award' => $delta,
          'spp_award' => $delta * $stat->spp_value,
          'award_date' => $_SERVER['REQUEST_TIME'],
          'award_uid' => $GLOBALS['user']->uid,
          'game_id' => 0,
          'award_comment' => 'manual edit',
        );
        $query = db_insert('bbl_user_player_stats')->fields($fields)->execute();
      
        $spp_total += $delta * $stat->spp_value;
      }
    }
	
	// update the spp total on the user_player table
    //$data['spp'] = $spp_total;
    db_update('bbl_user_players') 
        ->fields(array('spp' => $spp_total))
      ->condition('tpid', $dpid, '=')
      ->execute();
	  
    // Check how many additional skills this player should have
    // TODO - tidy code and Check how many currently assigned skills
    $no_skills = player_level_check($skill_levels, $spp_total);
    $prev_skill_count = $previous_values['no_skills']['#value'];
    if ($no_skills != $prev_skill_count) {
      $new_skills[$dpid] = $no_skills - $prev_skill_count;
      
      $skill_data = array(
          'tpid' => $dpid,
          'sid' => 0,
          'skill_value' => 0,
          'skill_type' => 'U',
          'award_comment' => 'New Skill',
      );
      
      for ($i = 0; $i < $new_skills[$dpid]; $i++) {
        $query = db_insert('bbl_user_player_skills')->fields($skill_data)->execute();
      }
    }
  } // end of player loop
  
  $node_id = $form_state['values']['team_selected'];
  drupal_goto('node/' . $node_id);
}

function bbl_admin_award_spp_validate($form, &$form_state) {
  // validate that only numeric values submitted
  
  foreach (element_children($form_state['values']['players']) as $id) {
    $row = $form_state['values']['players'][$id];
    foreach ($row as $key => $value) {
      if (strpos($key, 'ps') !== FALSE) {
        if (!is_numeric($value)) {
          form_set_error($key, t('Field ' . $key . ' contains non numerical values: ' . $value));
        }
      }
    }
  }
}

function theme_bbl_admin_award_spp($variables) {
  $form = $variables['form'];
  
  $stat_columns = player_stat_columns();
  
  // Initialize the variable which will store our table rows
  $rows = array();
  $output = drupal_render($form['description']);
  
  // first build a header array of header text and field names
  $header = array(
    array('data' => t('No'), 'field' => 'st.playernumber'),
    array('data' => t('Player Name'), 'field' => 'st.playername'),
    array('data' => t('Position'), 'field' => 'pl.position'),
    array('data' => t('Base Skills'), 'field' => 'pl.skills'),
    array('data' => t('SPP'), 'field' => 'st.spp'),
  );
  
  foreach ($stat_columns as $stat) {
    $header[] = array('data' => $stat->short_name, 'field' => 'st.' . $stat->player_column);
  }
  
  foreach (element_children($form['players']) as $id) {
    $data = array();
    
    foreach ($header as $head) {
      $field = $head['field'];
      $exp_field = explode('.', $field);
      $fieldname = $exp_field[1];
      // must pass data to drupal render as an reference variable so that it can mark off the element as having been rendered.
      array_push($data, drupal_render($form['players'][$id][$fieldname]));
    }
    
    $rows[] = array('data' => $data);
  }
  //drupal_set_message(t('row <pre>@team</pre>', array('@team' => print_r($rows, TRUE))));
  $table_id = 'spp-update-table';

  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => $table_id)));

  $output .= drupal_render_children($form);

  return $output;
}

/*
*  Query database for the player statistics in this rule set
*
*/
function player_stat_columns($ruleset_id = 0) {
  // TODO - DONE change to db_query, add naming column of form 'ps' & scid
  $result = db_query('SELECT scid, stat_name, short_name, spp_value, CONCAT(\'ps\',scid) AS player_column FROM bbl_stat_columns WHERE ruleset_id = :ruleset_id ORDER BY scid', array(':ruleset_id' => $ruleset_id));

  $return = array();
  
  // Change the array so that keys are scid column values with 'ps' prefix
  foreach ($result as $col) {
    $row = (array) $col;
    $return[$col->player_column] = $col;
  }
  
  return $return;
}

function player_level_check($levels_table, $spp) {
  // loop through levels table until spp is greater than or equal to the spp
  $no_skills = 0;
  
  foreach ($levels_table as $level) {
    if ($spp >= $level['spp_level']) {
	    $no_skills = $level['number_of_skills'];
	  }
	  else
  	{
	    return $no_skills;
	  }
  }
  return $no_skills;
}

/*
*  Return the skill levels table for this ruleset
*/
function get_skill_levels($ruleset_id = 0) {
  
  $result = db_query('SELECT slid, spp_level, number_of_skills, title
    FROM bbl_rs_skill_award_levels 
    WHERE ruleset_id = :ruleset_id
    ORDER BY slid', array(':ruleset_id' => $ruleset_id));
 
  $skill_level_array = array();
  
  foreach($result as $level) {
    $skill_level_array[] = (array) $level;
  }
  return $skill_level_array;
}

// TODO - not yet used
function get_base_player_roster($node_id) {
  
  $field_names = array('st.playernumber', 'st.playername', 'pl.position', 'pl.skills', 'st.spp');
   
  $query = db_select('bbl_user_players', 'st');
  $query->addField('st','tpid');
  foreach ($field_names as $field) {  // we use the $field_names array to build up the fields list
    $column = explode('.', $field);
    $query->addField($column[0], $column[1]);
  } 
  $query->join('bbl_rs_players', 'pl', 'st.playertype = pl.dpid');
  $query->join('bbl_user_roster', 'tt', 'tt.team_id = st.teamid');
  $query->condition('tt.team_node_id', $node_id, '=');
  $query->condition('st.playerstate', array(0,2), 'IN');  // do not display dead or retired players
  $query->orderBy('weight');
  $result = $query->execute();
  $resultset = $result->FetchAll();
  
  $roster = array();
  foreach ($resultset as $row) {
    $roster[$row->tpid] = (array) $row;
  }
  
  //drupal_set_message(t('stat_columns by fetchAllAssoc <pre>@team</pre>', array('@team' => print_r($field_names, TRUE))));
  return $roster;
}

function add_stats_to_roster($node_id, $roster) {
  /*
  Pivot sample from http://en.wikibooks.org/wiki/MySQL/Pivot_table
  could dynamically generate the query. read the artical carefully
  
  select name,
sum(score*(1-abs(sign(exam-1)))) as exam1,
sum(score*(1-abs(sign(exam-2)))) as exam2,
sum(score*(1-abs(sign(exam-3)))) as exam3,
sum(score*(1-abs(sign(exam-4)))) as exam4
from exams group by name;
*/
  // TODO - verify stat_columns data format
  $stat_columns = player_stat_columns(0);
  //drupal_set_message(t('stat_columns <pre>@data</pre>', array('@data' => print_r($stat_columns, TRUE))));
  
  // TODO - Group on TPID and SUM spp_award
  $result = db_query('SELECT st.tpid AS tpid, ps.ps_scid AS ps_scid, SUM(stat_award) AS stat_count 
    FROM bbl_user_players AS st 
    INNER JOIN bbl_user_player_stats AS ps ON st.tpid = ps.ps_tpid 
    INNER JOIN bbl_user_roster AS tt ON tt.team_id = st.teamid 
    WHERE (tt.team_node_id = :team_node_id) AND (st.playerstate IN (0, 2)) 
    GROUP BY tpid, ps_scid 
    ORDER BY tpid, ps_scid', array(':team_node_id' => $node_id));
  
  // First key the stats data by both the tpid and scid values so they 
  // can be identified
  $pivot = array();
  foreach ($result as $stat) {
	  $pivot[$stat->tpid . ',' . $stat->ps_scid] = $stat->stat_count;
  }
  
  // Next loop thru each stat column for each player. Check if a $pivot result exists else record 0.
  $new_roster = array();
  foreach ($roster as $player) {
    $row = (array) $player;
    
    foreach ($stat_columns as $stat) {
      $key = $row['tpid'] . ',' . $stat->scid;
      // add values to the player arrays within $roster
      $row['ps' . $stat->scid] = isset($pivot[$key]) ? $pivot[$key] : 0;
    }
	  
    $new_roster[] = $row;
  }
  
  return $new_roster;
}