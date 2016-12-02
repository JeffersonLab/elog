<?php

/**
* @file
* Misc helpr functions used by The Jefferson Lab Electronic Logbook Drupal Module.
*
* Miscellaneous helper functions relocated to this include file to minimize clutter
 * in the primary .module file. 
*/

// @todo Move these constants into drupal variabless with admin interface.
// So we won't server inaccessible internal links to
// external browsers who can't access them.
define('INTRANET', '129.57.');
if (!defined('EMAIL_DOMAIN')) {
  define('EMAIL_DOMAIN', '@jlab.org');
}
define('ELOG_ERA', '2009-01-01');

define('ELOG_DTM_DATA','https://accweb.acc.jlab.org/dtm/data/events');
define('ELOG_DTM_OPEN_EVENTS','https://cebaf.jlab.org/dtm/open-events');

// The starting sequence for non-legacy entries
// lognumbers below this are reserved for importing
// entries from legacy systems
define('FIRST_LOGNUMBER', 3000000);


/**
 * Builds a form to allow administrator to set config options
 * @param array $form
 * @param array $form_state
 * @return array
 */
function elog_settings_form($form, &$form_state) {

  $form['elog_deleted_dir'] = array(
    '#type' => 'textfield',
    '#title' => t('Deleted Directory'),
    '#description' => t('Where deleted entries will go.  An absolute path to a file system directory'
        . '(ex: /www/logbooks/files/deleted).  Must be writable by web server.'),
    '#size' => 60,
    '#maxlength' => 120,
    '#required' => TRUE,
    '#default_value' => variable_get('elog_deleted_dir'),
  );

  $form['elog_processed_dir'] = array(
    '#type' => 'textfield',
    '#title' => t('Processed Directory'),
    '#description' =>
      t('Where uploaded XML files will be written after their contents has been saved in the database.'
      . '(ex: /www/logbooks/files/processed).  Must be writable by web server.'),
    '#size' => 60,
    '#maxlength' => 120,
    '#required' => TRUE,
    '#default_value' => variable_get('elog_processed_dir'),
  );
  
  
    $form['elog_schema'] = array(
    '#type' => 'textfield',
    '#title' => t('Schema URL'),
    '#description' =>
      t('The URL of the Schema document used to valid logentry XML files'
      . '(ex: http://server/schema/Logentry.xsd).'),
    '#size' => 60,
    '#maxlength' => 120,
    '#required' => TRUE,
    '#default_value' => variable_get('elog_schema'),
  );

  // Submit button
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );

  return $form;
}

/**
 * Implements hook_form_validate()
 * @param array $form
 * @param array $form_state
 */
function elog_settings_form_validate($form, &$form_state) {

  $del_path = $form_state['values']['elog_deleted_dir'];
  if (!is_writable($del_path)) {
    form_set_error('Permission Error', "$del_path is not writable.");
  }

  $processed_path = $form_state['values']['elog_processed_dir'];
  if (!is_writable($processed_path)) {
    form_set_error('Permission Error', "$processed_path is not writable.");
  }

  $schema_url = $form_state['values']['elog_schema'];
  $response = drupal_http_request($schema_url);
  if (! 200 == $response->code) {
    form_set_error('Schema URL Error', "Unable to acess the Schema URL.");
  }
  
}

/**
 * Implements hook_form_submit()
 * @param array $form
 * @param array $form_state
 */
function elog_settings_form_submit($form, &$form_state) {
  variable_set('elog_deleted_dir', $form_state['values']['elog_deleted_dir']);
  variable_set('elog_processed_dir', $form_state['values']['elog_processed_dir']);
  variable_set('elog_schema', $form_state['values']['elog_schema']);
  drupal_set_message(t('The settings have been saved'));
}

