<?php

/**
 * @file
 * Functions that generate blocks for the Jefferson Lab Electronic Logbook
 *
 * Functions that generate or help to generate elog-specific Drupal
 * blocks for display in the sidebar and elsewhere.  
 */
// Some of the blocks require forms
include_once('forms.inc');

/**
 * Implements hook_block_info().
 */
function elog_block_info() {
  $blocks = array();

  // blocks intended for listings pages

  $blocks['elog_datepicker'] = array(
    'info' => t('Elog DatePicker'),
    'cache' => DRUPAL_CACHE_GLOBAL,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_daterange'] = array(
    'info' => t('Elog Date Range'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_autorefresh'] = array(
    'info' => t('Elog Autorefresh'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "<front>\n" . "book/*\n" . "tag/*\n" . "entries\n",
  );

  $blocks['elog_tag_filters'] = array(
    'info' => t('Elog Tag Filters.'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_book_filters'] = array(
    'info' => t('Elog Book Filters.'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_advanced_filters'] = array(
    'info' => t('Elog Advanced Filters.'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_sticky_listing'] = array(
    'info' => t('Elog Sticky Logentries'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_display_settings'] = array(
    'info' => t('Elog Display Settings'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_useful_links'] = array(
    'info' => t('Elog Context sensitive Useful Links'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_useful_links_3cols'] = array(
    'info' => t('Elog Context sensitive Useful Links in Columns'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "tag/*\n" . "user/*\n" . "entries\n",
  );

  $blocks['elog_dtm_listing'] = array(
    'info' => t('Ongoing Downtime Events'),
    'cache' => DRUPAL_NO_CACHE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "book/*\n" . "entries\n",
  );

  // Blocks intended for logentry pages

  $blocks['elog_backlinks'] = array(
    'info' => t('Elog Backlinks.'),
    'cache' => DRUPAL_CACHE_PER_PAGE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "entry/*",
  );

  $blocks['elog_entry_actions'] = array(
    'info' => t('Elog Entry Actions'),
    //'cache' => DRUPAL_NO_CACHE,
    'cache' => DRUPAL_CACHE_PER_PAGE,
    'visibility' => BLOCK_VISIBILITY_LISTED,
    'pages' => "entry/*",
  );

  $blocks['elog_prev_next'] = array(
    'info' => t('Elog Previous/Next Links'),
    'cache' => DRUPAL_NO_CACHE,
  );
  return $blocks;
}

function elog_get_daterange_block($account) {
  $form = drupal_get_form('elog_daterange_form');
  $content = drupal_render($form);
  $block = array(
    'subject' => t('Date Range'),
    'content' => $content,
  );
  return $block;
}

function elog_get_book_filters_block($account) {
  $content['#markup'] = '<div id="book-filters">';
  $content['#markup'] .= '<p>Deselecting a checkbox will hide the entries in the current listing for the logbook.</p>';
  $content['#markup'] .= '</div>';
  $block = array(
    'subject' => t('Book Filters'),
    'content' => $content,
  );
  return $block;
}

function elog_get_tag_filters_block($account) {
  $content['#markup'] = '<div id="tag-filters">';
  $content['#markup'] .= '<p>Deselecting a checkbox will hide the entries in the current listing that contain the tag.</p>';
  $content['#markup'] .= '</div>';
  $block = array(
    'subject' => t('Tag Filters'),
    'content' => $content,
  );
  return $block;
}

function elog_get_autorefresh_block($account) {


  //@todo add css rule that will hide the divs if no javascript
  //The div to show when enabled (client will hide/show and fill in minutes)
  $content['#markup'] = '<div id=refreshStatusDivEnabled>Minutes until refresh = ';
  $content['#markup'] .= '<span id=refreshMinutesSpan></span>';
  $content['#markup'] .= '</div>';

  //The div to show when enabled (client will hide/show)
  $content['#markup'] .= '<div id=refreshStatusDivDisabled>';
  $content['#markup'] .= '<span id=refreshTextSpan>Autorefresh disabled</span>';
  $content['#markup'] .= '</div>';

  //The div with the checkbox to toggle show/hide
  //$checked = isset($_SESSION['logbook_auto_refresh']) ? 'checked' : '';
  $checked = '';
  $content['#markup'] .= '<div id=refreshCheckboxDiv>';
  $content['#markup'] .= "<label><input type=checkbox $checked id=doLogbookRefresh>";
  $content['#markup'] .= 'Enable autorefresh</label></div>';
  $block = array(
    'subject' => t('Autorefresh'),
    'content' => $content,
  );

  return $block;
}

function elog_get_datepicker_block($account) {
  // A simple div to which a jquery datepicker will get attached at runtime.
  $content['#markup'] = '<div id="elog-calendar"></div>';
  $block = array(
    'subject' => t('Date Picker'),
    'content' => $content,
  );
  return $block;
}

/**
 * Returns backlinks for the logentry node being viewed.
 * Depends on drupal arg(1) call returning a logentry node.
 * 
 */
function elog_get_backlinks_block($account) {
  $content = NULL;
  if (arg(0) == 'node' && is_numeric(arg(1))) {
    $data = elog_get_backlinks(arg(1));
    if (!empty($data)) {
      $content['#prefix'] = '<div id="elog-backlinks">';
      $content['description'] = array(
        'type' => '#markup',
        '#markup' => '<p>The entries below have made references to this one.</p>',
      );
      $content['list'] = array(
        '#items' => $data,
        '#theme' => 'item_list',
      );
      $content['#suffix'] .= '</div>';
    }
    $block = array(
      'subject' => t('Backlinks'),
      'content' => $content,
    );
  }
  return $block;
}

/**
 * Presents the block for a logentry with make a follow-up url
 * Depends on drupal arg(1) call returning a logentry node.
 */
function elog_get_entry_actions_block($account) {
  $content = NULL;
  if (arg(0) == 'node' && is_numeric(arg(1))) {
    $content['#prefix'] = '<div id="elog-entry-actions">';
    $content['list'] = array(
      '#items' => elog_get_entry_action_links(arg(1)),
      '#theme' => 'item_list',
    );
    $content['#suffix'] = '</div>';
  }
  $block = array(
    'subject' => t('Actions'),
    'content' => $content,
  );
  return $block;
}

function elog_get_sticky_listing_block($account) {
  $data = NULL;
  $content = '';
  if (isset($_SESSION['currentLQ'])) {
    $sticky_query = new LogentryQuery();
    $sticky_query->sticky = 1;
    $sticky_query->results_per_page = 0;  //to suppress pager
    $sticky_query->logbooks = $_SESSION['currentLQ']->logbooks;
    $sticky_query->group_by = NULL;
    $data = $sticky_query->getDBQueryResults();
    $data = get_table_from_db_results($data, $sticky_query, FALSE);
  }
  if ($data) {
    $content = array(
      'content' => array(
        'type' => 'markup',
        '#markup' => $data,
        '#prefix' => '<div class="block-content sticky">',
        '#suffix' => '</div>',
      ),
    );
  }

  $block = array(
    'subject' => t('Sticky Entries'),
    'content' => $content,
  );
  return $block;
}

function elog_get_advanced_filters_block($account) {
  $block = '';
  //mypr($account->data);
  //if (isset($account->data['elog_page_preference']) && $account->data['elog_page_preference'] == 'one-column') {
    $content = array(
      'content' => array(
      '#type' => 'markup',
      'form' => drupal_get_form('elog_form_advanced_filters'),
      '#prefix' => '<div class="block-content">',
      '#suffix' => '</div>',
    ),
    );
    
    
    $block = array(
    'subject' => t('Filter Form'),
    'content' => $content,
    );
  //}
  return $block;
}

function elog_get_display_settings_block($account) {
  $form = drupal_get_form('elog_display_settings_form');
  $block = array(
    'subject' => t('Display Settings'),
    'content' => drupal_render($form),
  );
  return $block;
}

function elog_get_dtm_listing_block($account) {

  $block = '';
  $items = _elog_get_dtm_links();

  if (!empty($items)) {
    $content['list'] = array(
      '#prefix' => '<div class="content block-content dtm_listing">',
      '#items' => $items,
      '#theme' => 'item_list',
      '#suffix' => '</div>',
    );



    $block = array(
      'subject' => t('Ongoing Downtime Events'),
      'content' => $content,
    );
  }
  return $block;
}

function elog_get_useful_links_block($account) {
  $block = '';
  if (!isset($account->data['elog_page_preference']) || $account->data['elog_page_preference'] != 'one-column') {
    $content = elog_useful_links_list();
    $block = array(
      'subject' => t('Useful Links'),
      'content' => $content,
    );
  }
  return $block;
}

function elog_get_useful_links_3cols_block($account) {
  $block = '';
  if (isset($account->data['elog_page_preference']) && $account->data['elog_page_preference'] == 'one-column') {
    $list = elog_useful_links_list();
    $content = array(
      'content' => array(
        'type' => 'markup',
        '#markup' => $list,
        '#prefix' => '<div class="block-content cols3">',
        '#suffix' => '</div>',
      ),
    );

    $block = array(
      'subject' => t('Useful Links'),
      'content' => $content,
    );
  }
  return $block;
}

function elog_get_prev_next_block($account) {
  $content = elog_get_prev_next();
  $block = array(
    'content' => $content,
  );
  //mypr($content);
  return $block;
}

/**
 * Implements hook_block_view().
 */
function elog_block_view($block_name = '') {

  global $user;   // Some block displays depend on user prefs

  switch ($block_name) {
    case 'elog_daterange' : return elog_get_daterange_block($user);
      break;
    case 'elog_book_filters' : return elog_get_book_filters_block($user);
      break;
    case 'elog_tag_filters' : return elog_get_tag_filters_block($user);
      break;
    case 'elog_autorefresh' : return elog_get_autorefresh_block($user);
      break;
    case 'elog_datepicker' : return elog_get_datepicker_block($user);
      break;
    case 'elog_backlinks' : return elog_get_backlinks_block($user);
      break;
    case 'elog_entry_actions' : return elog_get_entry_actions_block($user);
      break;
    case 'elog_sticky_listing' : return elog_get_sticky_listing_block($user);
      break;
    case 'elog_dtm_listing' : return elog_get_dtm_listing_block($user);
      break;
    case 'elog_advanced_filters': return elog_get_advanced_filters_block($user);
      break;

    case 'elog_display_settings': return elog_get_display_settings_block($user);
      break;

    case 'elog_useful_links' : return elog_get_useful_links_block($user);
      break;
    case 'elog_useful_links_3cols' : return elog_get_useful_links_3cols_block($user);
      break;
    case 'elog_prev_next' : return elog_get_prev_next_block($user);
      break;
  }
}

/**
 * Returns an array of links to actions available on the current logentry node
 * the provided nid
 * @param type $nid
 * @return type
 * @todo Figure out good way to make a add comment link here
 * @todo Add "Email this entry" link
 */
function elog_get_entry_action_links($nid) {
  $links = array();
  $node = node_load($nid);
  $lognumber = elog_get_lognumber($node);
  $links[] = l('Post Follow-up Entry', 'node/add/logentry', array(
    'query' => array('followup' => $lognumber)));
  //$links[] = l('Add Comment', 'node/' . $node->nid, array('fragment' => 'comments-form-box', 'absolute' => TRUE));
  return $links;
}

/**
 * Returns the themed list of useful links appropriate to the 
 * the currently viewed logbook(s).  Relies on $_SESSION['currentLQ']
 * to know which logbooks are in context.
 * @return string (html)
 */
function elog_useful_links_list() {
  $output = '';
  if (isset($_SESSION['currentLQ'])) {
    $links = elog_get_useful_links($_SESSION['currentLQ']->logbooks);
    $links_list = array();
    $is_internal = is_intranet();
    foreach ($links as $l) {
      foreach ($l->field_link[$l->language] as $data) {
        if ($l->field_internal_only[$l->language][0]['value'] && !$is_internal) {
          continue;
        }
        else {
          $links_list[$data['title']] = l($data['title'], $data['url'], array('attributes' => $data['attributes']));
        }
      }
    }
    if (!empty($links_list)) {
      //tip from comment at http://www.php.net/manual/en/function.ksort.php
      uksort($links_list, 'strnatcasecmp');
      $theme_args = array('items' => $links_list,
        'type' => 'ul',
        'attributes' => array('class' => 'useful_links')
      );
      $output = theme('item_list', $theme_args);
    }
  }
  return $output;
}

/**
 * Performs an entity query and returns an array of useful_links entitites
 * appropriate to the currently selected logbooks set.  Takes appropriate
 * action to avoid showing internal only links to external clients.
 * @param array $logbooks (of tids)
 */
function elog_get_useful_links($logbooks) {
  $links = array();
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node');
  $query->entityCondition('bundle', 'useful_link');
  if (count($logbooks) > 0) {
    $query->fieldCondition('field_logbook', 'tid', array_keys($logbooks));
  }
  //dpm(entity_get_info()); 
  //dpm(field_info_fields()); 
  $query->fieldOrderBy('field_link','title','asc');
  // @todo by weight?
  //$query->propertyOrderBy('changed', 'desc');

  $result = $query->execute();
  if (!empty($result['node'])) {
    $links = entity_load('node', array_keys($result['node']));
  }
  return $links;
}

/**
 * Returns themed links for navigating through the most recently generated
 * elog_listing.  It uses the contents of $_SESSION['elog_listing']
 * to generate appropriate links.
 * @return string
 */
function elog_get_prev_next() {
  $output = '';
  //mypr($_SESSION['elog_listing']);
  if (isset($_SESSION['elog_listing'])) {
    if (count($_SESSION['elog_listing']) > 0) {
      $node = menu_get_object();
      if ($node) {
        if ($node->nid) {
          if ($node->type == 'logentry') {
            $lognumber = $node->field_lognumber[$node->language][0]['value'];
            if (array_key_exists($lognumber, $_SESSION['elog_listing'])) {
              $prev = $_SESSION['elog_listing'][$lognumber]->previous;
              $next = $_SESSION['elog_listing'][$lognumber]->next;
              $output = "<div class=prev-next>";

              // The labels and variables appear to mismatch here.  The reason
              // is because the previous variable really means previous in the
              // listing which in the default sort order is a newer entry and 
              // therefore labelled next (in time) for the user.
              if ($next) {
                $output .= '<span class="prev-link clearfix">';
                $output .= l(t('<<Prev'), "entry/$next", array(
                  'attributes' => array(
                    'title' => t('Progress backwards in time through the last listing of entries')
                  )
                ));
                $output .= "</span>";
              }
              if ($_SESSION['elog_listing']['index']) {
                $output .= '<span class="index-link">';
                $output .= t("<a href=\"@url\" title=\"Return to Listing\">Index</a>", array(
                  '@url' => $_SESSION['elog_listing']['index']));
                $output .= "</span>";
              }
              if ($prev) {
                $output .= '<span class="next-link clearfix">';
                $output .= l(t('Next>>'), "entry/$prev", array(
                  'attributes' => array(
                    'title' => t('Progress forwards in time through the last listing of entries')
                  )
                ));
                $output .= "</span>";
              }

              $output .= "</div>";
            }
          }
        }
      }
    }
  }
  return $output;
}

function _elog_get_dtm_links() {
  $data = _elog_get_data(ELOG_DTM_DATA);
  $event_id_locations = array(
    '1' => 'Accelerator',
    '2' => 'Hall A',
    '3' => 'HAll B',
    '4' => 'Hall C',
    '5' => 'Hall D',
  );


  $links = array();
  if (!empty($data)) {
    foreach ($data as $obj) {
      $title = l($obj->title, ELOG_DTM_OPEN_EVENTS, array(
        'query' => array('event_id' => $obj->id)));
      $title = '<div class=dtm_title>' . $title . '</div>';
      switch ($obj->escalation) {
        case 1 : $severity = 'dtm_yellow';
          break;
        case 2 : $severity = 'dtm_red';
          break;
        default : $severity = 'dtm_normal';
      }
      $duration = sprintf("<div class=\"dtm_duration %s\">[%s]</div>", $severity, $obj->duration);
      $area = '<div class=dtm_area>' . $event_id_locations[$obj->event_type_id] . ': </div>';
      $str = "$area $title $duration";
      $links[] = $str;
    }
  }
  return $links;
}

/**
 * 
 * @param type $obj
 * @return type
 * @deprecated -- this function no longer needed
 */
function _elog_get_dtm_duration_str($obj) {
  $time_down = strtotime($obj->time_down);
  $raw_minutes = round((time() - $time_down) / 60);
  //mypr($raw_minutes);
  $hours = floor(($raw_minutes) / 60);
  //mypr($hours);
  $minutes = round(($raw_minutes) % 60);
  $str = '';
  if ($hours < 1) {
    $str = format_plural($minutes, "1 minute", "@count minutes");
  }
  else {
    $str = format_plural($hours, '1 hour', '@count hours');
    $str .= ', ';
    $str .= format_plural($minutes, '1 minute', '@count minutes');
  }
  if ($hours < 2) {
    $severity = 'dtm_normal';
  }
  if ($hours >= 2 && $hours < 4) {
    $severity = 'dtm_yellow';
  }
  if ($hours >= 4) {
    $severity = 'dtm_red';
  }
  $str = "<div class=\"dtm_duration $severity\">[$str]</div>";
  return $str;
}
