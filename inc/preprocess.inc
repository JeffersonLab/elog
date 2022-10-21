<?php
/**
* @file
* Preprocess functions
*
* Functions that generate variables for templates.
*/
//namespace PHPElogLib;

// Import classes from the global namespace that we want to use
//use \Exception;
//use \stdClass;
//use \xmlWriter;
//use PHPElogLib\Logentry as Logentry;

/**
 * Generates variables for the search-results template(s) to use
 * @param array $vars
 */
function elog_preprocess_search_results(array &$vars) {
  //mypr($vars);
}


function elog_preprocess_maintenance_page(&$vars){
  // Intercept the maintenance response when the request method is PUT
  // because logbooks REST API expects XML response.  
  if ($_SERVER['REQUEST_METHOD'] == 'PUT'){
      header('content-type:text/xml');
      print "<?xml version=\"1.0\"?>\n";
      print "<Response stat=\"fail\">\n";
      print "<msg>The server is undergoing maintenance and cannot accept submissions at this time.</msg>\n";
      print "</Response>\n";
      die;
  }
}

/**
 * Generates variables for the htmlmail template(s) to use
 * @param array $vars
 */

function elog_preprocess_htmlmail(&$vars) {
  $vars['content'] = array(); 
  $vars['display_submitted'] = true;  //pixture reloaded template uses
  if (!array_key_exists('node', $vars) || ! is_object($vars['node'])){
    $vars['node'] = new stdClass();
  }
  $vars['node']->content = array();
  //The key will tell us what type of email.
  if ($vars['key'] == 'logentry_notify') {
    $vars['view_mode'] = 'full';
    //@todo figure out how to make a custom view_mode.
    //$_GET['overlay'] = true;
    elog_preprocess_node($vars);
    if (isset($vars['params']) && array_key_exists('comment',$vars['params'])){
        $vars['comment'] = drupal_render(comment_view($vars['params']['comment'], $vars['node'],$vars['view_mode']));
    }
    
  }
  //mypr($vars); die;
  
}

/**
 * Turns an array of fields into a delimited list of rendered elements
 * @param object $node
 * @param string $field_name
 * @param string $sep
 * @return NULL
 */
function implode_rendered_field($node, $field_name, $sep=' ') {
  $output = array();
  $field = field_get_items('node', $node, $field_name);
  if (! empty($field)) {
    foreach ($field as $delta => $item){
      $field_value = field_view_value('node', $node, $field_name, $field[$delta]);
      $output[] = drupal_render($field_value);
    }
  }
  if (count($output) > 0) {
    return implode($sep,$output);    
  }
  return NULL;
}

function elog_preprocess_page(array &$vars) {
  global $user;
  if (isset($vars['node']->type) && $vars['node']->type == 'logentry') {
    $block = module_invoke('elog', 'block_view', 'elog_prev_next');
    $vars['elog_prev_next_block'] = $block['content'];
  }
  if (isset($user->data['elog_page_preference']) && $user->data['elog_page_preference'] == 'one-column'){
      $vars['theme_hook_suggestions'] = array('page__onecolumn');
  }
  //mypr($vars);
}

function elog_preprocess_comment(array &$vars) {
  //mypr($vars);
  $vars['date'] = format_date($vars['comment']->changed, 'custom', 'D, m/d/Y - H:i');
  $vars['datetime'] = format_date($vars['comment']->changed, 'custom', 'Y-m-d\TH:i:sO'); // PHP 'c' format is not proper ISO8601!
  $vars['publication_date'] = '<time datetime="' . $vars['datetime'] . '" pubdate="pubdate">' . $vars['date'] . '</time>';

  $vars['files'] = implode(' ',get_rendered_field_attach('comment', $vars['comment'], 'full'));
  
  $vars['comment_submitted'] = t('by !username on !datetime',
    array(
      '!username' => $vars['author'],
      '!datetime' => $vars['publication_date'],
    )
  );
}


/**
 * Generates variables for node template(s) to use
 * @param array $vars
 */
function elog_preprocess_node(array &$vars) {
  //mypr($vars); 
  if (! array_key_exists('node', $vars)) {
    return FALSE;
  } 
  if ($vars['node']->type == 'logentry') {
    elog_preprocess_logentry($vars);
  }

}

/**
 * Generates variables for template(s) specific to nodes of type logentry
 * @param array $vars
 */
function elog_preprocess_logentry(array &$vars) {

  global $user;
  
  
  //mypr($vars['node']);
  if (isset($vars['content']['links'])){
    if ($vars['view_mode'] == 'elog_email'){
      unset($vars['content']['links']);
      $links = '';
    }else{    
      $links = $vars['content']['links'];
      //mypr($links);
      if (! empty($links['comment'])){
       foreach($links['comment']['#links'] as $name=>$attrs){
         switch ($name){
           case 'comment-comments' : unset($vars['content']['links']['comment']['#links']['comment-comments']);
                                     break;
           case 'comment-add' : $vars['content']['links']['comment']['#links']['comment-add']
                                     ['attributes']['title'] = 'Add a comment to this entry';
                                     break;                        
         }
       } 
      }
    }
  }
  
  // Will allow the current node type to be a class of the content div
  $vars['content_attributes_array']['class'][] = $vars['node']->type;
  
  if (! array_key_exists('node', $vars)) {
    return FALSE;
  } 

  $lang = $vars['node']->language;

  //If the entry has OPS-PR fields, fetch rendered opspr table from
  //legacy server.  
  $opspr = field_get_items('node',$vars['node'],'field_opspr',$lang);
  if ($opspr){
    $url = "http://opsweb.acc.jlab.org/CSUEApps/elog/opspr_item.php?elog_id=".elog_get_lognumber($vars['node']);
    $vars['opspr_table'] = file_get_contents($url);
    //$vars['opspr_table'] = $url;
  }else{
    $vars['opspr_table'] = '';
  }

  $field = field_get_items('node', $vars['node'], 'field_lognumber');
  if (isset($field[0])){
  	$field_value = field_view_value('node', $vars['node'], 'field_lognumber', $field[0],$vars['view_mode']);
  	$vars['lognumber'] = drupal_render($field_value);
  }else{
	$vars['lognumber'] = '';
  }
  $vars['logbooks'] = implode_rendered_field($vars['node'], 'field_logbook');
  //mypr($vars['logbooks']);
  $vars['tags'] = implode_rendered_field($vars['node'], 'field_tags');
  $vars['entrymakers'] = implode_rendered_field($vars['node'], 'field_entrymakers', ', ');
  $vars['extern_ref'] = implode_rendered_field($vars['node'], 'field_extern_ref', '<br />');
  
  $vars['content']['pr'] = '';
  $vars['pr'] = '';
  if (! empty($vars['node']->problem_report)){
    if ($vars['node']->problem_report->needs_attention == 1){
      $pr_status_class = 'needs_attention';
    }else{
      $pr_status_class = 'resolved';
    }
    
    $vars['content']['pr'] = array(
      '#type' => 'fieldset',
      '#title' => "PR (OPS)",
      '#attributes' => array('class' => array('problem_report',$pr_status_class)),
      //'#value' => $vars['node']->problem_report->buildContent(),
      'text'  => $vars['node']->problem_report->buildContent(),
    );
    $vars['pr'] = drupal_render($vars['content']['pr']);
  }
  
  //mypr($vars);
  //mypr($vars['node']->field_references);
  if (! empty($vars['node']->field_references)) {
    $ref_list = array();
    foreach ($vars['node']->field_references[$lang] as $delta => $logref) {
      $ref_node = load_node_by_lognumber($logref['value']);      
      if (! empty($ref_node)){
        $ref_url = drupal_get_path_alias('node/'. $ref_node->nid);
        $ref_str = $logref['value'] . ' - ' . $ref_node->title;
        $ref_list[] = l($ref_str,$ref_url);
      }
    }
    $vars['references'] = implode('<br />', $ref_list);
    //$vars['references'] = theme('item_list',array('items' => $ref_list, ));
  }else{
    $vars['references'] = NULL;
  }
  //mypr($ref_list);

  $backlinks = elog_get_backlinks($vars['node']->nid);
  if (! empty($backlinks)){
    $vars['backlinks'] = implode('<br />', $backlinks);
  }else{
    $vars['backlinks'] = ''; 
  }
  
  //$vars['comments'] = drupal_render(comment_node_page_additions($vars['node']));
  $comments = array_reverse (comment_get_thread($vars['node'],COMMENT_MODE_FLAT,50));
  $vars['comments'] = '';
    foreach ($comments as $delta => $cid) {
    $comment_obj = comment_load($cid);
    $rendered_comment_view = comment_view($comment_obj, $vars['node'],$vars['view_mode']);
    $vars['comments'] .= drupal_render($rendered_comment_view);
  }
  
  
  //if ($vars['comments']){mypr($vars); die;}
  // Publication date, formatted with time element
  $vars['date'] = format_date($vars['node']->created, 'custom', 'D, m/d/Y - H:i');
  $vars['datetime'] = format_date($vars['node']->created, 'custom', 'Y-m-d\TH:i:sO'); // PHP 'c' format is not proper ISO8601!
  $vars['date_linked'] = _elog_link_to_logbook_date($vars['node']->created, $vars['date'], elog_get_logbook_names($vars['node']));
  $vars['publication_date'] = '<time datetime="' . $vars['datetime'] . '" pubdate="pubdate">' . $vars['date_linked'] . '</time>';
  $vars['name'] = $vars['node']->name;
  $vars['node_url'] = '/'.drupal_get_path_alias('node/'. $vars['node']->nid);
  
  
  
  $vars['author_url'] = drupal_get_path_alias('user/'. $vars['node']->uid);
  $vars['author_link'] = l($vars['name'],$vars['author_url']);
  $vars['logentry_submitted'] = t('Submitted by !username on !datetime',
      array(
        '!username' => $vars['author_link'],
        '!datetime' => $vars['publication_date'],
      )
    );
  
  if (! empty($vars['node']->field_tags)) {
    foreach ($vars['node']->field_tags[$lang] as $delta => $tag){
        //Not sure why sometimes the taxonomy term is already loaded here
        //and other times not.  We'll have to check for and load it ourselves
        //those times
        if (array_key_exists('tid', $tag) && (! array_key_exists('taxonomy_term', $tag))) {
          $taxonomy_term = taxonomy_term_load($tag['tid']);
          $vars['node']->field_tags[$lang][$delta]['taxonomy_term'] = $taxonomy_term;
          $tag['taxonomy_term'] = $taxonomy_term;
        }
        //Add Readme as a class if the Readme tag is set on the node
        if (strtoupper($tag['taxonomy_term']->name) == 'README'){
          $vars['title_attributes_array']['class'][] = 'Readme';
      }
    }
  }
  
  // Need to get the images variable before the body because we may need 
  // to place one or more urls inline in the body.
  $images = array();
  $field = field_get_items('node', $vars['node'], 'field_image');
  if (! empty($field)) {
    foreach ($field as $delta => $item){
        // Force creation of the plentybig image style.
        // This is an attempt to force the image derivative to exist
        // when an email of a logentry gets sent before anybody has 
        // had a chance to force on-demand creation of the image style
        // which typically happens when the image is accessed via web
        // browser.
        // Attempts to address https://jira.acc.jlab.org/browse/ELOGWEB-11
       $derivative_uri = image_style_path('plentybig', $item['uri']);
        if (!file_exists($derivative_uri)) {         
          image_style_create_derivative(image_style_load('plentybig'), $item['uri'], $derivative_uri);
        }
      
      $images[$delta] = field_view_value('node', $vars['node'], 
                            'field_image', $field[$delta], $vars['view_mode']);
      
      //mypr($images[$delta]);
      if (array_key_exists('overlay', $_GET) && $_GET['overlay'] == true){
        $images[$delta]['#image_style'] = 'overlay';
        if ($images[$delta]['#item']['width'] > 800){
          $scale_factor = 800/$images[$delta]['#item']['width'];
          $images[$delta]['#item']['width'] = 800;
          $images[$delta]['#item']['height'] = round($images[$delta]['#item']['height']*$scale_factor);        
        }  
        $images[$delta]['#image_style'] = 'overlay';
      }
      
      
      //$images[$delta] = '<div>'.drupal_render($images[$delta]).'</div>';
      //mypr($images[$delta]);
    }
  }
  
  
  $vars['body'] = '';
  if (! empty($vars['node']->body)){
    $field = field_get_items('node', $vars['node'], 'body');
    if (! empty($field)) {
        $field_view = field_view_value('node', $vars['node'], 'body', $field[0], $vars['view_mode']);
        $body = drupal_render($field_view);

        if (preg_match_all('/\[figure:\d+\]/mi', $body, $m)) {

            foreach ($m[0] as $token) {

                if (preg_match('/^\[figure:(\d+)\]$/i', $token, $t)) {

                    $image_num = $t[1] - 1;

                    if (!empty($images)) {
                        if (array_key_exists($image_num, $images)) {
                            $url = '<div class="image-wrapper">' . drupal_render($images[$image_num]) . '</div>';

                            $body = str_replace($token, $url, $body);
                            unset($images[$image_num]);
                            //mypr($vars['content']['body'][0]['#markup']);
                        } else {
                            // Setting a warning here is problematic because it won't show up until
                            // the next page view -- it's too late for showing on this page.
                            //drupal_set_message("$token not found in attached images", 'warning');
                        }
                    }
                }
            }
        }
        $vars['body'] = $body;
    }
  }
  // Any remaining images get captions prepended
  foreach ($images as $delta=>$item){
    $output = '<div class="image-wrapper">';
    if (isset($item['#item']['title']) && $vars['view_mode'] == 'full'){
      $output .= '<div class="image-caption">'.$item['#item']['title'].'</div>';
    }
    $item['#item']['attributes']['view_mode'] = $vars['view_mode'];
    $output .= drupal_render($item);
    $output .= '</div>';
    $images[$delta] = $output;
  }
  $vars['images'] = implode(' ',$images);
  //mypr($vars);

  
  // The non-image file attachments
  $vars['files'] = implode(' ',get_rendered_field_attach('node', $vars['node'], $vars['view_mode']));
  
  // We want last_update to be same format as adaptive theme uses in
  // its $submitted variable.
  $vars['datetime_updated'] = format_date($vars['node']->changed, 'custom', 'Y-m-d\TH:i:sO');
  $vars['custom_date_and_time'] = format_date($vars['node']->changed, 'custom', 'D, m/d/Y - H:i');
  $vars['last_update'] = '<time datetime="' . $vars['datetime_updated'] . '" pubdate="pubdate">' . $vars['custom_date_and_time'] . '</time>';
 
  // Build an $updated variable similar to $submitted, but only
  // if node->created != node->changed
  // Build the submitted variable used by default in node templates
  if ($vars['node']->changed > $vars['node']->created) {
    $vars['updated'] = t('Last updated on !datetime',
      array(
        '!datetime' => $vars['last_update'],
      )
    );
  } else {
    $vars['updated'] = '';
  }
  
if (isset($vars['node']->nid)){
  $comment = new stdClass;
  $comment->nid = $vars['node']->nid;
  //mypr($vars['view_mode']); die;
  if ($vars['view_mode'] != 'teaser' && $vars['view_mode'] != 'search_result') {
    $comment_form = drupal_get_form('comment_form', $comment);
    $form = drupal_render($comment_form);
    $vars['comment_form'] = $form;
  }
}else{
    $vars['comment_form'] = '';
}
  
  //mypr($vars);
  //die('arrived at elog_preprocess_node');
}


function elog_preprocess_user_profile(array &$vars){
  if (array_key_exists('field_first_name', $vars) && array_key_exists('field_last_name', $vars)){
    $vars['full_name'] = $vars['field_first_name'][0]['safe_value'].' '.$vars['field_last_name'][0]['safe_value'];
  }  
  
  $vars['email'] = l($vars['elements']['#account']->mail, "mailto:".$vars['elements']['#account']->mail, array('absolute' => TRUE));
  
  //mypr($vars);
  
}


function _elog_link_to_logbook_date($timeStamp, $dateString, $logbooks){
  global $base_url;
  $url = sprintf("%s/entries?start_date=%s&end_date=%s",
      $base_url, $timeStamp-3600, $timeStamp+3600);
  foreach ($logbooks as $book){
    $url .= '&book='.$book;
  }
  
  return sprintf("<a href=\"%s\">%s</a>", $url, $dateString);
  
}

/**
 * Returns the field_attach field as an array of rendered entities
 * wrapped in divs
 * @param string $entity_type 'node','comment', etc.
 * @param object $entity a node or comment with field_attach
 * @param string $view_mode 'teaser','full', etc.
 * @return array
 */
function get_rendered_field_attach($entity_type, $entity, $view_mode) {
// The non-image file attachments
  //mypr($entity);
  $files = array();
  $field = field_get_items($entity_type, $entity, 'field_attach');
  if (! empty($field)) {
    foreach ($field as $delta => $item){
      $view = field_view_value($entity_type, $entity,'field_attach', $field[$delta], $view_mode);
      $files[$delta] = drupal_render($view);
      $files[$delta] = '<div>'.$files[$delta].'</div>';
    }
  }
  //mypr($files);
  return $files;
}
