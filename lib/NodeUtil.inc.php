<?php
/**
* File containing NodeUtil class
*/

//use PHPElogLib\Logentry as Logentry;
//use \Exception;
//use \EntityFieldQuery;

/**
* A utility class for creating and saving drupal nodes out of 
* Logentry objects.
 * @todo Really need to make this admin config option.
*/
class NodeUtil {

 //Users who are allowed to set author to someone else
 //@todo make configurable
 private static $admin_users = array(
   'accadm'   => TRUE,
   'apache'   => TRUE,
   'webrun'   => TRUE,
   'ccelog'   => TRUE,
   'glassfish'=> TRUE,
   'misweb' => TRUE,
   'wildfly' => TRUE,
   'elogsoapsvc' => TRUE,
   'ssgpssdisp' => TRUE,
 );
  
/**
* Creates a logentry node from a Logentry object.
* @param Logentry $elog the source object
* @param string $submitter The username who is submitting $elog
* @throws
*/
static function newNodeFromLogentry(Logentry $elog, $submitter){

    $existing_node = NULL;
    //Check for whether the node already exists
    if (self::lognumberExists($elog->lognumber)){
        $existing_node = load_node_by_lognumber($elog->lognumber);
        //throw new Exception ("node with lognumber $elog->lognumber already exists");
    }
    
    (object) $node = new stdClass(); 
    $node->type = 'logentry';
    node_object_prepare($node); // Set some default values
    $node->language = LANGUAGE_NONE; // Or e.g. 'en' if locale is enabled
   
    $author = $elog->author->username;
    if ( isset(self::$admin_users[$submitter]) || $author == $submitter){
      $theAuthor = elog_get_registered_user($author);
    }else{
      $theAuthor = elog_get_registered_user($submitter);
      $author = $submitter;
    }
 
    if (! $theAuthor ){
      throw new Exception("$author not found in logbooks users table.");
    }
    
    if ($existing_node){
      // Verify update permission
      if (! node_access('update',$existing_node, $theAuthor)){
        throw new Exception(
            "$author does not have permission to update log $elog->lognumber"
        );
      }
      //var_dump($existing_node); die;
      $node->nid = $existing_node->nid;
      $node->revision = TRUE;
      $node->log = $elog->revision_reason;
      $node->field_lognumber[$node->language][0]['value'] = $existing_node->field_lognumber[$existing_node->language][0]['value'];
    }else{
      $node->is_new = TRUE;
    }
    //var_dump($node); die;
    $node->title    = $elog->title;
    
       
    $node->uid = $theAuthor->uid; // UID of the author of the node; or use $node->name

    $node->body[$node->language][0]['value']   = $elog->body;
    $node->body[$node->language][0]['summary'] = text_summary($elog->body);
    
    if ($account = user_load_by_name($submitter)){
      $available_formats = drupal_map_assoc(
          array_keys(filter_formats($account)));
    }else{
      $available_formats = array();
    }
    switch (strtolower($elog->body_type)){
        case 'text/plain' : 
        case 'elog_text'  : $node->body[$node->language][0]['format']  = 'elog_text';
                            break;
        case 'text/html'  : 
        case 'html'       : if(isset($available_formats['trusted_html'])){
                              $node->body[$node->language][0]['format']  = 'trusted_html';
                            }else{
                              $node->body[$node->language][0]['format']  = 'full_html';
                            }
                            break;
        case 'full_html'  : $node->body[$node->language][0]['format']  = 'full_html';
                            break;
        case 'text':        
        default           : $node->body[$node->language][0]['format']  = 'plain_text';
        
    }
    
    if ($elog->sticky){
      $node->sticky = 1;
    }elseif ($elog->sticky === 0){
      $node->sticky = 0;
    }
    
    $node->field_logbook = array();
    foreach ($elog->logbooks as $book){
        $book_name = strtoupper($book);
        $book_terms = taxonomy_get_term_by_name($book_name);
        $book_obj = array_shift($book_terms);      
        if (is_object($book_obj)){
            if ($book_obj->tid){
                $node->field_logbook[$node->language][]['tid'] = $book_obj->tid;
            }
        }
    }    

    
    foreach ($elog->entrymakers as $maker){
        $node->field_entrymakers[$node->language][]['value']   = $maker->username;
    }


    
    foreach ($elog->notifications as $email){       
        $node->field_notify[$node->language][]['value']   = $email;
    }

    
    //var_dump($elog);
    
    $node->field_tags = array();
    foreach ($elog->tags as $tag){
        //print "processing $tag \n";
        $tag_obj = array_shift(taxonomy_get_term_by_name($tag));      
        if (is_object($tag_obj)){
            if ($tag_obj->tid){
                //print "found tag id ".$tag_obj->tid."\n";
                $node->field_tags[$node->language][]['tid'] = $tag_obj->tid;
            }
        }
    }    

    $node->field_opspr = array();
    foreach ($elog->opspr_events as $event){
        $tmp = array('component_id'=>$event->component_id,
                     'action' => $event->action,
                     'problem_id' => $event->problem_id,
        );
        if (empty($node->field_opspr[$node->language])){
          $node->field_opspr[$node->language] = array();
        }
        array_push($node->field_opspr[$node->language], $tmp);
    }   
    
    $node->field_downtime = array();
    if ($elog->downtime){
      $node->field_downtime[$node->language] = array();
      foreach ($elog->downtime as $k=>$v){
        $node->field_downtime[$node->language][0][$k] = strtotime($v);
      }
    }
    
    if ($elog->created){
        //$node->date = strtotime($elog->created);
        $node->created = strtotime($elog->created);
        //print "node created is ".$node->created;
    }
    
     
    /*
     * To the eloglib API and XML schema, references are defined in
     * the same manner.  But to Drupal the are different because logbook
     * references are to records within th elocal database 
     * whereas, ATLis, etc. exist outside.
     */
    $node->field_extern_ref = array();
    foreach ($elog->references as $reftype=>$reflist){
      foreach (array_keys($reflist) as $refnum){
        switch ($reftype){
          
          case 'lognumber'  :
          case 'logbook'    : // We need to look up the Node ID and actually reference that instead
                              $node->field_references[$node->language][]['value'] = $refnum;
                              break;
          
          case 'elog'       : // Legacy OPS logbooks
          case 'dtm'        : // Downtime Manager        
          case 'atlis'      :
          case 'ctlis'      :
          case 'felist'     :
          case 'halist'     :
          case 'hblist'     :
          case 'hclist'     :
          case 'hdlist'     :
          case 'insvactl'   :  
          case 'tatl'       : $new_ref = array('ref_name'=>$reftype, 'ref_id'=>$refnum);
                              if ( empty($node->field_extern_ref)){
                                $node->field_extern_ref[$node->language] = array();
                              }
                              array_push($node->field_extern_ref[$node->language], $new_ref); 
                              break;
        }
      }
    }
   
    $i = 0;
    $j = 0;
    $node->field_image = array();
    $node->field_attach = array();
    
    foreach ($elog->attachments as $A){
      $filename = tempnam('/tmp','XMLAttached_');  
      if ($A->encoding == 'url') {
        // @todo check to make sure URI is localhost
        $data = file_get_contents($A->data);
        if (! $data){
          watchdog('elog',"Unable to retrieve attachment ".$A->data);
          continue;
        }
      }else{
        $data = base64_decode($A->data);
      }
      if (file_put_contents($filename, $data)){
        //print $filename;
        $file_path = drupal_realpath($filename);
        $file = (object) array(
                     'uid' => $theAuthor->uid,
                     'uri' => $file_path,
                     'filemime' => $A->type,
                     'status' => 1,
                     'title' => $A->caption,        //field_image
                     'description' => $A->caption,  //field_attach
                     'display' => 1,
        );

        // You can specify a subdirectory, e.g. public://foo/
        //$file = file_copy($file, 'public://'.$A->filename,FILE_EXISTS_RENAME);
        $file = file_copy($file, 'temporary://'.$A->filename,FILE_EXISTS_RENAME);           
        if (preg_match('/(image\/[\S]+)/i',$A->type, $m)){
            $node->field_image[$node->language][$i] = (array) $file;
            $i++;
        }else{
            $node->field_attach[$node->language][$j] = (array) $file;
            $j++;
        }

        }
        unlink ($filename);
    }


    if ($elog->getPR()){
        $node->problem_report = $elog->getPR();
    }

    return $node;
    
}



// Set the lognumber of a node to match what it was in elog database
// Use the Drupal DB API    
static function lognumberExists($num){
    $nids = array();
    
    // Fetch using the Entity Query
    // http://drupal.org/node/1343708
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'logentry')
    ->fieldCondition('field_lognumber', 'value', $num, '=')
    ->addMetaData('account', user_load(1)); // run the query as user 1
    
    $result = $query->execute();
    if (isset($result['node'])) {
      return true;
    }
    
    return false;
}


// Set the lognumber of a node to match what it was in elog database
// Use the Drupal DB API    
static function nidFromLognumber($num){
    $nids = array();
    
    // Fetch using the Entity Query
    // http://drupal.org/node/1343708
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', 'logentry')
    ->fieldCondition('field_lognumber', 'value', $num, '=')
    ->addMetaData('account', user_load(1)); // run the query as user 1
    
    $result = $query->execute();
    if (isset($result['node'])) {
        $found = array_shift($result['node']);
        return $found->nid;
    }
    
    return false;
}

// Set the lognumber of a node to match what it was in elog database
// Use the Drupal DB API    
static function uidFromUsername($username){
    $user = user_load_by_name($username);
    if (! $user->uid){
        throw new Exception("username $username not found");
    }
    return $user->uid;
}


/**
* Attaches an attachment object with a base64-encoded data payload
* to a drupal entity object (node, comment, etc.) as a file.
* @param $obj The drupal entity object to which file will be attached.
* @param $A The base64-style Attachment object
* @uid  The drupal uid of the file owner.
*/
function attachAttachment(&$obj, $A, $uid){


$filename = tempnam('/tmp','XMLAttached_');
$data = base64_decode($A->data);
if (file_put_contents($filename, $data)){
    //print $filename;
    $file_path = drupal_realpath($filename);
    $file = (object) array(
        'uid' => $uid,
        'uri' => $file_path,
        'filemime' => $A->type,
        'status' => 1,
        'display' => 1,
    );

    // You can specify a subdirectory, e.g. public://foo/
    $file = file_copy($file, 'public://'.$A->filename,FILE_EXISTS_RENAME);

    if (preg_match('/(image\/[\S]+)/',$A->type, $m)){
        $obj->field_image[$obj->language][] = (array) $file;
    }else{
        $obj->field_attach[$obj->language][] = (array) $file;
    }

    }
    unlink ($filename);
}

/**
 * Function returns a date-based path where a processed
 * XML file will be stored.
 * @return string absolute path
 * @throws Exception
 */
function get_logentry_subdir(){
    $pathparts = array(LOGENTRY_PROCESSED_DIR, date('Y'), date('m'), date('d'));
    $path = implode('/',$pathparts);
    if (! (is_dir($path) && is_writable($path) )){
      if (! mkdir($path,0755, true)){
        throw new Exception("Unable to make processed directory $path");
      }
    }
    return $path;
  }
  
  
} //class
