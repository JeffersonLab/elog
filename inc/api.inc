<?php

/**
 * @file
 * The Jefferson Lab Electronic Logbook Drupal Module.
 *
 * Functions that support the JSON API 
 */

/**
 * Return the list of logbooks formatted for API specification
 * @return type
 */
function elog_api_logbooks() {
  $vocab = taxonomy_vocabulary_machine_name_load('logbooks');
  $records = _elog_api_taxonomy($vocab->vid);
  return _elog_api_response($records);
}

/**
 * Return the list of tags formatted for API specification
 * @return type
 */
function elog_api_tags() {
  $vocab = taxonomy_vocabulary_machine_name_load('tags');
  $records = _elog_api_taxonomy($vocab->vid);
  return _elog_api_response($records);
}

function _elog_api_response($data, $stat = 'ok') {
  $response = new stdClass();
  $response->stat = $stat;
  $response->data = $data;
  
  if ($_GET['callback']){
    _elog_api_jsonp($response);
    drupal_exit();
  }
  
  header("content-type: application/json");
  header("content-type: Access-Control-Allow-Origin: *");
  header("content-type: Access-Control-Allow-Methods: GET");
  return $response;
}

function _elog_api_error($msg, $stat = 'error') {
  $response = new stdClass();
  $response->stat = $stat;
  $response->message = $msg;
  
  if ($_GET['callback']){
    _elog_api_jsonp($response);
  }
  
  return $response;
}

function _elog_api_jsonp($response){
  header("content-type: application/json");
  header("content-type: Access-Control-Allow-Origin: *");
  header("content-type: Access-Control-Allow-Methods: GET");
  print $_GET['callback']. '(' . json_encode($response) . ');';
  drupal_exit();
}


/**
 * Return the Crew Chief Announcements content
 */
function elog_api_announcements(){
  $query = db_select('node','n')->fields('n'); 
  $query->condition('n.type','crew_chief_announcement','=');
  $query->leftJoin('field_data_field_message', 'm', 'n.nid = m.entity_id');
  $query->fields('m', array('field_message_value', 'field_message_format'));
  
  $results = $query->execute()->fetchAll();
  //var_dump($results);
  $data = array();
  foreach ($results as $announce){
    $data[] = array(
      'title' => $announce->title,
      'message' => $announce->field_message_value,
    ); 
  }
  return _elog_api_response($data);
}



/**
 * Accepts an array as returned by field_image or field_attach
 * and returns a simple associative array with fields keyed 
 * for API clients.
 * @param type $arr
 */
function _elog_api_attachment($arr) {
  $attachment = array(
  'url' => file_create_url($arr['uri']),
  'thumbnail' => image_style_url('large',$arr['uri']),  
  'type' => $arr['filemime'],
  'caption' => $arr['title']
  );
  return $attachment;
}

/**
 * Because it would be so hideous to retrieve the attachments directly via 
 * database query, we don't bother.  We'll fall back to using loading the
 * node and accessing the attachments as fields thereof.
 * @param type $nid
 */
function _elog_api_attachments_struct($nid) {
  $attachments = array();
  $node = node_load($nid);
  if ($node) {
    // Drupal stores images and files separately, but to us 
    // they're both just "attachments"    
    if ($node->field_image) {
      foreach ($node->field_image[$node->language] as $arr) {
        $attachments[] = _elog_api_attachment($arr);
      }
    }

    if ($node->field_attach) {
      foreach ($node->field_attach[$node->language] as $arr) {
        $attachments[] = _elog_api_attachment($arr);
      }
    }
  }
  return $attachments;
}

  function _elog_api_body_struct($format, $body) {
    $struct = array();
    if (stristr($format, 'html')) {
      $struct['format'] = 'html';
    }
    else {
      $struct['format'] = 'text';
    }
    $struct['content'] = $body;
    return $struct;
  }

  /**
   * Returns a set of taxonomy terms formatted for API
   * 
   * @param type $vid
   * @return type
   */
  function _elog_api_taxonomy($vid) {
    $roots = taxonomy_get_tree($vid, 0, 1, FALSE);
    $terms = array();
    foreach ($roots as $r) {
      $kids = taxonomy_get_children($r->tid, $vid);
      $terms[$r->name]['description'] = $r->description;
      foreach ($kids as $k) {
        $terms[$r->name]['children'][] = _elog_api_taxonomy_term($k);
      }
    }
    return $terms;
  }

  /**
   * Returns an entire entry or selected fields of one or more entries.
   * 
   * @param type $lognumber (optionak specific lognumber to retrieve
   * @return type
   */
  function elog_api_entries($lognumber = NULL) {

    // Drupal stoures pager data in globals
    // @see https://api.drupal.org/api/drupal/developer!globals.php/7
    global $pager_total_items;
    global $pager_limits;
    global $pager_page_array;
    global $pager_total;

    
    if ($lognumber) {
      return _elog_api_single_entry($lognumber);
    }

    $lq = new LogentryQuery();
    $lq->group_by = 'NONE';
    $lq->start_date = strtotime("-180 days");
    $lq->addFiltersFromQueryStr($_SERVER['QUERY_STRING']);
    $query = $lq->getDBQuery();
    $results = $query->execute();
    $data = array();
    $data['currentItems'] = $results->rowCount();
    $data['totalItems'] = $pager_total_items[0];     // Only one item in pager array?
    $data['pageLimit'] = $pager_limits[0];
    $data['currentPage'] = $pager_page_array[0];
    $data['pageCount'] = $pager_total[0];

    $data['entries'] = array();
    while ($r = $results->fetchAssoc()) {
      $data['entries'][] = _elog_api_entries_fields($r, $lq->fields);
    }

    return _elog_api_response($data);
  }

 
  
  
  function _elog_api_single_entry($lognumber){
    
    $node = load_node_by_lognumber($lognumber);
        
    if (! empty($node)){  
      $data['entry'] = _elog_api_single_entry_fields($node);                   
      return _elog_api_response($data);
    }else{
      return _elog_api_error('Unable to retrieve requested entry.');
    }
    return;
    
  }
  
  /**
   * Returns a taxonomy term as an associativ array
   * where the drupal database column names have been replaced
   * with "friendlier" aliases where necessary.
   */
  function _elog_api_taxonomy_term($term) {
    $record = array(
      'id' => $term->tid,
      'name' => $term->name,
      'description' => $term->description,
      'weight' => $term->weight,
    );
    return $record;
  }

  function _elog_api_terms_list($tids) {
    $labels = array();
    foreach (explode(',', $tids) as $tid) {
      if (!empty($tid) && is_numeric($tid)){
        $term = taxonomy_term_load($tid);
        $labels[] = $term->name;
      }
    }
    return $labels;
  }

  
  /**
   * Returns the elog node object as a multi-dimensional array suited for
   * being passed to json_encode.  It aliases fields where necessary in order
   * to match the Web Data API documentation.
   * 
   * @see https://logbooks.jlab.org/content/available-fields 
   */
  function _elog_api_single_entry_fields($node){
    $entry = array();
            
    $elog = new Logentry();

    $elog->constructFromNode($node);
    // var_dump($elog);
    // Fields that must exist
    $entry['lognumber'] = $elog->lognumber;
    $entry['title'] = $elog->title;
    $entry['author'] = $elog->author->username;
    $entry['books'] = array_keys($elog->logbooks);
    
    $entry['created'] = _elog_api_timestamp_struct(strtotime($elog->created));
    
    // Fields that might be empty
    //$entry['body'] = _elog_api_body_struct($elog->body_type, $elog->body);  
    $entry['body'] = _elog_api_body_struct($elog->body_type, check_markup($elog->body, $node->body['und'][0]['format']));   
    //$entry['body'] = _elog_api_body_struct($elog->body_type, $node->body['und'][0]['safe_value']);
    
    $entry['tags'] = array_values($elog->tags);
    
    $entry['entrymakers'] = $elog->entry_makers;
    
    $entry['references'] = _elog_api_references_struct($elog->references);
    
    
    $entry['attachments'] = _elog_api_attachments_struct($node->nid);
    $entry['numattachments'] = count($entry['attachments']);      
    
    $entry['numcomments'] = count($elog->comments);
    
    if (count($elog->comments) > 0){
      $entry['comments'] = _elog_api_comments($node);
    }
    

    // Problem Report Fields 
    $pr = $elog->getPR();	
    if (! empty($pr)){
      $entry['needsattention'] = $pr->needs_attention;
    }
    
    return $entry;
    
  }
  
 
  function _elog_api_timestamp_struct($timestamp){
   return array(
      'timestamp' => $timestamp,
      'string'    => date('Y-m-d H:i', $timestamp)
    );
 }
 
 function _elog_api_references_struct($references){
   $data = array();
   if ($references){
    foreach ($references as $type => $reflist){
      foreach ($reflist as $ref){
        $data[] = array('type' => $type, 'lognumber' => $ref);
      }
    }
   }
   return $data;
 }
 
  
 function _elog_api_comments($node){
    
    // Fetch the Comments (if any)  
    $comments = array();
    $cids = comment_get_thread($node, COMMENT_MODE_FLAT, 1000);
    if (count($cids) > 0) {
      foreach ($cids as $cid) {
        $node_comment = comment_load($cid);

        $lang = $node_comment->language;
        $comment = array();
        $comment['body'] = _elog_api_body_struct('text', $node_comment->comment_body[$lang][0]['value']);
        $comment['author'] = $node_comment->name;
        $comment['title'] = $node_comment->subject;
        $comment['created'] =  _elog_api_timestamp_struct($node_comment->created);

        
        //@see https://drupal.stackexchange.com/questions/56487/how-do-i-get-the-path-for-public
        $wrapper = file_stream_wrapper_get_instance_by_uri('public://');
        if ($wrapper && $node_comment->field_image) {          
            $comment['attachments'] = array();
            $publicPath =  $wrapper->realpath();
            foreach ($node_comment->field_image[$node_comment->language] as $arr) {
              $comment['attachments'][] = _elog_api_attachment($arr);
            }
        }  
        $comments[] = $comment;
      }
    }
    // The comments retrieved by comment_get_thread
    // need to be reversed so they are newest first.
    return array_reverse($comments);
 }
  
  
  /**
   * Returns a taxonomy term as an associativ array
   * where the drupal database column names have been replaced
   * with "friendlier" aliases where necessary.
   */
  function _elog_api_entries_fields($record, $fields) {
    if (empty($fields)) {
      $fields = array('lognumber', 'title', 'created','tags');
    }
    
    $entry = array();
    foreach ($fields as $f) {
      switch (strtolower($f)) {
        case 'lognumber' : $entry['lognumber'] = $record['field_lognumber_value'];
          break;
        case 'title' : $entry['title'] = $record['title'];
          break;
        case 'created' : $entry['created'] = array(
            'timestamp' => $record['created'],
            'string' => date('Y-m-d H:i', $record['created']),
          );
          break;
        case 'author' : $entry['author'] = $record['name'];
          break;
        case 'entrymakers' : $entry['entrymakers'] = $record['entrymakers'];
          break;
        case 'numcomments' : $entry['numComments'] = $record['comments'];
          break;
        case 'numattachments' : $entry['numAttachments'] = $record['attachment_count'];
          break;
        case 'needsattention' : $entry['needsAttention'] = $record['needs_attention'];
          break;
        case 'books' : $entry['books'] = _elog_api_terms_list($record['book_tids']);
          break;
        case 'tags' : $entry['tags'] = _elog_api_terms_list($record['tag_tids']);
          break;
        case 'body' : $entry['body'] = _elog_api_body_struct($record['body_format'], $record['body_value']);
          break;
        case 'attachments' : $entry['attachments'] = _elog_api_attachments_struct($record['nid']);
          break;
        
      }
    }
    //var_dump($record);
    return $entry;
  }
  
