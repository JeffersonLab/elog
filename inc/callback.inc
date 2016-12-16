<?php
/**
* @file
* Callback functions
*
* Functions that generate data for MENU_CALLBACK menu items 
* such as ajax helpers or for cron job urls.
*/
//use \Exception;
//use \stdClass;
//use PHPElogLib\Logentry as Logentry;

/**
 * Provides json output of usernames for use in form autocomplete helpers.
 * Supports comma-delimited lists by only trying to match the final piece.
 * 
 * @param type $string chars to match
 */
function entrymaker_autocomplete($string) {
  $matches = array();
  if (strlen($string) > 2){
    $pieces = array_map('trim', preg_split("/[,]+/", $string, NULL, PREG_SPLIT_NO_EMPTY));
    $last_piece = trim(array_pop($pieces));
    // Don't query DB until at least 3 chars present
    if (strlen($last_piece) > 2){
      $query = db_select('users', 'u');
      $query->fields('u', array('name'));
      $query->leftJoin('field_data_field_first_name', 'f', 'u.uid = f.entity_id');
      $query->addField('f', 'field_first_name_value', 'first');
      $query->leftJoin('field_data_field_last_name', 'l', 'u.uid = l.entity_id');   
      $query->addField('l', 'field_last_name_value', 'last');
      $query->where("lower(concat(u.name,' ',f.field_first_name_value,' ',l.field_last_name_value))
                      LIKE lower(:q)", array(':q'=>'%'.$last_piece.'%')); 
      $result = $query->execute();

      foreach ($result as $row) {
        $display = sprintf("%s (%s %s)", 
            check_plain($row->name), 
            check_plain($row->first),
            check_plain($row->last));
        $key = implode(', ',$pieces);
        if (empty($key)){
          $key = $row->name.', ';
        }else{
          $key .= ','.$row->name.', ';
        }
        $matches[$key] = $display;
      }
      if(empty($matches)) $matches[] = t('No users found for: %string', array('%string' => $last_piece));
    }
  }
  // Return the result to the form in json
  drupal_json_output($matches);
}

/**
 * Provides json output of usernames for use in form autocomplete helpers
 * @param type $string chars to match
 * @todo improve to handle atlis, etc.
 */
function references_autocomplete($string) {
  $matches = array();
  if (strlen($string) > 2){
   
    $query = db_select('node', 'n')->fields('n', array('title','created'));
    $query->join('field_data_field_lognumber', 'l', 'n.nid = l.entity_id'); 
    $query->fields('l',array('lognumber'=>'field_lognumber_value')); 
    if (is_numeric($string)){
      $query->where("l.field_lognumber_value LIKE :q", array(':q'=>'%'.$string.'%'));
    }else{
      $query->where("n.title LIKE :q", array(':q'=>'%'.$string.'%'));
    }
    $result = $query->execute();
    //var_dump($result);
    // save the query to matches
    foreach ($result as $row) {
      $display = $row->field_lognumber_value.' - '.check_plain($row->title);
      $matches[$row->field_lognumber_value] = $display;
    }
    if(empty($matches)) {
     $matches[] = t('No logentries match: %string', array('%string' => $string));
     //$matches[] = $string.$query->__toString();  
    }
  }
  
  // Return the result to the form in json
  drupal_json_output($matches);
}

/**
 * Provides json output of usernames for use in form autocomplete helpers
 * @param type $string chars to match
 * @todo improve to show/search first, last names too.
 */
function email_autocomplete($string) {
  $matches = array();
    if (strlen($string) > 2){
    $pieces = array_map('trim', preg_split("/[,]+/", $string, NULL, PREG_SPLIT_NO_EMPTY));
    $last_piece = trim(array_pop($pieces));
    // Don't query DB until at least 3 chars present
    if (strlen($last_piece) > 2){

      //search table `article` for Article that begin with the letters the user enters in the form
      $query = db_select('users', 'u');
      $query->fields('u', array('mail'));
      $query->leftJoin('field_data_field_first_name', 'f', 'u.uid = f.entity_id');
      $query->addField('f', 'field_first_name_value', 'first');
      $query->leftJoin('field_data_field_last_name', 'l', 'u.uid = l.entity_id');   
      $query->addField('l', 'field_last_name_value', 'last');
      $query->where("lower(concat(u.name,' ',f.field_first_name_value,' ',l.field_last_name_value))
                      LIKE lower(:q)", array(':q'=>'%'.$last_piece.'%')); 
      $result = $query->execute();

      // save the query to matches
      foreach ($result as $row) {
        $display = sprintf("%s (%s %s)", 
            check_plain($row->mail), 
            check_plain($row->first),
            check_plain($row->last));
        $key = implode(', ',$pieces);
        if (empty($key)){
          $key = $row->mail.', ';
        }else{
          $key .= ','.$row->mail.', ';
        }
        $matches[$key] = $display;
      }
      
//      foreach ($result as $row) {
//        $display = sprintf("%s  (%s %s)", 
//            check_plain($row->mail), 
//            check_plain($row->first),
//            check_plain($row->last));
//        $matches[$row->mail] = $display;
//      }
      if(empty($matches)) {
        $matches[] = t('No email addresses match: %string', array('%string' => $last_piece));
      }
    }
  }
  // Return the result to the form in json
  drupal_json_output($matches);
}


/**
 * Fetches the listing of logbooks as a 2-D array
 * [set][book] => description
 * Outputs as JSON-encoded string
 */
function logbooks_json() {
  $vid = get_taxonomy_vid_by_name('Logbooks');
  $roots = taxonomy_get_tree($vid, 0, 1, FALSE);
  $books = array();
  foreach ($roots as $r){
    $kids = taxonomy_get_children($r->tid, $vid);
    foreach ($kids as $k){
      $books[$r->name][$k->name] = $k->description;
    }
  }
  ksort($books);
  drupal_json_output($books);
}

/**
 * Retroactively add a missing tag to a log entry without having to make
 * a whole new revision.  
 * @param type $tag
 * @param type $lognumber
 */
function elog_add_tag($tag, $lognumber){ 
  $node = load_node_by_lognumber($lognumber);
  if (!isset($node->nid)){
    die("$lognumber could not be loaded");
  }
  $term = get_taxonomy_term_from_name($tag, 'Tags');
  if (isset($term) && $term !== FALSE){
    $already_tagged = FALSE;
    // The field_get_items will give us back an array of delta=>['tid'=>tid]
    $curr_tags = field_get_items('node',$node,'field_tags');
    foreach ($curr_tags as $delta => $arr){
      if ($arr['tid'] == $term->tid){
        $already_tagged = TRUE;
      }
    }
    if (! $already_tagged){
      print "must tag!";
      $new_delta = count($curr_tags);  //b.c delta began at 0
      $success = db_insert('field_data_field_tags')
        ->fields(array(
          'entity_type' => 'node',
          'bundle'    => 'logentry',
          'entity_id' => $node->nid,
          'revision_id' => $node->vid,
          'language' => $node->language,
          'delta'   => $new_delta,
          'field_tags_tid' => $term->tid,
        ))
        ->execute();
       node_load($node->nid, $node->vid, TRUE);  // flush node cache
       print "success was $success";
    }else{
      print "already tagged!";
    }
  }else{
    print "$tag is not valid tag";
  }
  die;
 
}


function elog_delete($context, $arg){
  if ($context == 'entry'){
    return elog_delete_entry($arg);
  }
  if ($context == 'range'){
    return elog_delete_range($arg);
  }
  if ($context == 'book'){
    return elog_purge_logbook($arg);
  }
  
}

/**
 * Delete a single logentry given its lognuber.
 * @param type $lognumber
 */
function elog_delete_entry($lognumber){
  $node = load_node_by_lognumber($lognumber);
  try{
    node_delete($node->nid);
    print "Success: $lognumber was deleted";
  }catch(Exception $e){
    print "Error: failed to delete $lognumber";
  }
}
/**
 * Convenience function to delete entries imported from old elog
 * so that they can be reimported (useful when debugging import tools.
 * @param type $range
 */
function elog_delete_range($range){
  if (user_access('administer nodes')){
    if (preg_match('/^(\d+)-(\d+)$/', $range, $m)){
      $first_num = $m[1];
      $last_num = $m[2];
      print "deleting from $first_num to $last_num";
      if ($m[2] < FIRST_LOGNUMBER || $_REQUEST['force']){
        // Select the nodes that we want to delete.
        // http://drupal.org/node/92861
        $node_type = 'logentry';
        $query = db_select('node', 'n')
             ->fields('n', array('nid'))
             ->condition('n.type', $node_type, '=');
         $query->join('field_data_field_lognumber', 'b', 'n.nid = b.entity_id');
         $query->condition('b.field_lognumber_value', $first_num, '>=');
         $query->condition('b.field_lognumber_value', $last_num, '<=');       
        $_SESSION['delete_count'] = 0;
        elog_batch_delete($query);
        exit(0);
     }else{
       print "\n<br> Max deletable lognumber is ".FIRST_LOGNUMBER;
     }
    }
  }else{
    print "Insufficient Privilege";
  }
  die("\n<br>done");
}

/**
 * Delete the nodes selected by the provided query
 * @param SelectQuery $query
 */
function elog_batch_delete(SelectQuery $query){
  //We batch process the deletes a few hundred at a time.
  $batch_size = 250;

  //for a technique of redirecting browser after each batch,
  //see http://jamestombs.co.uk/2008-10-27/drupal-deleting-node-content-of-a-certain-type-in-bulk/944 
  $deleted_count = 0;
  $result = $query->execute();
  $nids = array();
  foreach ($result as $record) {
    $deleted_count++;
    $nids[] = $record->nid;
    if (count($nids) > 0 && count($nids) % $batch_size == 0){
        node_delete_multiple($nids);
        $nids = array();
        $_SESSION['delete_count'] += $batch_size;
        // Reissue
        drupal_goto($_GET['q']);
    }
  }
  node_delete_multiple($nids);
  $delete_count = $_SESSION['delete_count'] += count($nids);
  print "Deleted $deleted_count entries";  
}


function elog_purge_logbook($book){
  if (strtoupper($book) == 'TLOG' && user_access('administer nodes')){
    $tid = get_logbook_tid(strtoupper($book));
    if ($tid){
      watchdog('elog', 'Purging TLOG', NULL, WATCHDOG_ERROR);
      // Select the nodes that we want to delete.
      // http://drupal.org/node/92861
      $node_type = 'logentry';
      $query = db_select('node', 'n')
           ->fields('n', array('nid'))
           ->condition('n.type', $node_type, '=');
      
       $query->join('field_data_field_logbook', 'b', 'n.nid = b.entity_id');
       $query->condition('b.field_logbook_tid', $tid, '=');
       $_SESSION['delete_count'] = 0;
       elog_batch_delete($query);
    }
  }else{
    print "$book does not exist or you do not have privilege to purge its contents";
  }
  drupal_exit();
}

/**
 * Fetches rows from the logentry_notify table where the sent column is NULL
 * and attempts to generate email for each.  If email is sent, the sent column
 * is updated with a timestamp corresponding to time email was sent.
 */
function elog_process_email_queue() {

  $query = db_select('logentry_notify', 'n')
      ->fields('n')
      ->isNull('n.sent');

  // Aggregate the emails so that all recips of a given message are on a single
  // To: header.  
  $result = $query->execute();
  $master_schedule = array();
  while ($r = $result->fetchAssoc()) {
    if ($r['cid']){
      $master_schedule[$r['nid']][$r['cid']][$r['to_addr']] = $r;
    }else{
      $master_schedule[$r['nid']][0][$r['to_addr']] = $r;
    }
  }
   
  $nodes_to_clear = array();
  
  foreach ($master_schedule as $nid=>$comment_set){
    $node = node_load($nid);
    if (! $node ) { continue; }  // Probably a node since deleted.
    foreach ($comment_set as $cid=>$recip_set){
      $to = implode(', ', array_keys($recip_set));
      // Assumption that from, prefix, etc. same for all recips of
      // a given nid, cid pair
      $data = array_shift($recip_set);
      $from = $data['from_addr'];
      $subject = $data['subject_prefix'] .' '. $node->title;
      if ($cid > 0){
        $success = elog_send_mail($to, $from, $subject, $node, $cid);
      }else{
        $success = elog_send_mail($to, $from, $subject, $node);
      }
      if ($success) {
          $nodes_to_clear[$nid] = $node;
      }else{
        $message = "Failed to send email for $nid from $from to $to";
        watchdog('elog', $message, NULL, WATCHDOG_ERROR);
      }
      
      }
    }

    foreach ($nodes_to_clear as $nid => $node){
        $updated = db_update('logentry_notify')
          ->fields(array('sent' => REQUEST_TIME))
          ->condition('nid', $nid, '=')
          ->execute();
        if (!$updated) {
          $message = "Failed to mark that node $nid was sent from $from to $to";
          watchdog('elog', $message, NULL, WATCHDOG_ERROR);
        }        
    }
}

/**
 * Sends logbook API-compliant XML representation of a specified entry
 * @param integer $nid
 * @throws Exception
 */
function node_xml_output($nid) {
  watchdog('elog', 'node_xml_output called with nid='.$nid, NULL, WATCHDOG_DEBUG);
  try {
    //ob_clean();
    if (!is_numeric($nid)) {
      throw new Exception('Invalid node id in URL request');
    }
    $node = node_load($nid);
    if (!$node) {
      throw new Exception("Node $nid does not exist");
    }
    
    if ($node->type == 'logentry') {
      $logentry = new Logentry($node);  //sets attachment_encoding to url
    }else{
      throw new Exception("Node is not of type logentry");
    }
  
    header('Content-type: text/xml; charset=UTF-8');  
    if (isset($_GET['comments'])){
      print $logentry->getXML('Logentry');
    }else{
      // Second param is list of properties to exclude from XML
      print $logentry->getXML('Logentry', array('comments'));
    }

  } catch (Exception $e) {
    watchdog('elog', $e->getMessage(), NULL, WATCHDOG_INFO);
    return MENU_NOT_FOUND;
  }
  //die;
  exit(0);
}

/**
 * Sends logbook API-compliant XML representation of a specified entry
 * @param integer $lognumber
 * @throws Exception
 * @todo is this still necessary?  Or will node_xml_output always get called
 * instead?
 */
function entry_xml_output($lognumber) {
  watchdog('elog', 'entry_xml_output called with lognumber='.$lognumber, NULL, WATCHDOG_DEBUG);
  try {
    if (!is_numeric($lognumber)) {
      throw new Exception('Invalid lognumber in URL request');
    }

    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'logentry')
        ->fieldCondition('field_lognumber', 'value', $lognumber, '=')
        ->addMetaData('account', user_load(1)); // run the query as user 1

    $result = $query->execute();
    $keys = array_keys($result['node']);
    $nid = array_shift($keys);

    $node = node_load($nid);
    if (!$node) {
      throw new Exception("Lognumber $lognumber does not exist");
    }
    node_xml_output($node->nid);
  } catch (Exception $e) {
    watchdog('elog', $e->getMessage(), NULL, WATCHDOG_INFO);
    return MENU_NOT_FOUND;
  }
//  die;
  exit(0);
}


/**
 * This function and its associated elog_menu() invocation are deprecated.
 * They were previously used to maintain the schema documents by a special cron
 * job running on dbm.  The job is now performed by taxonomy hooks in the elog
 * module that only fire when taxonomy vocab changes have actually been made.
 * 
 * Outputs the leaf-node logbook names as enumeration restrictions
 * for inclusion in the Logentry XML schema
 * Store and retrieve from cache when possible
 */
function logbooks_xsd(){
  header("Content-type: text/xml");
  print make_xsd_from_vocab('Logbooks','logbook',
      'The name of a logbook (ELOG, HCLOG, etc.)');
  exit(0);
}

/**
 * This function and its associated elog_menu() invocation are deprecated.
 * They were previously used to maintain the schema documents by a special cron
 * job running on dbm.  The job is now performed by taxonomy hooks in the elog
 * module that only fire when taxonomy vocab changes have actually been made.
 * 
 * Outputs the leaf-node tag names as enumeration restrictions
 * for inclusion in the Logentry XML schema
 * Store and retrieve from cache when possible
 */
function elog_tags_xsd(){
  header("Content-type: text/xml");
  print make_xsd_from_vocab('Tags','tag',
      'Tags used to categorize entries (Readme, Autolog, EndOfRun, etc.)');
  exit(0);
}


/**

 */
function elog_tags_admin(){
  drupal_goto('admin/structure/taxonomy/tags');
  exit(0);
}


/**
 * This function and its associated elog_menu() invocation are deprecated.
 * They were previously used to maintain the schema documents by a special cron
 * job running on dbm.  The job is now performed by taxonomy hooks in the elog
 * module that only fire when taxonomy vocab changes have actually been made.
 * 
 * @param type $vocab
 * @param type $name
 * @param type $desc
 * @return type
 */
function make_xsd_from_vocab($vocab,$name,$desc){
  $vid = get_taxonomy_vid_by_name($vocab);
  $list = taxonomy_get_tree($vid, 0, NULL, TRUE);
  $xw = new xmlWriter();
  $xw->openMemory();
  $xw->setIndent(true);
  $xw->startElementNS('xs','schema','http://www.w3.org/2001/XMLSchema');
  $xw->startElement('xs:element');
  $xw->writeAttribute('name',$name);
  
  $xw->startElement('xs:annotation');
  $xw->startElement('xs:documentation');
  $xw->text($desc);
  $xw->EndElement();
  $xw->EndElement();
  
  $xw->startElement('xs:simpleType');
  $xw->startElement('xs:restriction');
  $xw->writeAttribute('base','xs:string');
  //$list will be an array of term objects
  //We skip the terms whose names have a space,
  //treating them as hierarchy
  //mypr($list);

  foreach ($list as $term){
    if (! taxonomy_get_children($term->tid, $vid)){
      $xw->startElement('xs:enumeration');
      $xw->writeAttribute('value',$term->name);
      $xw->EndElement();
    }
  }
  $xw->EndElement();
  $xw->EndElement();
  $xw->EndElement();
  $xw->EndElement();
  return $xw->outputMemory(true);
}


/**
 * Returns just content
 * @param type $term_type
 * @param type $term
 * @return type
 * @todo remove this entire function once we go to client-side
 * full page auto-refresh
 */
function elog_refresh($term_type, $term_name=NULL){
  $content = "<em>Notice: Please refresh the entire page to continue.</em> ";
  print $content;
  exit(0);
}