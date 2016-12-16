<?php
/**
* @file
* Callback functions
*
* Functions that generate data for PAGE_CALLBACK menu items 
* 
* 
*/



/**
 * Generates the page title of elog_listing page based on book
 * selected.
 * @param string $book 
 * @return string
 */
function elog_listing_title($book) {
  if (is_valid_logbook($book)) {
    return t('Logbook: @book', array('@book' => strtoupper($book)));
  }
  else {
    return t('@book is not a valid logbook name.', array('@book' => check_plain($book)));
  }
}

/**
 * Generates the HTML to display the listing for a logbook
 * @param object $term logbook taxonomy item
 * @param string $taxonomy_filter optional taxonomy term to limit results
 * @return string (themed html)
 */
function book_listing($term = NULL, $taxonomy_filter=NULL) {
  if (is_object($term)){
    //$children = taxonomy_get_children($term->tid, $term->vid);
    $children = taxonomy_get_tree($term->vid, $term->tid);
    //mypr($children);
  }else{
    return t('Parameter is not a logbook object');
  }

  $lq = new LogentryQuery();
 
  if (count($children) > 0){
    foreach ($children as $child) {
      $book = strtoupper($child->name);
      $lq->addLogbook($book);
    }
  }else{
    $book = strtoupper($term->name);
    $lq->addLogbook($book);
    //When the user views an individual book, we record the fact
    //in a session variable so that we can default to that selection
    //if he goes to make a logentry.
    $_SESSION['currentBook'] = $book;
  }
  $lq->addTag($taxonomy_filter);

  $listing = elog_listing($lq);
  if (!$listing) {
    $listing = '<h2>' . t('No Items to list.') . '</h2>';
    $listing .= '<h3>' . t('Date Range: @d1 - @d2', array(
          '@d1' => date('Y-m-d', $lq->start_date),
          '@d2' => date('Y-m-d', $lq->end_date))
        ) . '</h3>';
  }

  // Set the rss feed for this book
  // @todo Move this to a block
  $feed = url("rss/$book", array('absolute' => TRUE));
  drupal_add_feed($feed, $book);
  
  return $listing;
}

/**
 * Generates the HTML to display the listing for the named tag
 * @param object $term tags taxonomy item
 * @param string $display Output format (table, teaser, full, etc.)
 * @param string $taxonomy_filter optional logbook term to limit results
 * @return string (themed html)
 */
function tag_listing($term = NULL, $book_filter=NULL) {
  
  if (is_object($term)){
    //$children = taxonomy_get_children($term->tid, $term->vid);
    $children = taxonomy_get_tree($term->vid, $term->tid);
  }else{
    return t('Parameter is not a tag object');
  }
 
  $lq = new LogentryQuery();
  
  if (count($children) > 0){
    foreach ($children as $child) {
      $lq->addTag($child->name);
    }
  }else{
     $lq->addTag($term->name);
  }
  $lq->addLogbook($book_filter);
  
  //$lq->start_date = strtotime(ELOG_ERA);
  //$listing = '<div id="book-filters"></div>';
  $listing = elog_listing($lq);
  if (!$listing) {
    $listing = '<h2>' . t('No Items to list.') . '</h2>';
  }
  return $listing;
  
  
}

/** 
 * Generates the HTML to display global entries listing suitable for
 * the front page.  It will honor per-user front page logbook prefs
 * if they have been set.
 * @param object $term tags taxonomy item
 * @param string $display Output format (table, teaser, full, etc.)
 * @param string $taxonomy_filter optional logbook term to limit results
 * @return string (themed html)
 */
function main_listing(){
  global $user;
  $GLOBALS['conf']['cache'] = FALSE;
  $lq = new LogentryQuery();
  $lq->group_by = 'DAY';
  $lq->cols['books'] = TRUE;
  if (! isset($_REQUEST['form_id'])){   //request trumps prefs
    if ($user->uid > 0 && ! empty($user->data['elog_frontpage_logbooks'])){
      //mypr($user->data['elog_frontpage_logbooks']);
      if (count($user->data['elog_frontpage_logbooks']) == 1){
        $lq->cols['books'] = FALSE;

      }
      foreach ($user->data['elog_frontpage_logbooks'] as $book){
        $lq->addLogbook($book);
      }
    }else{
      $lq->excludeBook('TLOG');
    }
  }  
  
  $listing = elog_listing($lq); 
  if (!$listing) {
    $listing = '<h2>' . t('No Items to list.') . '</h2>';
    $listing .= '<h3>' . t('Date Range: @d1 - @d2', array(
          '@d1' => date('Y-m-d', $lq->start_date),
          '@d2' => date('Y-m-d', $lq->end_date))
        ) . '</h3>';
  }
  return $listing;  
}


/**
 * Generates the HTML to display the listing of comments by 
 * specified user
 * @param string $author
 * @return string (themed html)
 */
function author_comments($name = NULL) {
  $output = '';
  if ($name) {
    $author = user_load_by_name($name);
    //mypr($author);
    if ($author) {
      $cq = new CommentQuery();
      $cq->addAuthor($author);
      $cq->addFiltersFromArray($_GET);
      //mypr($cq);
      $data = $cq->getDBQueryResults();
      //mypr($data);
      $output = t("<h2>Comments posted by @author</h2>", array('@author' => $author->name));
      $output .= get_comments_from_db_results($data);
      $output .= theme('pager');
    }
  }
  return $output;
}



/*
 * Generates a list of log entries from the given Query object
 * combined with extracted URL query parameters from request_uri
 * @param LogentryQuery $lq
 * @param string $display   The desired output format.
 * @return string (themed html)
 */

function elog_listing(LogentryQuery $lq, $display=NULL) {
  
  if (! $display){
    if (isset($_SESSION) && array_key_exists('logbook_display_settings', $_SESSION) &&
        isset($_SESSION['logbook_display_settings']['listing_format'])){
          $display = $_SESSION['logbook_display_settings']['listing_format'];
    }else{
      $display = 'table';
    }
  }
  
  // The $_SESSION would be set by logbook_display_settings_form_submit
  if (isset($_SESSION) && array_key_exists('logbook_display_settings', $_SESSION)){
    $lq->applyDisplaySettings($_SESSION['logbook_display_settings']); 
  }else{
    $lq->applyDisplaySettings($GLOBALS['logbook_display_settings']); 
  }
  
  $lq->addFiltersFromArray($_GET);
  $lq->addFiltersFromQueryStr($_SERVER['QUERY_STRING']);
  
  $_SESSION['logbook_display_settings']['group_by'] = $lq->group_by;
  
  // @todo limit the results for full or teaser mode?
  //if ($display == 'headlines') {
  //  $lq->results_per_page = 25;
  //}
  
  $_SESSION['currentLQ'] = $lq;
  
  
  
  $data = $lq->getDBQueryResults();
  if (!$data) {
    return NULL;
    //@todo include details of the filters that caused no match
    //$output = '<h2>'.t('No Items to list.').'</h2>';
  }
  else {    
    switch ($display) {
      case 'headlines'  : $output = elog_get_teasers_from_db_results($data, $lq);
                          $output .= theme('pager');
                          break;
      case 'full'       : $output = elog_get_nodes_from_db_results($data, $lq);
                          $output .= theme('pager');
                          break;
      case 'table'    : 
      default         : $output = elog_get_table_from_db_results($data, $lq);
                        $output .= theme('pager');
                        break;
    }
                    
  }
  
  //In order to include the block here, we need a way to theme.
  //$dtm = module_invoke('elog','block_view','elog_dtm_listing');
  // mypr($dtm);
  
  return $output;
}



/**
 * Given a result set from db_select(), return a properly themed
 * table.  While iterating over the result set, go ahead and fill the
 * $_SESSION['elog_listing'] array that can be used to navigate
 * through the entries using prev/next links.
 * @param array $results grouped database rows
 * @param LogentryQuery $lq The query used to generate the data
 * @param boolean $make_nave Whether to populate $_SESSION['elog_listing'] or not,
 * @return string (themed html)
 */


function elog_get_teasers_from_db_results($results, LogentryQuery $lq){
  return elog_get_nodes_from_db_results($results, $lq, 'teaser', TRUE);
}

/**
 * Given a result set from db_select(), return a properly themed
 * table.  While iterating over the result set, go ahead and fill the
 * $_SESSION['elog_listing'] array that can be used to navigate
 * through the entries using prev/next links.
 * @param array $results grouped database rows
 * @param LogentryQuery $lq The query used to generate the data
 * @param boolean $make_nave Whether to populate $_SESSION['elog_listing'] or not,
 * @return string (themed html)
 * @todo Limit the items per page in "full" mode?
 */
function elog_get_nodes_from_db_results($results, LogentryQuery $lq, $view_mode='full', $make_navi = TRUE) {

  $prev = NULL;
  $next = NULL;
  $navi = array();
  $output = '';
  foreach ($results as $key => $subset) {
    $rows = array();
    if ($key) {
      $output .= "<h2 class=\"headlines-groupby\">$key</h2>\n";
    }
    foreach ($subset as $r) {
      // It's convenient to set the navigation here while we're
      // already looping through the results.
      $navi[$r['field_lognumber_value']] = new stdClass();
      $navi[$r['field_lognumber_value']]->previous = $prev;
      if (array_key_exists($prev, $navi)) {
        $navi[$prev]->next = $r['field_lognumber_value'];
      }
      $navi[$r['field_lognumber_value']]->next = NULL;
      $prev = $r['field_lognumber_value'];
      
      $node = node_load($r['nid']);
      $view_node = node_view($node,$view_mode);
      $tag_names = array('None');
      if (! empty($view_node['field_tags'])){
        for ($i=0;$i<count($view_node['field_tags']['#items']); $i++){
          $tag_names[$i] =  $view_node['field_tags'][$i]['#title'];
        }
      }
      
      $book_names = array('None');
      if (! empty($view_node['field_logbook'])){
        for ($i=0;$i<count($view_node['field_logbook']['#items']); $i++){
          $book_names[$i] =  $view_node['field_logbook'][$i]['#title'];
        }
      }
      
      $data_tags = implode(',',$tag_names);
      $data_books = implode(',',$book_names);
      
      $view_node['#prefix'] = "<div class=\"teaser-wrapper\" data-tags=\"$data_tags\" data-books=\"$data_books\" >";
      $view_node['#suffix'] = "</div>";
      //mypr($view_node);
      $output .= drupal_render($view_node);
      //mypr($r);  
    }

  }
  if ($make_navi) {
    $_SESSION['elog_listing'] = $navi;
    $_SESSION['elog_listing']['index'] = request_uri();
  }
  return $output;
}


/**
 * Given a result set from db_select(), return a properly themed
 * table.  While iterating over the result set, go ahead and fill the
 * $_SESSION['elog_listing'] array that can be used to navigate
 * through the entries using prev/next links.
 * @param array $results grouped database rows
 * @param LogentryQuery $lq The query used to generate the data
 * @param boolean $make_nave Whether to populate $_SESSION['elog_listing'] or not,
 * @return string (themed html)
 */
function elog_get_table_from_db_results($results, LogentryQuery $lq, $make_navi = TRUE) {

  //http://minidrupal.com/node/19
  //drupal_add_library('system', 'ui.dialog');
  //drupal_add_js('jQuery(document).ready(function(){jQuery("#dialog").dialog({ modal: true });});', 'inline');
  //mypr($results);
  
  if ($lq->group_by == 'DAY' || $lq->group_by == 'SHIFT') {
    $posted = 'posted-short';
  }else{
    $posted = 'posted-full';
  }
  
  $header[0] = array(
    'data' => 'Lognumber',
    'field' => 'lognumber',
    'class' => array('lognumber'),
  );

  $header[1] = array(
    'data' => 'Date',
    'field' => $lq->table_date,
    'class' => array($posted),
  );
  // This is just to keep drupal from marking the wrong column as active for sort
  if (! isset($_GET['sort'])){ $header[1]['sort']  = 'desc'; }
  
  $header[2] = array(
    'data' => 'Author',
    'field' => 'uid',
    'class' => array('author'),
  );
  
  // Add a logbooks column if more than one is active
  if ( $lq->cols['books'] || count($lq->logbooks)> 1 ){
    $header[] = array(
      'data' => 'Logbook(s)',
      'class' => array('logbooks'),
    );  
  }
  
 // Add a tags column if more than one is active
  if ( $lq->cols['tags'] || count($lq->tags)> 1){
    $header[] = array(
      'data' => 'Tags',
      'class' => array('tags'),
    );  
  }
  
  $header[] = array(
    'data' => 'Title',
    'field' => 'title',
    'class' => array('title'),
  );

  $tag_terms = get_taxonomy_terms_by_vocab('Tags');
  $book_terms = get_taxonomy_terms_by_vocab('Logbooks');

  
  $prev = NULL;
  $next = NULL;
  $navi = array();
  //$output = '';
  $output = array();
  
  $delta=0;
  foreach ($results as $key => $subset) {
    $rows = array();
    if ($key) {
      //$output .= "<h2>$key</h2>\n";
      $output[$delta]['heading'] = array(
        '#type'=>'markup',
        '#markup'=>"<h2>$key</h2>\n",
      );
    }
    foreach ($subset as $r) {
      
      
      //mypr($r);
      
      
      // It's convenient to set the navigation here while we're
      // already looping through the results.
      $navi[$r['field_lognumber_value']] = new stdClass();
      $navi[$r['field_lognumber_value']]->previous = $prev;
      if (array_key_exists($prev, $navi)) {
        $navi[$prev]->next = $r['field_lognumber_value'];
      }
      $navi[$r['field_lognumber_value']]->next = NULL;
      $prev = $r['field_lognumber_value'];

      // We want to be able to use tags as class attributes on
      // links
      $logentry_tag_names = array();
      $logentry_terms = array();      
      $tids = explode(',', $r['tag_tids']);
      foreach ($tids as $t) {
        if ($t) {
          $logentry_tag_names[] = $tag_terms[$t];
          $path = 'tag/' . $tag_terms[$t];
          $logentry_terms[] = l($tag_terms[$t], $path);
        }
      }
      $tags = implode(', ', $logentry_terms);
      
      // We want to be able to list books if more than one
      $logentry_book_names = array();
      $book_links = array();
      $books = explode(',', $r['book_tids']);
      foreach ($books as $b) {
        if ($b) {
          $logentry_book_names[] = $book_terms[$b];
          $path = 'book/' . $book_terms[$b];
          $book_links[] = l($book_terms[$b], $path, 
                array('attributes' => array('class' => 'logbooks')));
        }
      }
      $logbooks = implode(', ', $book_links);

      // Here's the real meat - building the row to be displayed
      //$username = theme_author($r['name']);
      //$account = user_load_by_name($r['name']);
      if (elog_uid_is_pseudo_user($r['uid']) && ! empty($r['entrymakers'])){
        $username = $r['entrymakers'];
      }else{
        $username = elog_theme_author($r['name']);
      }

      //$lognumber = theme('lognumber', array('lognumber'=>$r['field_lognumber_value']));
      $lognumber = theme('elog_formatter_elog_lognumber_url', array(
        'element' => array(
          'value' => $r['field_lognumber_value'])
          )
      );
      $logtitle = theme('logtitle', array(
        'logtitle' => $r['title'],
        'lognumber' => $r['field_lognumber_value'],
        'terms' => $logentry_tag_names,
          ));
      // If the date is in the grouping, just use time in the table
      if ($lq->group_by == 'DAY' || $lq->group_by == 'SHIFT') {
        $logdate = date('H:i', $r[$lq->table_date]);
      }
      else {
        $logdate = date('Y-m-d H:i', $r[$lq->table_date]);
      }

      $icons = '';
      if ($r['comment_count'] > 0){
        $div_title = format_plural( $r['comment_count'],
                                    '1 comment',
                                    '@count comments');
        $icons .= sprintf("<div class=\"comments-icon\" title=\"%s\" data-lognumber=\"%s\"></div>",
            $div_title, $r['field_lognumber_value']);
      }
      if ($r['attachment_count'] > 0){
        $div_title = format_plural( $r['attachment_count'],
                                    '1 attachment',
                                    '@count attachments');
        $icons .= sprintf("<div class=\"attachments-icon\" title=\"%s\" data-lognumber=\"%s\"></div>",
            $div_title, $r['field_lognumber_value']);
      }
      
      // Show the clock for legacy downtime entries or the newer type.
      if ((array_key_exists('field_downtime_time_down', $r) && $r['field_downtime_time_down']) 
          || (array_key_exists('dtm_refcount', $r) && $r['dtm_refcount'] > 0)){
        $icons .= sprintf("<div class=\"downtime-icon\" title=\"Downtime Related Entry\" data-lognumber=\"%s\"></div>",
            $r['field_lognumber_value']);
      }
      
      if (isset($r['needs_attention'])){
        if ( $r['needs_attention'] == 1){
          $icons .= sprintf("<div class=\"pr-attention\" title=\"PR Needs Attention\" data-lognumber=\"%s\"></div>",
              $r['field_lognumber_value']);
        }else{
          $icons .= sprintf("<div class=\"pr-resolved\" title=\"PR Resolved\" data-lognumber=\"%s\"></div>",
              $r['field_lognumber_value']);
        }
      }
      
      if ($icons){
        $icons = '<div class=iconsbox>'.$icons.'</div>';
      }
      // Build the columns array
      //$datacols = array($lognumber.$icons, $logdate);
      //@see https://drupal.org/node/1135020 for why class needs to be array
      $datacols = array();
      array_push($datacols, array('data'=>$lognumber.$icons, 'class'=>array('lognumber')));
      array_push($datacols, array('data'=>$logdate, 'class'=>array('logdate')));
      array_push($datacols, array('data'=>$username, 'class'=>array('author')));
      
      if ($lq->cols['books'] || count($lq->logbooks)> 1){
        array_push($datacols, array('data'=>$logbooks, 'class'=>array('logbooks')));
      }

      if ($lq->cols['tags'] || count($lq->tags)> 1){
        array_push($datacols,  array('data'=>$tags, 'class'=>array('tags')));
      }
      
      array_push($datacols, $logtitle);
      
      $data_tags = $logentry_tag_names ? implode(',',$logentry_tag_names) : 'None';

      $rows[] = array(
        'data' => $datacols,
        'class' => $logentry_terms,
        //'data-tags' => implode(',',$logentry_tag_names),
        'data-tags' => $data_tags,
        'data-books' => implode(',',$logentry_book_names),
      );
      
    }
 
   $output[$delta]['table'] = array(
    '#theme' => 'table',
    '#attributes' => array('class'=>array('elog_listing')),
    '#header' => $header,
    '#rows' => $rows,
   );
   $delta++;
  }
  if ($make_navi) {
    $_SESSION['elog_listing'] = $navi;
    $_SESSION['elog_listing']['index'] = elog_get_navi_index();
  }
  return drupal_render($output);
}

function elog_get_navi_index(){
  $uri = request_uri();
  // If the current uri is for an ajax autorefresh, we need to adjust
  $uri = preg_replace('/\/refresh\/entries/','/\/entries/',$uri);
  return $uri;
}

/**
 * Given a result set from db_select(), return a properly themed
 * table.  While iterating over the result set, go ahead and fill the
 * $_SESSION['elog_listing'] array that can be used to navigate
 * through the entries using prev/next links.
 * @param array $results grouped database rows
 * @param LogentryQuery $lq The query used to generate the data
 * @param boolean $make_nave Whether to populate $_SESSION['elog_listing'] or not,
 * @return string (themed html)
 * @todo Limit the items per page in "full" mode?
 */
function get_comments_from_db_results($results) {

  //mypr($results);
  $output = '';
  foreach ($results as $key => $subset) {
    $rows = array();
    if ($key) {
      $output .= "<h2 class=\"headlines-groupby\">$key</h2>\n";
    }
    foreach ($subset as $r) {
      $comment = comment_load($r['cid']);
      $node = node_load($r['nid']);
      $view_comment = comment_view($comment,$node,'full');
      $view_comment['#prefix'] = "<div class=\"comment-wrapper\" >";
      $view_comment['#suffix'] = "</div>";
      $output .= drupal_render($view_comment);
    }

  }
  return $output;
}



function elog_user_comments($var){
  if (is_numeric($var)){
    $user = user_load($var);
  }else{
    $user = user_load_by_name($var);
  }
  return author_comments($user->name);
  
}

