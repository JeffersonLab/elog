<?php
/**
* File implementing Comment class.
*/
//namespace PHPElogLib;

/**
* Comment class.  
*
* Comment is the most basic type of Comment. 
*/
class Comment{


/** 
* The log number
* Comments must be associated with a Logentry.  The lognumber
* indicates which Logentry.
*
* @var integer
*/
protected $lognumber;


/** 
* The comment time
* Needs to be in ISO 8601 date format (ex: 2004-02-12T15:19:21+00:00)
* @var string 
*/
protected $created;


/** 
* The author of the logentry
* @var stdClass 
*/
protected $author;


/** 
* The optional title
* @var string 
*/
protected $title;


/** 
* The comment body
* @var string 
*/
protected $body;


/** 
* The comment body type
* Indicates the formatting of the text in $body.  
* Valid values correspond to subset of text formats defined in Drupal
* (plain_text, filtered_html, full_html, etc.)
* @var string 
*/
protected $body_type='text';


/** 
* The list of attachments for the entry
* @var array 
*/
protected $attachments = array();



/** 
* Notifications to send
* @var array 
*/
protected $notifications = array();



/**
* Instantiates a Comment
*
* Constructor is overloaded and accepts as an argument either:
*   1) A DOMDocument object in Comment.xsd format
*   2) A Lognumber and a Comment Body
* 
*/
public function __construct(){
    $args = func_get_args(); 
    if (count($args) == 1){
        if (is_a($args[0], 'DOMDocument')){
            $this->constructFromDom($args[0]);
        }
    }elseif (is_numeric($args[0]) && is_string($args[1])){
        $this->lognumber = $args[0];
        $this->body = $args[1];
    }    
    if (count($args) > 2){
        throw new Exception ("Too many arguments");
    }
}


/**
* Sets created field to current timestamp if not already set.
* Not useful for importing entries, but desirable when users are creating
* new entries.
*/
protected function checkCreated(){
    if (! $this->created){
        $this->created = date('c');
    }
}



/**
* creates a user object suitable for use as Author or an EntryMaker.
* May pass in a string to interpret as a username or as an associative array
* of additional fields defined in Entrymaker.xsd, but which must also include username.
*    example: array('username'=>foo, 'staff_id'=100)
*    
* @param mixed $userinfo
* @throws if username not provided
*/
protected function makeUser($userinfo){

    (object) $Maker = new \stdClass();

    if (is_array($userinfo)){
        foreach($userinfo as $key=>$val){
            $Maker->$key = $val;
        }
    }elseif(is_string($userinfo)){
        $Maker->username = $userinfo;
    }
    
    if (! $Maker->username){
        throw new Exception('username must be specified for an entrymaker');
    }
    
    return $Maker;
}


/**
* Accepts a DOMDocument and uses xpath queries to extract the elements
* of interest into object properties.
*
* @param DOMDocument $dom
*/
protected function constructFromDom($dom){   
    $xpath = new DOMXPath($dom);

    $query = 'lognumber';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
        $this->lognumber = $entry->nodeValue;
    }
    
    $query = 'title';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
        $this->title = $entry->title;
    }

    $query = 'created';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
        $this->created = $entry->nodeValue;
    }
    
    $query = 'Author';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      foreach($entry->childNodes as $field){
        $tmp[$field->tagName] = $field->nodeValue;
      }
      $this->setAuthor($tmp);
      //var_dump($tmp);
    }
    
    $query = 'body';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
        $this->body = $entry->nodeValue;
        $this->body_type = $entry->getAttribute('type');
    }



    $query = 'Attachments/*';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
        if ($entry->tagName == 'Attachment'){
            (object) $Attachment = new stdClass();
            foreach($entry->childNodes as $field){
                $Attachment->{$field->tagName} = $field->nodeValue;
            }
            
            $this->attachments[] = $Attachment;
        }

    }

}


/**
* Sets the internal timestamp of the entry to be astring in ISO 8601 date format 
*   ex: 2004-02-12T15:19:21+00:00 
* @param mixed $date date in a format parsable by php strtotime()
*/
public function setCreated($date){
    if (is_numeric($date)){
        $this->created = date('c',$date);
    }else{
        $this->created = date('c',strtotime($date));
    }
}

/**
* Sets the author property.
* @param string $name 
* 
*/
public function setAuthor($name){
   $Maker = $this->makeUser($name);
   $this->author = $Maker;
}


/**
* Sets the author property.
* @param string $name 
* 
*/
public function setTitle($title){
  $this->title = $title;
}

/**
* sets the body
* @param string $text The content to place in the body
* @param string $type Specifies the formatting of text: full_html, filtered_html, plain_text (the default)
*/
public function setBody($text, $type='plain_text'){
    $this->body = $text;
    $this->body_type = $type;
}


/**
* sets the lognumber
* @param integer $num 
*
*/
public function setLognumber($num){
    if (is_numeric($num)){
        $this->lognumber = $num;
    }
}


/**
* Adds an attachment.
* Stores it in the object as a base64 encoded string.
*    
* @param string $filename
* @param string $caption (defaults to filename)
* @param string $type Specify a mime_type (defaults to autodetect)
* @throws
*/
public function addAttachment($filename, $caption='', $type=''){

    (object) $Attachment = new stdClass();
    $data = file_get_contents($filename);
    if ( $data === false){
        throw new Exception ("Unable to read contents of $filename");
    }
    $Attachment->filename = basename($filename);
    $Attachment->data = base64_encode($data);

    
    if ($caption){
        $Attachment->caption = $caption;    
    }else{
        $Attachment->caption = basename($filename);
    }
    
    if ($type){
        $Attachment->type = $type;    
    }else{
        // Try to get the mime type
        $finfo = finfo_open(FILEINFO_MIME_TYPE);
        $Attachment->type = finfo_file($finfo, $filename);
    }
        
    $this->attachments[] = $Attachment;
}


/**
* Return object properties as a Comment XML DOMDocument
*
* @param string $name A name to use for the DOMElement.  If not specified, defaults to class name.
*                   
*/
function getXML($name='')
{
    if (! $name) {$name = get_class($this);}
    $xw = new xmlWriter();
    $xw->openMemory();
    $xw->setIndent(true);
    $xw->startElement($name);
    
 
    foreach (get_object_vars($this) as $n => $var)
    {
        
        // We'll do attachments after the loop so that
        // they'll go at the bottom of the xml.  This makes it easier
        // to inspect the file with command line tools.
        if ($n == 'attachments'){
           continue;   
        }

        if ($n == 'body_type'){
            continue;       //handled with body
        }
        
        if ($n == 'notifications'){
            continue;       //tbd
        }

        if ($n == 'author'){
          $xw->startElement('Author');
          foreach(get_object_vars($var) as $mprop => $mval){
                  $xw->writeElement($mprop, $mval);
          }
          $xw->endElement();
          continue;
        }

        if ($n == 'body' && $this->body != ''){
            $xw->startElement('body');
            $xw->writeAttribute('type',$this->body_type);
            $xw->writeCData($var);
            $xw->endElement();
            continue;
        }


        // Output the simple scalar properties.
        // Avoid writing empty/null tags
        if ($var !== null && is_scalar($var)){
            $xw->writeElement($n, $var);
        }
    }
    
    // Now the attachments
    if (count($this->attachments) > 0){
        $xw->startElement('Attachments');
        $count = 1;
        foreach ($this->attachments as $A){
            $xw->startElement('Attachment');
            $xw->writeElement('caption', htmlspecialchars($A->caption));
            $xw->writeElement('type', $A->type);
            $xw->writeElement('filename', htmlspecialchars($A->filename));
            $xw->writeElement('data', $A->data);
            $xw->endElement();
            $count++;
        }
        $xw->endElement();
    }

    
    $xw->endElement();
    return $xw->outputMemory(true);
}


/**
*
* This PHP5 intereceptor method allows the user to retrieve protected class properties
* in a controlled fashion.  If the class has a method named getProperty() then the output
* of that method is returned instead of the property itself. This allows us to 
* filter/restrict client access on a variable-by-variable basis without
* having to clutter the code with gratuitous get methods.
*
* @link http://us2.php.net/manual/en/language.oop5.overloading.php
*
* @param string $prop   The class property the caller tried to read.
* @return mixed $this->$prop on success, false on failure
*/

function __get($prop)
{
    // First check for a get_$prop method
    $getter = "get".ucfirst($prop);
    if (method_exists($this, $getter))
    {
        return $this->$getter($prop);
    }          
    elseif (isset($this->$prop))
    {
        return $this->$prop;
    }
    else
    {
        return false;
    }
}

}// Comment
?>