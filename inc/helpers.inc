<?php

/**
* @file
* Misc helpr functions used by The Jefferson Lab Electronic Logbook Drupal Module.
*
* Miscellaneous helper functions relocated to this include file to minimize clutter
 * in the primary .module file. 
*/



/**
 * Fetches remote data that adheres to the HCO REST interface of
 * returning a json object containg stat="ok|fail" and data properties
 * Utilizes drupal cache to avoid excess calls to remote web server.
 * @param type $url
 * @return null
 */
function _elog_get_data($url, $use_cache = FALSE) {
  if ($use_cache){
    if ($cached = cache_get($url)){
      return $cached->data;
    }
  }
  $fetch = drupal_http_request($url);  
  if (empty($fetch->error)) {
    $response = json_decode($fetch->data);
    if ($response->stat == 'ok') {
      if ($use_cache){
        cache_set($url, $response->data, 'cache', CACHE_TEMPORARY);
      }
      //drupal_set_message('cache miss!! '.$url);
      return $response->data;
    }
    elseif ($response->stat == 'fail') {
      drupal_set_message($response->error);
      //watchdog too/instead?
    }
  }
  else {
    drupal_set_message(t('Unable to retrieve Data'),'error');
  }
  return NULL;
}



/**
 * Callback that always returns false.
 * @return boolean
 */
function elog_always_false(){
  return FALSE;
}



/**
 * Returns an array of links to the logentry nodes that refer to 
 * the provided nid
 * @param type $nid
 * @return type
 */
function elog_get_backlinks($nid) {
  $node = node_load($nid);
  $lognumber = elog_get_lognumber($node);
  $query = db_select('node', 'n');
  $query->fields('n', array('title', 'nid'));
  $query->join('field_data_field_references', 'r', 'r.entity_id = n.nid');
  $query->condition('r.field_references_value', $lognumber, '=');
  $result = $query->execute();
  $links = array();
  while ($data = $result->fetchAssoc()) {
    $links[] = l($data['title'], "node/" . $data['nid']);
  }
  return $links;
}

/**
 * Returns comment objects for a given node nid
 * @param type $nid
 * @return type
 */
function elog_get_comments($nid) {
  $sql = "SELECT cid from comment WHERE nid=? ORDER by created DESC";
  $result = db_query($sql, array($nid));
  $cids = array();
  foreach ($result as $row) {
    $cids[] = $row->cid;
  }
  return comment_load_multiple($cids);
}


/**
 * Function returns a date-based path where a processed
 * XML file will be stored.
 * @return string absolute path
 * @throws Exception
 */
function elog_get_deleted_subdir() {
  $deleted_dir = variable_get('elog_deleted_dir');
  $pathparts = array($deleted_dir, date('Y'), date('m'));
  $path = implode('/', $pathparts);
  if (!(is_dir($path) && is_writable($path) )) {
    if (!mkdir($path, 0755, true)) {
      throw new Exception("Unable to make archive directory $path");
    }
  }
  return $path;
}



/**
 * Convenience function to extract the lognumber from a
 * a node of type logentry
 * @param type $node
 * @return type
 */
function elog_get_lognumber($node) {
  if ($node && $node->type == 'logentry') {
    $lang = $node->language;
    if (isset($node->field_lognumber)){
      return $node->field_lognumber[$lang][0]['value'];
    }
  }
  return NULL;
}

/**
 * Convenience function to return a simple array
 * of the names of the logbooks for an entry.
 * 
 * a node of type logentry
 * @param type $node
 * @return type
 */
function elog_get_logbook_names($node) {
  $items = array();
  if ($node && $node->type == 'logentry') {
    foreach (field_get_items('node', $node, 'field_logbook') as $item){
      $items[$item['tid']] = $item['taxonomy_term']->name;
    }    
  }
  return $items;
}

/**
 * Returns an array of pseudo users in the form
 * uid => name
 * 
 * @param type $node
 * @return type
 */
function elog_get_pseudo_users() {
  if (field_info_instance('user', 'is_pseudo_user', 'user')){    

    if ($cached = cache_get('elog_pseudo_users')){
      return $cached->data;
    }
   
    $sql = "select uid, name 
              from users left join field_data_is_pseudo_user 
              on entity_id=uid 
              where is_pseudo_user_value=1;";
    $result = db_query($sql);
    $data = array();
    foreach ($result as $row) {
      $data[$row->uid] = $row->name;
    }

    cache_set('elog_pseudo_users', $data, 'cache', CACHE_TEMPORARY);
   
    return $data;
  }
}

/**
 * Returns drupal user account object for specified username.
 * 
 * If the jlab_staff module is available, the function will attempt
 * to register a missing user using information from jlab staff service
 * and then return the newly created user.
 * 
 * @param type $username
 * @return type
 */
function elog_get_registered_user($username){
  $account = user_load_by_name($username);
  if (! $account && module_exists('jlab_staff')){
    jlab_staff_register($username);
    $account = user_load_by_name($username);
  }
  return $account;
}


/**
 * Accepts a uid or 
 * @param type $var
 */
function elog_uid_is_pseudo_user($uid){
  $pseudos = elog_get_pseudo_users();
  if ($pseudos && array_key_exists($uid, $pseudos)){
    return TRUE;
  }
  return FALSE;
}


/**
 * Returns a comment as a plain text string. 
 * @todo Should this really be in the theme layer?
 * @param type $comment
 * @return string
 */
function elog_get_plaintext_comment($comment) {
  //var_dump($comment->comment_body);
  $plaintext = ' - by ' . $comment->name;
  $plaintext .= ' on ' . format_date($comment->created, 'custom', 'D, m/d/Y - H:i') . ".\n\n";
  $plaintext .= $comment->subject;
  if (!empty($comment->comment_body[$comment->language][0]['value'])) {
    $plaintext .= " - ";
    $plaintext .= $comment->comment_body[$comment->language][0]['value'] . "\n\n";
  }

  //var_dump($plaintext);die;
  return $plaintext;
}

/**
 * Collapses a field array containing term reference items into a flat
 * array of tids.
 * @param type $field
 * @param type $lang
 * @return type
 */
function elog_get_term_ref_field_items($field, $lang = 'und') {
  $items = array();
  //mypr($field);
  if (!empty($field)){
    foreach ($field[$lang] as $delta => $item) {
      $items[] = $item['tid'];
    }
  }
  return $items;
}

/**
 * Collapses a field array containing text items into a flat
 * array of values.
 * @param type $field
 * @param type $lang
 * @return type
 */
function elog_get_text_field_items($field, $lang = 'und') {
  $items = array();
  //mypr($field);
  if (!empty($field)){
    foreach ($field[$lang] as $delta => $item) {
      $items[] = $item['value'];
    }
  }
  return $items;
}








/**
 * Wrapper that will call kpr only if kpr function is available (i.e. Devel
 * module is enabled).  Prevents white screen of death when devel module is
 * disabled.
 * @param string $var
 */
function mypr($var) {
  if (function_exists('kpr')) {
    $trace = debug_backtrace();
    $caller = array_shift($trace);
    kpr($caller);   // So we know where the original data was from.
    kpr($var);
  }
}




/**
 * Experimental/Possible code related to proxy users who may edit entries
 * owned by others.
 */
//function elog_node_access($node, $op, $account){
//  if (! is_object($node)){ return NODE_ACCESS_IGNORE; }
//  if ($node->type == 'logentry'){
//    //mypr($op);
//    if ($op == 'update'){
//      $node_author = user_load($node->uid);
//      foreach($node_author->field_proxy_users['und'][0] as $delta => $uid){
//        if ($uid == $account->uid){
//          return NODE_ACCESS_ALLOW;
//        }
//      }
//      mypr($node_author);
//    
//    mypr($account);
//      
//    }
//    
//  }
//  return NODE_ACCESS_IGNORE;
//}
