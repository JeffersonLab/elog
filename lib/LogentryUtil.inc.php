<?php 
/**
* File containing LogentryUtil and supporting classes.
* @todo use elogcert with PUT to server
*/

/** Logentry root directory */
if (!defined('LOGENTRY_ROOT')) {
        define('LOGENTRY_ROOT', dirname(__FILE__) . '/');
}

require(LOGENTRY_ROOT . '/Logentry.inc.php');
require(LOGENTRY_ROOT . '/OPSPR.inc.php');
require(LOGENTRY_ROOT . '/Comment.inc.php');


/**
* LogentryUtil is a tool to assist in creating properly formatted XML files for
* the unified Jlab electronic logbook system.  It provides an object-oriented 
* interface for creating, validating and submitting the XML files.
*/
class LogentryUtil{

/**
* The filesystem queue directory
* @var string
* Should be the same on ACE and CUE
*/
protected static $queueDir = '/group/elogbooks/logentryq/new';


/**
* The filesystem temp directory
* @var string
*/
protected static $tmpDir = '/tmp';


/**
* The server for immediate submission
* Note that for now it has a self-signed certificate that requires 
* special consideration when connecting (i.e. -k flag to curl)
* @var string
*/
protected static $putServer = 'https://logbooks.jlab.org/incoming';


/**
* Stores the most recently returned message from putServer.
* @var string
*/
public static $lastServerMsg;


/**
* Instantiates with minimal required fields of a logbook and a title
*    
* @param string $type (LOGENTRY, OPS-PR, etc.)
* @return mixed Logentry or null
*/
public static function newEntry($type='LOGENTRY'){    
    if (strtoupper($type) == 'LOGENTRY'){
        return new Logentry();
    }
    return null;
}


/**
* Instantiates a Logentry Object from an XML file on disk.
*    
* @param string $filename
* @return Logentry
* @throws
*/
public static function newEntryFromXMLFile($filename){    
    $dom = new DOMDocument();
    $dom->preserveWhiteSpace = false;
    if (! $dom->load($filename, LIBXML_PARSEHUGE)){
        throw new Exception("Unable to read $filename");
    }
    $root = $dom->documentElement->tagName;
    if ($root == 'log_entry' && class_exists('ElogXMLParser')){
        $parser = new ElogXMLParser();
        $elog = $parser->load_from_file($filename);
        $logentry = new Logentry($elog);
        return $logentry;
        die("Legacy Logentry");
    }elseif ($root == 'Logentry'){
        $return = new Logentry($dom);
        return $return;
    }elseif ($root == 'OPSPR'){
        $return = new OPSPR($dom);
        return $return;    
    }elseif ($root == 'Comment'){
        $return = new Comment($dom);
        return $return;
    }else{
      throw new Exception ('XML File does not contain a valid entity');
    }    
}


/**
* Saves a Logentry object to an XML file
*    
* @param string $filename
* @param Logentry $entry
* @throws
*/
public static function saveToFile($filename, Logentry $entry){    
   if (file_put_contents($filename, $entry->getXML()) === false){
       throw new Exception("Failed to write $filename");
   }    
}


/**
* Returns a file name in the recommended format:
* YYYYMMDD_HHMMSS_PID_HOSTNAME_RND.xml
*/
public static function makeQueueFileName(){
    $pid = posix_getpid();
    $hostname = trim(`hostname`);
    $date = date('Ymd');
    $time = date('His');
    $name = sprintf("%s_%s_%s_%s_%d.xml",$date,$time,$pid,$hostname,rand(1,999));
    return $name;
}
    

/**
* Saves a Logentry's XML output to a file in the elog queue directory
* Returns the name of the file that was saved
*    
* @param Logentry $entry
* @return string
* @throws
*/
public static function saveToQueue(Logentry $entry){ 
   $filename = self::$queueDir.'/'.self::makeQueueFileName();
   while (file_exists($filename)){
       //print "\nName Collision $filename\n";
       $filename = self::$queueDir.'/'.self::makeQueueFileName();
   }
   if (file_put_contents($filename, $entry->getXML()) === false){
       throw new Exception("Failed to save $filename");
   }
    
}


/**
* Saves a Logentry's XML directly to the server for immediate submission.
* Returns boolean corresponding to server's response.  The raw server response
* is stored in LogentryUtil::$lastServerMsg
*    
* @param Logentry $entry
* @return string
* @throws
*/
public static function saveToServer(Logentry $entry, $key=NULL, $server=''){
   if ($server){
     self::$putServer = $server;
   }
   
   $basename = self::makeQueueFileName();
   $filename = self::$tmpDir.'/'.$basename;
   while (file_exists($filename)){
       //print "\nName Collision $filename\n";
       $basename = self::makeQueueFileName();
       $filename = self::$tmpDir.'/'.$basename;
   }
   if (file_put_contents($filename, $entry->getXML()) === false){
       throw new Exception("Unable to write temp file");
   }
   $FH = fopen($filename, 'r');
   $ch = curl_init ();
   curl_setopt($ch, CURLOPT_URL, self::$putServer.urlencode($basename));
   curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);  // cert is self-signed right now
   curl_setopt($ch, CURLOPT_INFILE, $FH);
   curl_setopt($ch, CURLOPT_INFILESIZE, filesize($filename));
   curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1); 
   curl_setopt($ch, CURLOPT_PUT, true);
   $result = curl_exec($ch);
   fclose($FH);
   if ($result === false){
       throw new Exception('curl failure.  Unable to send file.');
   }else{
       self::$lastServerMsg = $result;
       $success = self::interpretServerResponse($result);
       unlink($filename);
       return $success;
   }
   
}


/**
* Extracts boolean success or failure from the XML of 
* putServer response.
*
* @param $txt
* @return boolean
*/
public static function interpretServerResponse($text){
    $dom = new DOMDocument();
    if (! $dom->loadXML($text)){                // Presume garbled response is failure.
        throw new Exception("Unable to process server response:\n".$text);
        return false;     
    }
    
    $root = $dom->documentElement->tagName;
    if ($root == 'Response'){
        $stat = $dom->documentElement->getAttribute('stat');
        if ($stat == 'ok'){
            $xpath = new DOMXpath($dom);
            $nodes = $xpath->query('lognumber');
            foreach($nodes as $node){
                $lognumber = $node->nodeValue;
            }
            return $lognumber;
        }
    }else{
        throw new Exception("Server did not return Response entity in XML:\n".$text);
    }
    return false;
}



/**
* Validates the XML of the logentry against the approriate schema.
* if it returns false call getValidationErrors() to get the description
* of what tests failed.
* @param Logentry $entry
* @return boolean
* @throws
*/
public static function validateEntry(Logentry $entry, $schema){ 
    
    // We need to save the current entry to a tempfile and then
    // load it as a DOM document.
    $filename = tempnam(self::$tmpDir,'validateXML_');
    if (file_put_contents($filename, $entry->getXML()) === false){
        throw new Exception("Failed to write temp file $filename");
    }
    libxml_use_internal_errors(true);
    $dom = new DOMDocument();
    $dom->load($filename, LIBXML_PARSEHUGE);
    if ($dom->schemaValidate($schema)){
        return true;
    }
    
    return false;
}
    
/**
 * Validates the XML file against the approriate schema.
 * if it returns false you should call getValidationErrors() to get 
 * the error description
 * of what tests failed.
 * @param string $filename
 * @return boolean
 * @throws
*/
public static function validateXMLFile($filename, $schema){ 
        watchdog('elog',$schema);
    if (! is_readable($filename)){
        throw new Exception("Unable to read $filename for validation");
    }
    libxml_use_internal_errors(true);
    $dom = new DOMDocument();
    $dom->load($filename, LIBXML_PARSEHUGE );
    $root = $dom->documentElement->tagName;
    if ($dom->schemaValidate($schema)){
        return true;
    }
    return false;
}

/**
* Returns the buffered error messages from the last XML
* validation attempt and clears the buffer.
* @return string;
*/
static public function getValidationErrors(){
    $return = '';
    $errors = libxml_get_errors();
    foreach ($errors as $error) {
        $return .= self::getXMLErrorString($error);
    }
    libxml_clear_errors();
    return $return;
}


/**
* Converts a libXML error into human readable form.
* @param $error a libXML error
* @return $string
*/
static public function getXMLErrorString($error)
{
    $return = '';
    switch ($error->level) {
        case LIBXML_ERR_WARNING:
            $return .= "Warning $error->code: ";
            break;
         case LIBXML_ERR_ERROR:
            $return .= "Error $error->code: ";
            break;
        case LIBXML_ERR_FATAL:
            $return .= "Fatal Error $error->code: ";
            break;
    }

    $return .= trim($error->message) .
               "\n  Line: $error->line" .
               "\n  Column: $error->column";

    if ($error->file) {
        $return .= "\n  File: $error->file";
    }

    return "$return\n\n--------------------------------------------\n\n";
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
public static function getXMLresponse(stdClass $O, $stat='ok'){

    $xw = new xmlWriter();

    $xw->openMemory();
    $xw->startDocument();
    $xw->setIndent(true);
    $xw->startElement('Response');
    $xw->writeAttribute('stat',$stat);
    $xw->writeRaw("\n");
    //var_dump($O);
    foreach (get_object_vars($O) as $n => $var){
        $xw->writeElement($n, htmlspecialchars($var));
    }  

    $xw->endElement();
    $xw->endDocument();
    return $xw->outputMemory(true);
}

   
}// LogentryUtil
