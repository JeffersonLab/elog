<?php

/**
 * @file
 * Forms used by The Jefferson Lab Electronic Logbook Drupal Module.
 *
 * Includes functions that return forms as well as their _submit, and _validate 
 * functions. 
 */

/**
 * Returns the FAPI render array for a form that allows a user
 * to specify a number of session-based display options
 * @param type $form
 * @return string
 */
function elog_display_settings_form($form) {


  $default_values = $GLOBALS['logbook_display_settings'];
  if (array_key_exists('logbook_display_settings', $_SESSION)) {
    foreach ($_SESSION['logbook_display_settings'] as $setting => $value) {
      $default_values[$setting] = $value;
    }
  }

  $form['#id'] = 'logbook-display-settings';
  $form['#prefix'] = '<p>Choose how entries are displayed on this page.'
      . '  You must press <em>Apply</em> for these to take effect.</p>';
  $form['#suffix'] = '';
  $form['hide_autologs'] = array(
    '#title' => t('Hide Autologs'),
    '#type' => 'checkbox',
    '#default_value' => $default_values['hide_autologs'],
    '#required' => FALSE,
  );
  $form['enable_preview'] = array(
    '#title' => t('Enable Preview'),
    '#id' => 'enable_logentry_preview',
    '#type' => 'checkbox',
    '#default_value' => $default_values['enable_preview'],
    '#required' => FALSE,
  );
  $form['entries_per_page'] = array(
    '#type' => 'select',
    '#title' => t('Entries per Page:'),
    '#options' => drupal_map_assoc(array(25, 50, 100, 250, 500)),
    '#default_value' => $default_values['entries_per_page'],
  );
  $form['table_date'] = array(
    '#type' => 'select',
    '#title' => t('Date is:'),
    '#options' => array(
      'changed' => 'Last Update',
      'created' => 'Creation',
    ),
    '#default_value' => $default_values['table_date'],
  );
  $form['group_by'] = array(
    '#type' => 'select',
    '#title' => t('Group Entries:'),
    '#options' => array(
      'SHIFT' => 'By Shift',
      'DAY' => 'By Day',
      'NONE' => 'No Grouping'
    ),
    '#default_value' => $default_values['group_by'],
  );
  $form['listing_format'] = array(
    '#type' => 'select',
    '#title' => t('Display Format:'),
    '#options' => array(
      'table' => 'Table',
      'headlines' => 'Teasers',
      'full' => 'Full Content',
    ),
    '#default_value' => $default_values['listing_format'],
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => 'Apply');

  return $form;
}

function elog_display_settings_form_submit($form, &$form_state) {
  elog_apply_display_settings($form_state['values']);
}

/**
 * Sets $_SESSION values if they exist as keys in array parameter
 * @param type $values
 */
function elog_apply_display_settings($values) {
  if ($values['hide_autologs']) {
    $_SESSION['logbook_display_settings']['hide_autologs'] = 1;
  }
  else {
    $_SESSION['logbook_display_settings']['hide_autologs'] = 0;
  }

  if (array_key_exists('enable_preview', $values) && $values['enable_preview']) {
    $_SESSION['logbook_display_settings']['enable_preview'] = 1;
  }
  else {
    $_SESSION['logbook_display_settings']['enable_preview'] = 0;
  }

  if (is_numeric($values['entries_per_page'])) {
    $_SESSION['logbook_display_settings']['entries_per_page'] = $values['entries_per_page'];
  }

  if (array_key_exists('table_date', $values) && $values['table_date']) {
    $_SESSION['logbook_display_settings']['table_date'] = check_plain($values['table_date']);
  }

  if ($values['group_by']) {
    $_SESSION['logbook_display_settings']['group_by'] = check_plain($values['group_by']);
  }
  if ($values['listing_format']) {
    $_SESSION['logbook_display_settings']['listing_format'] = check_plain($values['listing_format']);
  }
}

/**
 * Builds the form for filtering log entries by date, tag, etc.
 * @param type $form
 * @param type $form_state
 * @return string
 */
function elog_daterange_form($form, &$form_state) {

  if (isset($_SESSION['currentLQ'])) {
    $lq = $_SESSION['currentLQ'];
  }
  else {
    $lq = new LogentryQuery();
  }

  $format = 'Y-m-d H:i';
  $date = date($format);
  // See other available attributes and what they do in date_api_elements.inc
  $form['#id'] = 'elog-daterange-form';
  $form['start_date'] = array(
    //'#type' => 'date_select',
    //'#type' => 'date_text',
    '#type' => 'date_popup',
    '#title' => t('From'),
    '#default_value' => date($format, $lq->start_date),
    '#date_format' => $format,
    '#date_label_position' => 'within',
    '#description' => '',
    '#size' => 10,
  );
  $form['end_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('To'),
    '#default_value' => date($format, $lq->end_date),
    '#date_format' => $format,
    '#date_label_position' => 'within',
    '#description' => '',
    '#size' => 10,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  $form['#method'] = 'get';
  return $form;
}

function elog_form_advanced_filters($form, &$form_state) {

  if (isset($_SESSION['currentLQ'])) {
    $lq = $_SESSION['currentLQ'];
  }
  else {
    $lq = new LogentryQuery();
    $lq->addFiltersFromArray($_GET);
  }
  
  //mypr($_GET);
  //mypr($lq);


  $format = 'Y-m-d H:i';
  $date = date($format);

  $form['date_wrapper'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('field-wrapper', 'inline-labels'),
    ),
  );
  $form['includes'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('field-wrapper', 'inline-labels'),
    ),
  );
  $form['excludes'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('field-wrapper', 'inline-labels'),
    ),
  );
  $form['filter_wrapper'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('field-wrapper'),
    ),
  );
  $form['search_wrapper'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('field-wrapper'),
    ),
  );

  $form['display_wrapper'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('field-wrapper', 'inline-labels'),
    ),
  );


//  $form['date_wrapper']['table_date'] = array(
//    '#type' => 'select',
//    '#title' => t('Date Field:'),
//    '#options' => array(
//      'changed' => 'Last Update',
//      'created' => 'Creation',
//    ),
//    '#default_value' => !empty($_SESSION['logbook_display_settings']['table_date']) ? $_SESSION['logbook_display_settings']['table_date'] : NULL,
//  );
  
  $form['date_wrapper']['label'] = array(
    '#markup' => t('Dates'),
    '#prefix' => '<label>',
    '#suffix' => '</label>',
  );

  
  $form['date_wrapper']['start_date'] = array(
    '#type' => 'textfield',
    '#title' => t('From'),
    '#default_value' => date($format, $lq->start_date),
    '#description' => '',
    '#attributes' => array(
      'class' => array('datetimepicker'),
      'data-placeholder' => 'Logbooks',
    ),
    //'#prefix' => '<label>From: </label>',
    '#size' => 15,
  );
  $form['date_wrapper']['end_date'] = array(
    '#type' => 'textfield',
    '#title' => t('To'),
    '#default_value' => date($format, $lq->end_date),
    '#description' => '',
    '#attributes' => array(
      'class' => array('datetimepicker'),
      'data-placeholder' => 'Logbooks',
    ),
    //'#prefix' => '<label>To: </label>',
    '#size' => 15,
  );


//Reuse the logbooks and tags widgets
//$form_tmp = $form;
//$entity = entity_create('node', array('type'=>'logentry'));
//field_attach_form('node', $entity, $form_tmp, $form_state, NULL, array(
//  'field_name' => 'field_logbook'
//));
//field_attach_form('node', $entity, $form_tmp, $form_state, NULL, array(
//  'field_name' => 'field_tags'
//));
////mypr($form_tmp['field_tags']);
////tweak the settings for current context
//$form_tmp['field_logbook']['und']['#required'] = FALSE;
//$form_tmp['field_logbook']['und']['#default_value'] = array_keys($lq->logbooks);
//$form['filter_wrapper']['logbooks'] = $form_tmp['field_logbook']['und'];  
//
//
//$form_tmp['field_tags']['und']['#required'] = FALSE;
//$form_tmp['field_tags']['und']['#default_value'] = array_keys($lq->tags);
//$form['filter_wrapper']['tags'] = $form_tmp['field_tags']['und'];  
//



  $form['includes']['label'] = array(
    '#markup' => t('Include'),
    '#prefix' => '<label>',
    '#suffix' => '</label>',
  );

  $form['includes']['logbooks'] = array(
    '#type' => 'select',
    '#default_value' => is_array($lq->logbooks) ? array_keys($lq->logbooks) : array(),
    '#multiple' => TRUE,
    '#options' => _elog_widgets_get_taxonomy_tree_options('logbooks'),
    '#attributes' => array(
      'class' => array('select2'),
      'data-placeholder' => 'Logbooks',
    ),
  );

  $form['includes']['tags'] = array(
    '#type' => 'select',
    '#default_value' => is_array($lq->tags) ? array_keys($lq->tags) : array(),
    '#multiple' => TRUE,
    '#options' => _elog_widgets_get_taxonomy_tree_options('tags'),
    '#attributes' => array(
      'class' => array('select2'),
      'data-placeholder' => 'Tags',
    ),
  );



  $form['excludes']['label'] = array(
    '#markup' => t('Exclude'),
    '#prefix' => '<label>',
    '#suffix' => '</label>',
  );

  $form['excludes']['exclude_logbooks'] = array(
    '#type' => 'select',
    '#default_value' => array_keys($lq->exclude_books),
    '#multiple' => TRUE,
    '#options' => _elog_widgets_get_taxonomy_tree_options('logbooks'),
    '#attributes' => array(
      'class' => array('select2'),
      'data-placeholder' => 'Logbooks',
    ),
  );

  $form['excludes']['exclude_tags'] = array(
    '#id' => 'exclude_tags_select',
    '#type' => 'select',
    '#default_value' => array_keys($lq->exclude_tags),
    '#multiple' => TRUE,
    '#options' => _elog_widgets_get_taxonomy_tree_options('tags'),
    '#attributes' => array(
      'class' => array('select2'),
      'data-placeholder' => 'Tags',
    ),
  );

  $form['excludes']['hide_autologs'] = array(
    '#id' => 'exclude_autologs_checkbox',
    '#title' => t('Autologs'),
    '#type' => 'checkbox',
    '#default_value' => !empty($_SESSION['logbook_display_settings']['hide_autologs']) ? $_SESSION['logbook_display_settings']['hide_autologs'] : 0,
    '#required' => FALSE,
  );

  $form['search_wrapper']['search_str'] = array(
    '#type' => 'textfield',
    '#title' => 'Title, Body or Author Contains:',
    '#default_value' => check_plain($lq->search_str),
    '#size' => 40,
  );



  $form['display_wrapper']['group_by'] = array(
    '#type' => 'select',
    '#title' => t('Group:'),
    '#options' => array(
      'SHIFT' => 'By Shift',
      'DAY' => 'By Day',
      'NONE' => 'No Grouping'
    ),
    '#default_value' => (!empty($_SESSION['logbook_display_settings']['group_by'])) ? $_SESSION['logbook_display_settings']['group_by'] : NULL,
  );
  $form['display_wrapper']['listing_format'] = array(
    '#type' => 'select',
    '#title' => t('Display:'),
    '#options' => array(
      'table' => 'Table',
      'headlines' => 'Teasers',
      'full' => 'Full Content',
    ),
    '#default_value' => (!empty($_SESSION['logbook_display_settings']['listing_format'])) ? $_SESSION['logbook_display_settings']['listing_format'] : NULL,
  );
  $form['display_wrapper']['entries_per_page'] = array(
    '#type' => 'select',
    '#title' => t('Paginate:'),
    '#options' => drupal_map_assoc(array(25, 50, 100, 250, 500)),
    '#default_value' => (!empty($_SESSION['logbook_display_settings']['entries_per_page'])) ? $_SESSION['logbook_display_settings']['entries_per_page'] : 100,
  );

  //mypr($form);
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );


  $form['#attached']['js'] = array(
    drupal_get_path('module', 'elog') . '/js/elog.js',
    drupal_get_path('module', 'elog') . '/js/jquery.datetimepicker.js',
  );
  
  $form['#attached']['css'] = array(
    drupal_get_path('module', 'elog') . '/css/jquery.datetimepicker.css',
  );
  
  $form['#attached']['library'][] = array('elog', 'select2');

  //$form['#method'] = 'get';
  return $form;
}

/**
 * 
 * Some of the "advanced filters perform Display Settings work that involves
 * setting session variables.  We do that $_SESSION work here and then
 * forward the remainder of the request as GET parameters.
 * @param type $form
 * @param type $form_state
 */
function elog_form_advanced_filters_submit($form, $form_state) {
  //mypr($form_state);
  //mypr($_POST);
  //$query = drupal_http_build_query($_POST);
  //mypr($query);
  elog_apply_display_settings($form_state['values']);
  drupal_goto('/entries', array('query' => $_POST));
  //die;
}


/**
 * Builds the form that lets users specify their own display preferences
 */
function elog_form_user_prefs($form, &$form_state, $uid) {

  $account = user_load($uid);
  //mypr($account);
  // Logbook selections

  $form['#account'] = $account;
  $form['front_page'] = array(
    '#type' => 'fieldset',
    '#title' => t('Front Page'),
    '#weight' => 5,
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => 'Choose to limit which logooks get displayed on the front page.  If no selection is made, then all logbooks will be shown.',
  );


  $form['front_page']['frontpage_logbooks'] = array(
    '#type' => 'select',
    '#default_value' => (!empty($account->data['elog_frontpage_logbooks'])) ? $account->data['elog_frontpage_logbooks'] : array(),
    '#multiple' => TRUE,
    '#options' => _elog_widgets_get_taxonomy_tree_options('logbooks'),
    '#attributes' => array(
      'class' => array('select2-m'),
      'data-placeholder' => 'Choose Logbooks',
    ),
  );

  // Blocks
  $form['layout'] = array(
    '#type' => 'fieldset',
    '#title' => t('Layout Preferences'),
    '#weight' => 6,
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => 'Choose between two column layout with a sidebar on the right or the old-school single column layout.',
  );

  $form['layout']['page_preference'] = array(
    '#type' => 'radios',
    '#options' => array('one-column' => 'One Column Layout', 'two-column' => 'Two Column Layout'),
    '#default_value' => (!empty($account->data['elog_page_preference'])) ? $account->data['elog_page_preference'] : 'two-column',
  );

  
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => 'Apply');

  $form['#attached']['js'] = array(
    drupal_get_path('module', 'elog') . '/js/elog.js',
    drupal_get_path('module', 'elog') . '/js/jquery.datetimepicker.js',
  );

  $form['#attached']['css'] = array(
    drupal_get_path('module', 'elog') . '/css/jquery.datetimepicker.css',
  );

  $form['#attached']['library'][] = array('elog', 'select2');


  return $form;
}


function elog_form_user_prefs_submit($form, &$form_state) {
  //mypr($user->data);
  $account = $form['#account'];
  if (count($form_state['values']['frontpage_logbooks']) > 0) {
    $account->data['elog_frontpage_logbooks'] = $form_state['values']['frontpage_logbooks'];
  }
  else {
    unset($account->data['elog_frontpage_logbooks']);
  }

  if (isset($form_state['values']['page_preference'])) {
    $account->data['elog_page_preference'] = $form_state['values']['page_preference'];
  }
  if (user_save($account, array('data' => $account->data))) {
    drupal_set_message('Preferences saved');
    $account = user_load($account->uid, TRUE); //force cache refresh  
  }

  //mypr($account); die;
}

