<?php

/**
* @file
* Definitions of constants and globally-scoped variables.  
* Prior to php 5.6, we are not able to declare arrays as constants.
*
*/


/**
 * The default values for each of the available display settings.
 * Some of these may be overridden by Drupal variables settable
 * via the module's admin form.
 *  
 * @global array $GLOBALS['elog_group_by_options']
 * @name $elog_group_by_options 
 */
$GLOBALS['logbook_display_settings'] = array(
  'hide_autologs' => 0,
  'entries_per_page' => '50',
  'group_by' => variable_get('elog_default_group_by','SHIFT'),
  'listing_format' => 'table',
  'enable_preview' => 0,
  'table_date' => 'created',
);


/**
 * The master list of options for how entries may be grouped.
 *  
 * @global array $GLOBALS['elog_group_by_options']
 * @name $elog_group_by_options 
 */
$GLOBALS['elog_group_by_options'] = array(
      'SHIFT' => 'By Shift',
      'DAY' => 'By Day',
      'NONE' => 'No Grouping'
);

/**
 * The master list of options for how many entries to show per page.
 *  
 * @global array $GLOBALS['elog_group_by_options']
 * @name $elog_group_by_options 
 */
$GLOBALS['elog_entries_per_page_options'] = drupal_map_assoc(array(25, 50, 100, 250, 500));


