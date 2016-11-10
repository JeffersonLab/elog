<?php
//#!/usr/bin/php
/**
* Script to process logentry XML files uploaded by HTTP Put.
* Peforms the following steps:
*   1.  Saves the uploaded file to a temp directory
*   2.  Instantiates an Logentry type from it
*   3.  Attempts to save it as a node of the appropriate type
*   4.  Returns an XML message of success or failure
*      4a.  On success moves the temp file to a processed directory
*      4b.  On failure moves the temp file to an exceptions directory
*
* Example CLI invocation via curl:
*
* curl -k -T dummy.txt https://logbooks.jlab.org/incoming/dummy.txt
*
* @todo move this into a drupal module and hook into menu system
*
*/

// If the script isn't at root, some of the includes in 3rd party modules
// fail b/c they look for relative paths.
define('DRUPAL_ROOT', '/www/logbooks/html');  // autodetect?

//define('LOGENTRY_PROCESSED_DIR','/group/elogbooks/logentryq/processed');
define('MAX_PUT_FILE_SIZE',64000000);  // Max incoming file to accept (64M).
define('MAX_PUT_BODY_SIZE',8388608);  // Max body allowed (8M).


// Necessary if running from command line
$_SERVER['REMOTE_ADDR'] = "localhost"; 

// Users who are allowed to submit entries with lognumbers from the
// old elog system
$legacy_users = array(
  'accadm' => 1,
  'apache' => 1,
  'theo' => 1,
);

// Our responses are going to be XML
header("content-type:text/xml");


try{
    chdir(DRUPAL_ROOT);
    require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
    require_once(DRUPAL_ROOT .'/includes/common.inc');
    require_once(DRUPAL_ROOT .'/includes/path.inc');
    drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

    // If Drupal is in maintenance mode, we can't accept incoming
    // entries.
    if (variable_get('maintenance_mode', 0)) {
      $msg = "The server is undergoing maintenance ";
      $msg .="and cannot accept submissions at this time."; 
      throw new Exception($msg);
    }
    
    
    /***********************************
    /* Note that the drupal_bootstrap above will
    /* have included the LogentryUtil and NodeUtil
    /* libraries for us out of the elog module lib directory.
    /* require_once('LogentryUtil.php');
    /* require_once('NodeUtil.inc.php');
    /***********************************/   

    $R = new stdClass();

    /* PUT data comes in on the input stream */
    $putdata = fopen("php://input", "r");

    /* Open a file for writing */
    $pinfo = pathinfo(rawurldecode($_SERVER['REQUEST_URI']));
    $basename = $pinfo['basename'];
    
    $outfile = '/tmp/'.$basename;
    if (file_exists($outfile)){
        throw new Exception("A file named $basename has already been accepted.");
    }

    $processed_file = get_processed_subdir().'/'.$basename;
    if (file_exists($processed_file)){
        throw new Exception("A file named $basename has already been processed.");
    }
    
    $fp = fopen("$outfile", "w");

    if ($fp === false){
        throw new Exception("Unable to save file.");
    }

    /* Read the data 1 KB at a time
       and write to the file */
    while ($data = fread($putdata, 1024)){
      fwrite($fp, $data);
    }

    /* Close the streams */
    fclose($fp);
    fclose($putdata);

    if (filesize($outfile) > MAX_PUT_FILE_SIZE){
        unlink ($outfile);
        throw new Exception("XML File exceeds max length (in bytes) of ".MAX_PUT_FILE_SIZE);
    }
    
    // Now SSL client inspection & verification steps
    $submitter = elog_get_ssl_client();
    $serial = elog_get_ssl_serial();    
    if (!elog_ssl_serial_is_valid($serial)){
       watchdog('elog', 'Rejected XML file posted with revoked SSL certificate @serial and common name @cn', 
           array('@serial'=>$serial, '@cn' => $submitter), WATCHDOG_NOTICE);
       throw new Exception('The SSL client certificate has been revoked.');
    }else{
       watchdog('elog', 'XML file posted with SSL certificate @serial with common name @cn', 
           array('@serial'=>$serial, '@cn' => $submitter), WATCHDOG_INFO);
    }
    
    
    //Validate the xml file against the schema
    if (! LogentryUtil::validateXMLFile($outfile, variable_get('elog_schema'))){
      throw new Exception(LogentryUtil::getValidationErrors());
    }
    $message = "memory usage before newEntry is ".memory_get_usage(true);
    //watchdog('elog', $message, NULL, WATCHDOG_DEBUG);

    $obj = LogentryUtil::newEntryFromXMLFile($outfile);
    
    $message = "memory usage after newEntry is ".memory_get_usage(true);
    //watchdog('elog', $message, NULL, WATCHDOG_DEBUG);

    if (is_a($obj, 'Comment')){
        $R = save_comment($obj);
    }
    if (is_a($obj, 'Logentry')){
        $R = save_logentry($obj, $submitter);
    }
    if (is_a($obj, 'OPSPR')){
        throw new Exception("OPSPR is not currently accepted.");
    }
    print getXMLResponse($R);    
    $success = true;    
}catch(Exception $e){
    
    $R->msg = $e->getMessage();
    print getXMLResponse($R, 'fail');
    $success = null;
}

if ($success){
  if (! rename($outfile, $processed_file)){
    error_log("Failed to move processed file $outfile to $processed_file");
  }
  
}else{
  if (isset($outfile) && file_exists($outfile)){
    unlink($outfile);
  }
}

die;   // end processing


//---------------------

/**
 * 
 * @global type $is_https
 * @param Logentry $elog
 * @return type
 * @todo Handle update/revision to existing entry.
 */
function save_logentry($elog, $submitter){
    global $is_https; //defined by drupal core
    global $user;     //defined by drupal core
    global $legacy_users;
    
    
    $R = new stdClass();
    
    
    if ($elog->body > MAX_PUT_BODY_SIZE){
      throw new Exception ("Body field is too large.  Max bytes = ".MAX_PUT_BODY_SIZE);
    }
    
   
    //$message = "memory usage before newNode is ".memory_get_usage(true);
    //watchdog('elog', $message, NULL, WATCHDOG_DEBUG);
    $node = NodeUtil::newNodeFromLogentry($elog, $submitter); 
    
    //$message = "memory usage a newNode is ".memory_get_usage(true);
    //watchdog('elog', $message, NULL, WATCHDOG_DEBUG);
     
    // Set the effective user to the submitter so that revisions get
    // attributed properly
    if ($node->uid){
      if ($node->uid == 1){
        throw new Exception("User admin may not submit entries remotely");
      }
      $user = user_load($node->uid);
    }
    
    if($node = node_submit($node)) { // Prepare node for saving
  
       //$message = "memory usage after node_submit is ".memory_get_usage(true);
       //watchdog('elog', $message, NULL, WATCHDOG_DEBUG);
      
        // The node_submit above screws up our date!
        // @todo prevent revision being newer than prior version?
        if ($elog->created){
            $node->created = strtotime($elog->created);
        }

        //$message = "lognumber before node_save is ".$elog->lognumber;
        //watchdog('elog', $message, NULL, WATCHDOG_DEBUG);
        if ( ($elog->lognumber > 100) && ($elog->lognumber < FIRST_LOGNUMBER)){          
          if ($legacy_users[$submitter]){
            watchdog('elog', "Setting lognumber to $elog->lognumber for legacy data", NULL, WATCHDOG_DEBUG);
            $node->field_lognumber[$node->language][0]['value'] = $elog->lognumber;
            //$node->lognumber = $elog->lognumber;
            $node->changed = $node->created;
          }else{
            throw new Exception("$submitter is not allowed to submit legacy entries");
          }
        }    
        node_save($node);
        
        
        $R->msg = "Entry saved.";
        $R->lognumber = $node->field_lognumber[$node->language][0]['value'];
        
        if ($node->field_lognumber[$node->language][0]['value'] < FIRST_LOGNUMBER ){
          elog_fix_date($node);
        }
        
        if ($is_https){     //drupal global boolean
            $R->url = 'https://'.$_SERVER['HTTP_HOST'] . '/' . drupal_lookup_path('alias', 'node/'.$node->nid);
        }else{
            $R->url = 'http://'.$_SERVER['HTTP_HOST'] . '/' . drupal_lookup_path('alias', 'node/'.$node->nid);
        }
        
    }else{
        $R->msg = "Failed to save node!";
    }
    return $R;
}


function save_comment($comment){     
    global $is_https; //defined by drupal core
    global $user;     //defined by drupal core
    
    $nid = NodeUtil::nidFromLognumber($comment->lognumber);
    if (! $nid){
        throw new Exception("Lognumber ".$comment->lognumber."not found.");
    }

    $uid = NodeUtil::uidFromUsername($comment->author->username);
    // @see http://timonweb.com/how-programmatically-create-nodes-comments-and-taxonomies-drupal-7
    $drupal_comment = new stdClass(); 
    $drupal_comment->nid = $nid; // nid of a node you want to attach a comment to
    $drupal_comment->cid = 0; // leave 0 for new comment
    $drupal_comment->pid = 0; // parent comment id, 0 if none
    $drupal_comment->status = COMMENT_PUBLISHED;
    $drupal_comment->uid = $uid; // user's id, who left the comment
    $drupal_comment->is_anonymous = 0; //false
    $drupal_comment->language = LANGUAGE_NONE; // The same as for a node
    $drupal_comment->subject = 'Comment on Logentry';
    $drupal_comment->comment_body[$drupal_comment->language][0]['value'] = $comment->body; //
    
    foreach ($comment->attachments as $A){
        NodeUtil::attachAttachment($drupal_comment, $A, $uid);
    }
    
    comment_save($drupal_comment);
    
    $R->msg = "Comment saved.";
    $R->lognumber = $comment->lognumber;
    if ($is_https){     //drupal global boolean
        $R->url = 'https://'.$_SERVER['HTTP_HOST'] . '/' . drupal_lookup_path('alias', 'node/'.$nid);
    }else{
        $R->url = 'http://'.$_SERVER['HTTP_HOST'] . '/' . drupal_lookup_path('alias', 'node/'.$nid);
    }

    //Set up notifications (if any) that were requested in the Logentry
    //@todo the comment should have a different body - the comment text.
    //schedule_email($elog, $R->lognumber);
    
    return $R;
}


/**
* returns an XML Response entity.
*
* The XML of the wrapped inside <Response> tags like so
* <Response stat="ok">
*   <lognumber>123456</lognumber>
*   <url>https://logboks.jlab.org/entry/123456</url>
*   <msg />
* </Response>
*
* @param stdClass $O object containing data to return
* @param string $stat status to return (ok|fail)
*/
function getXMLresponse(stdClass $O, $stat='ok'){
    return LogentryUtil::getXMLResponse($O, $stat);
}


/**
Looks for a drupal path alias, if it's not there, then it returns node/[nid]
 */
function getPathAlias($nid){

    global $base_url;
    //check for an alias using drupal_lookup_path()
    if((drupal_lookup_path('alias', 'node/'.$nid)!==false))
        $alias = drupal_lookup_path('alias', 'node/'.$nid); 
    return $base_url.'/'.$alias;
}

/**
 * Function returns a date-based path where a processed
 * XML file will be stored.
 * @return string absolute path
 * @throws Exception
 */
function get_processed_subdir(){
    $procdir = variable_get('elog_processed_dir');
    $pathparts = array(variable_get('elog_processed_dir'), date('Y'), date('m'), date('d'));
    $path = implode('/',$pathparts);
    
    if ( is_dir($path) && is_writable($path) ){
      return $path;
    }else if ( mkdir($path,0755, true)){
        return $path;
    }   
    throw new Exception("Unable to make processed directory $path");
    
  }

/**
 * Returns the Common Name field extracted of the curent SSL client.
 * 
 * @note requires the apache setting
 *   SSLOptions +StdEnvVars
 * @return string 
 * @throws Exception if SSL data not available from server
 */
function elog_get_ssl_client(){ 
    // Don't understand why/when the server distinquishes
    // between the two StdEnvVar names
    if ( isset($_SERVER['SSL_CLIENT_S_DN_CN'])){
      return $_SERVER['SSL_CLIENT_S_DN_CN'];
    }
    if ( isset($_SERVER['REDIRECT_SSL_CLIENT_S_DN_CN'])){
      return $_SERVER['REDIRECT_SSL_CLIENT_S_DN_CN'];
    }
    throw new Exception("Unable to extract submitter's identity from SSL certificate");
}
 
/**
 * Returns true if the Serial number of the current SSL client is not
 * flagged as revoked.
 * 
 * @note requires the apache setting
 *   SSLOptions +StdEnvVars
 * 
 * @return string
 * @throws Exception if SSL data not available from server
 */
function elog_get_ssl_serial(){
    // Don't understand why/when the server distinquishes
    // between the two StdEnvVar names
    if ( isset($_SERVER['SSL_CLIENT_M_SERIAL'])){
      return $_SERVER['SSL_CLIENT_M_SERIAL'];
    }
    if ( isset($_SERVER['REDIRECT_SSL_CLIENT_M_SERIAL'])){
      return $_SERVER['REDIRECT_SSL_CLIENT_M_SERIAL'];
    }
   throw new Exception("Unable to extract serial number from SSL certificate"); 
}

/**
 * Answers whether the current SSL client certificate has a serial number that
 * is on the revoked serials list.
 * 
 * @return boolean
 */
function elog_ssl_serial_is_valid($serial){
    $revoked = variable_get('elog_certs_revoked_serials',array());
    if ($serial && ! in_array($serial, $revoked)){
      return TRUE;
    }   
    return FALSE;    
}

/**
 * Set Posted to the original elog date
 * This is intended to support importing legacy entries
 * 
 * @param type $node
 */
  
function elog_fix_date($node){
    $num_updated = db_update('node') 
      ->fields(array(
        'changed' => $node->created,
      ))
      ->condition('nid', $node->nid, '=')
      ->execute();
}
?>