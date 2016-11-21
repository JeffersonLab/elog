<?php

/**
 * File implementing Logentry class.
 */

if (!defined('EMAIL_DOMAIN')) {
  define('EMAIL_DOMAIN', '@jlab.org');
}

/**
 * Logentry class.  
 *
 * Logentry is the most basic type of logentry.
 */
class Logentry {

  /**
   * The log number
   * This number is assigned by the database and null for an entry
   * being submitted for the first time.  It will be set when submitting
   * a revision.
   *
   * @var integer
   */
  protected $lognumber;

  /**
   * The log entry title.  
   * 255 character limit imposed by Drupal database.
   * @var string 
   */
  protected $title;

  /**
   * The author of the logentry
   * @var stdClass 
   */
  protected $author;

  /**
   * The log entry time
   * Needs to be in ISO 8601 date format (ex: 2004-02-12T15:19:21+00:00)
   * @var string 
   */
  protected $created;

  /**
   * The log entry body
   * @var string 
   */
  protected $body;

  /**
   * Whether the logentry should be sticky at the top of lists
   * @var integer (0/1 representing boolean)
   */
  protected $sticky;

  /**
   * The log entry body type
   * Indicates the formatting of the text in $body.  
   * Valid values correspond to subset of text formats defined in Drupal
   * (plain_text, filtered_html, full_html, etc.)
   * @var string 
   */
  protected $body_type;

  /**
   * The list of logbooks for the entry
   * @var array 
   */
  protected $logbooks = array();

  /**
   * The list of attachments for the entry
   * @var array 
   */
  protected $attachments = array();

  /**
   * The list of users credited with making the entry
   * @var array of stdClass
   */
  protected $entrymakers = array();

  /**
   * The list of tags associated with the log entry.
   * Must be valid terms from the tags vocabulary
   * @var array 
   */
  protected $tags = array();

  /**
   * The possible list of opspr events
   * @var array 
   */
  protected $opspr_events = array();

  /**
   * The object that holds fields of a new-style ProblemReport (PR)
   */
  protected $pr;

  /**
   * The possible downtime fields
   * @var array 
   */
  protected $downtime = array();

  /**
   * References to external databases
   * @var array [type][]=>[id]
   */
  protected $references = array();

  /**
   * Notifications to send
   * @var array 
   */
  protected $notifications = array();

  /**
   * Comments attached to the Logentry
   * @var array of Comments
   */
  protected $comments = array();

  /**
   * Text to log as reason for a new revision
   */
  protected $revision_reason;

  /**
   * Instantiates a logentry
   *
   * Constructor is overloaded and accepts as an argument either:
   *   1) A DOMDocument object in Logentry.xsd format
   *   2) An Elog object from the current elog system
   *   4) A string to be the title of a new logentry
   * 
   */
  public function __construct() {
    $args = func_get_args();
    if (count($args) == 1) {

      if (is_a($args[0], 'DOMDocument')) {
        $this->constructFromDom($args[0]);
      } elseif (is_a($args[0], 'DOMElement')) {
        //Must convert to a DOMDocument in order to use DOMXpath
        $dom = new DOMDocument('1.0', 'UTF-8');
        $dom->appendChild($dom->importNode($args[0], TRUE));
        //echo $dom->saveXML();
        $this->constructFromDom($dom);
      } elseif (is_a($args[0], 'Elog')) {
        $this->constructFromElog($args[0]);
      } elseif (is_a($args[0], 'StdClass')) {
        $this->constructFromNode($args[0]);
      } elseif (is_string($args[0])) {
        $this->setTitle($args[0]);
        $this->setCreated(mktime());
      }
    } elseif (count($args) == 2) {
      if (is_a($args[0], 'StdClass')) {
        $this->constructFromNode($args[0], $args[1]);
      }
    }
    if (count($args) > 2) {
      throw new Exception("Too many arguments");
    }
  }

  /**
   * Adds the unix uid running the process to the entrymakers list.
   * Not useful during import of old entries, but desirable in production usage.
   */
  protected function checkEntrymakers() {

    if (count($this->entrymakers) < 1) {
      $os_user = posix_getpwuid(posix_getuid());
      $this->addEntrymaker(array(
        'username' => $os_user['name'],
        'uid' => $os_user['uid']
      ));
    }
  }

  /**
   * Sets created field to current timestamp if not already set.
   * Not useful for importing entries, but desirable when users are creating
   * new entries.
   */
  protected function checkCreated() {
    if (!$this->created) {
      $this->created = date('c');
    }
  }

  /**
   * Accepts a DOMDocument and uses xpath queries to extract the elements
   * of interest into object properties.
   *
   * @param DOMDocument $dom
   */
  protected function constructFromDom($dom) {

    // Check if we were handed old-style XML
    $root = $dom->documentElement->tagName;
    if ($root == 'log_entry') {
      return $this->constructFromLegacyDom($dom);
    }

    $xpath = new DOMXPath($dom);

    $query = 'Logbooks/*';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      if ($entry->nodeName == 'logbook') {
        $this->logbooks[strtoupper($entry->nodeValue)] = $entry->nodeValue;
      }
    }

    $query = 'lognumber';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->lognumber = $entry->nodeValue;
    }

    $query = 'revision_reason';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->setRevisionReason($entry->nodeValue);
    }

    $query = 'title';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->title = $entry->nodeValue;
    }

    $query = 'Author';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      foreach ($entry->childNodes as $field) {
        $tmp[$field->tagName] = $field->nodeValue;
      }
      $this->setAuthor($tmp);
    }

    if (module_exists('elog_pr')) {
      $query = 'ProblemReport';
      $entries = $xpath->query($query);
      foreach ($entries as $entry) {
        $this->pr = new ElogPR(array(), 'elog_pr');
        $this->pr->constructFromDom($entry);
      }
    }



    $query = 'sticky';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->setSticky($entry->nodeValue);
    }


    $query = 'created';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->created = $entry->nodeValue;
    }



    $query = 'body';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->body = $entry->nodeValue;
      $this->body_type = $entry->getAttribute('type');
      if (!$this->body_type) {
        $this->body_type = 'text';
      }
    }

    $query = 'Entrymakers/*';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      if ($entry->tagName == 'Entrymaker') {
        $tmp = array();
        foreach ($entry->childNodes as $field) {
          $tmp[$field->tagName] = $field->nodeValue;
        }
        $this->addEntryMaker($tmp);
      }
    }

    $query = 'References/*';
    $references = $xpath->query($query);
    foreach ($references as $ref) {
      if ($ref->tagName == 'reference') {
        $refType = $ref->getAttribute('type');
        $value = $ref->firstChild->nodeValue;
        $this->addReference($refType, $value);
      }
    }


    $query = 'Tags/*';
    $tags = $xpath->query($query);
    foreach ($tags as $tag) {
      if ($tag->tagName == 'tag') {
        $value = $tag->firstChild->nodeValue;
        $this->addTag($value);
      }
    }

    $query = 'Notifications/*';
    $notifications = $xpath->query($query);
    foreach ($notifications as $notify) {
      if ($notify->tagName == 'email') {
        $value = $notify->firstChild->nodeValue;
        $this->addNotify($value);
      }
    }

    $query = 'Attachments/*';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      if ($entry->tagName == 'Attachment') {
        (object) $Attachment = new stdClass();
        foreach ($entry->childNodes as $field) {
          $Attachment->{$field->tagName} = $field->nodeValue;
          if ($field->tagName == 'data') {
            $Attachment->encoding = $field->getAttribute('encoding');
          }
        }

        $this->attachments[] = $Attachment;
      }
    }

    $query = 'OPSPREvents/*';
    $events = $xpath->query($query);
    foreach ($events as $event) {
      $pr = new stdClass();
      foreach ($event->childNodes as $field) {
        $key = $field->tagName;
        $value = $field->nodeValue;
        $pr->$key = $value;
      }
      $this->opspr_events[] = $pr;
    }

    $query = 'Downtime/*';
    $times = $xpath->query($query);
    $downtime = array();
    foreach ($times as $time) {
      $key = $time->tagName;
      $value = $time->nodeValue;
      $downtime[$key] = $value;
    }
    $this->downtime = $downtime;
  }

  /**
   * Accepts a DOMDocument containing XML of the sort used in OPS logbook
   * system from 2000-2013 and uses xpath queries to extract the elements
   * of interest into object properties.
   *
   * @param DOMDocument $dom
   */
  protected function constructFromLegacyDom($dom) {

    $xpath = new DOMXPath($dom);

    // The type field would be NEWTS or OPS-PR
    $entry_type = $dom->documentElement->getAttribute('type');

    $query = 'logbook';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      if ($entry->nodeName == 'logbook') {
        $this->addLogbook($entry->nodeValue);
      }
    }

    $query = 'title';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->setTitle($entry->nodeValue);
    }

    $query = 'os_user';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->setAuthor($entry->nodeValue);
    }

    $query = 'log_user';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->addEntryMaker($entry->nodeValue);
    }

    $query = 'elog_id';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->lognumber = $entry->nodeValue;
    }

    $query = 'timestamp';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->setCreated($entry->nodeValue);
    }

    $query = 'priority';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $priority = $entry->nodeValue;
      if (strtoupper($priority) == 'README' || strtoupper($priority) == 'VIP') {
        $this->addTag('Readme');
      }
    }

    $query = 'text';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $body = $entry->nodeValue;
      // Map mime-type to drupal text format name
      // Note that the way "text" entries were presented in the old
      // logbook was really as html, but wrapped inside of pre tag
      switch (strtolower($entry->getAttribute('type'))) {
        case 'text/html' : $body_type = 'html';
          $this->body = $body;
          break;
        case 'text/plain' :
        default : $body_type = 'html';
          $this->body = '<pre class="elogtext">' . $body . '</pre>';
          break;
      }
      $this->body_type = $body_type;
    }

    $query = 'attachment';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $mime_type = $entry->getAttribute('type');
      $caption = $entry->getAttribute('name');
      $this->addAttachment($entry->nodeValue, $caption, $mime_type);
    }


    $query = 'reference';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->addReference('elog', $entry->nodeValue);
    }


    $query = 'pssref';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->addReference('psslog', $entry->nodeValue);
    }


    $query = 'notify';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
      $this->addNotify($entry->nodeValue);
    }


    //Optional type-specific fields
    //Does dtlite always use NEWTS?
    //@todo do we need to bother with area_id?
    if ($entry_type == 'NEWTS' || $entry_type == 'OPS-PR') {
      $pr = new stdClass();
      $nodes = $xpath->query('newts_component');
      if ($nodes->length > 0) {
        $pr->component_id = $nodes->item(0)->nodeValue;
      }
      $nodes = $xpath->query('newts_problem');
      if ($nodes->length > 0) {
        $pr->problem_id = $nodes->item(0)->nodeValue;
      }
      $nodes = $xpath->query('newts_action');
      if ($nodes->length > 0) {
        $pr->action = $nodes->item(0)->nodeValue;
      }
      $this->opspr_events[] = $pr;
    }
  }

  /**
   * Sets the sticky property.
   * @param mixed $val 
   * Sticky entries stay floated at the top of lists.
   */
  public function setSticky($val) {
    if ($val) {
      $this->sticky = 1;
    } else {
      $this->sticky = 0;
    }
  }

  public function setPR(ElogPR $pr) {
    $this->pr = $pr;
  }

  /**
   * Sets the author property.
   * @param string $name 
   * 
   */
  public function setAuthor($name) {
    $Maker = $this->makeUser($name);
    $this->author = $Maker;
  }

  /**
   * Sets the reason for a new revision
   * @param string $name 
   * 
   */
  public function setRevisionReason($str) {
    $this->revision_reason = $str;
  }

  /**
   * Sets the internal timestamp of the entry to be astring in ISO 8601 date format 
   *   ex: 2004-02-12T15:19:21+00:00 
   * @param mixed $date date in a format parsable by php strtotime()
   */
  public function setCreated($date) {
    if (is_numeric($date)) {
      $this->created = date('c', $date);
    } else {
      $this->created = date('c', strtotime($date));
    }
  }

  /**
   * Adds a logbook association
   * @see https://logbooks.jlab.org/logbooks
   * @param string $logbook
   */
  public function addLogbook($logbook) {
    if (is_string($logbook)) {
      $bookName = strtoupper($logbook);
      $this->logbooks[$bookName] = $bookName;
    }
  }

  /**
   * Adds an email adress to notify of the logentry
   * @param string $logbook
   */
  public function addNotify($email) {
    if (is_string($email)) {
      $addr = strtolower($email);
      if (!stristr($email, '@')) {
        $email .= EMAIL_DOMAIN;
      }
      $this->notifications[$addr] = $email;
    }
  }

  /**
   * Adds a reference to external data key
   * @param string $type (lognumber, atlis, etc.)
   * @param integer $ref (numeric elog_id, task_id, etc.)
   */
  public function addReference($type, $ref) {
    if ($type && $ref) {
      $key = strtolower($type);
      $this->references[$key][$ref] = $ref;
    }
  }

  /**
   * Adds a tag
   * A term from the tags vocabulary
   * @see https://logbooks.jlab.org/tags
   * @param string $tag
   */
  public function addTag($tag) {
    if ($tag) {
      $key = strtolower($tag);
      $this->tags[$key] = $tag;
    }
  }

  /**
   * Sets the title
   * Limited to 255 characters.  
   * @param string $title
   * @throws if title too long
   */
  public function setTitle($title) {
    // Would it be kinder to simply truncate?
    if (strlen($title) > 255) {
      throw new Exception("Title exceeds limit of 255 characters");
    }
    $this->title = $title;
  }

  /**
   * sets the body
   * @param string $text The content to place in the body
   * @param string $type Specifies the formatting of text: full_html, filtered_html, plain_text (the default)
   */
  public function setBody($text, $type = 'plain_text') {
    $this->body = $text;
    $this->body_type = $type;
  }

  /**
   * gets the body
   * @return string
   */
  public function getBody() {
    return $this->body;
  }

  /**
   * gets the pr
   * @return string
   */
  public function getPR() {
    return $this->pr;
  }

  public function getBodyType() {
    return $this->body_type;
  }

  /**
   * sets the lognumber
   * @param integer $num 
   *
   */
  public function setLognumber($num) {
    if (is_numeric($num)) {
      $this->lognumber = $num;
    }
  }

  /**
   * Adds an entry maker.
   * May pass in a string to interpret as a username or as an associative array
   * of additional fields defined in Entrymaker.xsd, but which must also include username.
   *    example: array('username'=>foo, 'staff_id'=100)
   *    
   * @param mixed $userinfo
   * @throws if username not provided
   */
  public function addEntryMaker($userinfo) {
    $Maker = $this->makeUser($userinfo);
    $this->entrymakers[$Maker->username] = $Maker;
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
  protected function makeUser($userinfo) {

    (object) $Maker = new stdClass();

    if (is_array($userinfo)) {
      foreach ($userinfo as $key => $val) {
        $Maker->$key = $val;
      }
    } elseif (is_string($userinfo)) {
      $Maker->username = $userinfo;
    }

    if (!$Maker->username) {
      throw new Exception('username must be specified for an entrymaker');
    }

    return $Maker;
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
  public function addAttachment($filename, $caption = '', $type = '') {

    (object) $Attachment = new stdClass();
    $Attachment->encoding = 'base64';
    $data = file_get_contents($filename);
    if ($data === false) {
      throw new Exception("Unable to read contents of $filename");
    }
    $Attachment->filename = basename($filename);
    $Attachment->data = base64_encode($data);


    if ($caption) {
      $Attachment->caption = $caption;
    } else {
      $Attachment->caption = basename($filename);
    }

    if ($type) {
      $Attachment->type = $type;
    } else {
      // Try to get the mime type
      $finfo = finfo_open(FILEINFO_MIME_TYPE);
      $Attachment->type = finfo_file($finfo, $filename);
    }

    $this->attachments[] = $Attachment;
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
  public function addAttachmentURL($url, $caption = '', $type = '') {

    (object) $Attachment = new stdClass();
    $Attachment->encoding = 'url';
    $Attachment->filename = urldecode(basename($url));
    $Attachment->data = $url;

    if ($caption) {
      $Attachment->caption = $caption;
    } else {
      $Attachment->caption = $Attachment->filename;
    }

    if ($type) {
      $Attachment->type = $type;
    } else {
      $Attachment->type = '';
    }
    $this->attachments[] = $Attachment;
  }

  /**
   * Return object properties as a Logentry XML DOMDocument
   * Note that calls to xmlWriter::writeElement seem to implicitly encode
   * html entitites and so we don't want to call htmlspecialchars ourselves
   * because that results in double-encoding and is a problem.
   *
   * @param string $name A name to use for the DOMElement.  defaults to class name.
   * @param array $excludes array of elements to exclude
   *                   
   */
  function getXML($name = 'Logentry', $excludes = NULL) {
    if (!is_array($excludes)) {
      $exclude = array();
    }
    $xw = new xmlWriter();
    $xw->openMemory();
    $xw->setIndent(true);
    $xw->startElement($name);


    foreach (get_object_vars($this) as $n => $var) {
      if ($excludes && in_array($n, $excludes)) {
        continue;
      }

      // We'll do attachments after the loop so that
      // they'll go at the bottom of the xml.  This makes it easier
      // to inspect the file with command line tools.
      if ($n == 'attachments') {
        continue;
      }

      if ($n == 'logbooks') {
        $xw->startElement('Logbooks');
        foreach ($var as $logbook) {
          $xw->writeElement('logbook', $logbook);
        }
        $xw->endElement();
        continue;
      }

      if ($n == 'tags' && count($this->tags) > 0) {
        $xw->startElement('Tags');
        foreach ($var as $tag) {
          $xw->writeElement('tag', $tag);
        }
        $xw->endElement();
        continue;
      }


      if ($n == 'entrymakers' && count($this->entrymakers) > 0) {
        $xw->startElement('Entrymakers');
        foreach ($var as $maker) {
          $xw->startElement('Entrymaker');
          foreach (get_object_vars($maker) as $mprop => $mval) {
            if ($mval) {
              $xw->writeElement($mprop, $mval);
            }
          }
          $xw->endElement();
        }
        $xw->endElement();
        continue;
      }

      if ($n == 'author') {
        $xw->startElement('Author');
        foreach (get_object_vars($var) as $mprop => $mval) {
          $xw->writeElement($mprop, $mval);
        }
        $xw->endElement();
        continue;
      }

      if ($n == 'body_type') {
        continue;       //handled with body
      }

      if ($n == 'notifications' && count($this->notifications) > 0) {
        $xw->startElement('Notifications');
        foreach ($var as $email) {
          $xw->writeElement('email', $email);
        }
        $xw->endElement();
        continue;
      }

      if ($n == 'comments' && count($this->comments) > 0) {
        $xw->startElement('Comments');
        //$xw->writeRaw("\n");
        foreach ($var as $comment) {
          if (method_exists($comment, 'getXML')) {
            $xw->writeRaw($comment->getXML());
          }
        }
        $xw->endElement();
        continue;
      }

      if ($n == 'opspr_events' && count($this->opspr_events) > 0) {
        $xw->startElement('OPSPREvents');
        foreach ($this->opspr_events as $pr_event) {
          $xw->startElement('OPSPREvent');
          foreach (get_object_vars($pr_event) as $mprop => $mval) {
            $xw->writeElement($mprop, $mval);
          }
          $xw->endElement();
        }
        $xw->endElement();
        continue;
      }

      if ($n == 'downtime' && !empty($this->downtime)) {
        $xw->startElement('Downtime');
        foreach ($this->downtime as $key => $value) {
          $xw->writeElement($key, $value);
        }
        $xw->endElement();
        continue;
      }

      if ($n == 'references' && count($this->references) > 0) {
        $xw->startElement('References');
        foreach ($this->references as $type => $ref) {
          foreach ($ref as $r) {
            $xw->startElement('reference');
            $xw->writeAttribute('type', $type);
            $xw->text($r);
            $xw->endElement();
          }
        }
        $xw->endElement();
        continue;
      }


      if ($n == 'body' && $this->body != '') {
        $xw->startElement('body');
        switch ($this->body_type) {
          case 'elog_text' : //Really was HTML
          case 'text/html' :
          case 'html' :
          case 'trusted_html' :
          case 'full_html' : $body_type = 'html';
            break;
          default : $body_type = 'text';
        }
        $xw->writeAttribute('type', $body_type);
        $xw->writeCData($var);
        $xw->endElement();
        continue;
      }


      if ($n == 'pr'  && is_a($this->pr, 'ElogPR' )) {
        $xw->writeRaw($this->pr->getXML());
      }


      // Output the simple scalar properties.
      // Avoid writing empty/null tags
      if ($var !== null && is_scalar($var)) {
        $xw->writeElement($n, $var);
      }
    }

    // Now the attachments
    if (count($this->attachments) > 0) {
      $xw->startElement('Attachments');
      $count = 1;
      foreach ($this->attachments as $A) {
        $xw->startElement('Attachment');
        $xw->writeElement('caption', $A->caption);
        $xw->writeElement('type', $A->type);
        $xw->writeElement('filename', $A->filename);
        $xw->startElement('data');
        $xw->writeAttribute('encoding', $A->encoding);
        $xw->text($A->data);
        $xw->endElement();
        $xw->endElement();
        $count++;
      }
      $xw->endElement();
    }




    $xw->endElement();
    return $xw->outputMemory(true);
  }

  /**
   * Accepts a legacy Elog object from which to construct
   * properties.
   */
  protected function constructFromElog(Elog $E) {

    if ($E->elog_id > 0) {
      $this->lognumber = $E->elog_id;
    }
    $this->setTitle($E->title);

    // Map mime-type to drupal text format name
    // Note that the way "text" entries were presented in the old
    // logbook was really as html, but wrapped inside of pre tag
    switch (strtolower($E->text_type)) {
      case 'text/html' : $drupal_type = 'html';
        $body = $E->text;
        break;
      case 'text/plain' :
      default : $drupal_type = 'html';
        $body = '<pre class="elogtext">' . $E->text . '</pre>';
        break;
    }

    $this->setBody($body, $drupal_type);

    foreach ($E->logbooks as $L) {
      $this->addLogbook($L->logbook_id);
    }

    $maker_count = 0;
    $tmp = array();
    foreach ($E->entry_makers as $Maker) {
      $tmp['username'] = $Maker->username;
      $tmp['staff_id'] = $Maker->user_id;
      $tmp['firstname'] = $Maker->firstname;
      $tmp['lastname'] = $Maker->lastname;
      $this->addEntryMaker($tmp);
      $maker_count++;
    }

    // Deal with group account entries (alarms, etc.)
    //if ($maker_count < 1){
    //    $tmp['username'] = $E->os_username;
    //    $this->addEntryMaker($tmp);
    //}
    $this->setAuthor($E->os_username);

    foreach ($E->attachments as $A) {

      (object) $Attachment = new stdClass();
      $data = $A->data;
      if ($data === null) {
        throw new Exception("Unable to read contents of $A->filename_orig");
      }
      $Attachment->filename = $A->filename_orig;
      $Attachment->data = base64_encode($data);
      $Attachment->encoding = 'base64';
      $Attachment->caption = $A->name;

      $finfo = finfo_open(FILEINFO_MIME_TYPE);
      $type = finfo_buffer($finfo, $data);
      if ($type) {
        $Attachment->type = $type;
      } else {
        // Use legacy mime_Type
        $Attachment->type = $A->mime_type;
      }
      $this->attachments[] = $Attachment;
    }

    if ($E->file_timestamp) {
      $this->created = date('c', $E->file_timestamp);
    } else {
      $this->created = date('c');
    }

    if ($E->entry_type == 'OPS-PR' || $E->entry_type == 'DOWNTIME') {
      foreach ($E->opsprs as $O) {
        $pr = new stdClass();
        $pr->action = $O->action;
        $pr->action_by = $O->action_by->username;
        $pr->assignee = $O->assignee->username;
        $pr->action_timestamp = date('c', $O->action_timestamp);
        $pr->component_id = $O->component_id;
        $pr->problem_id = $O->problem_id;
        $pr->action_text = $O->action_text;
        if ($O->action == 'MERGE' && is_numeric($O->merged_with)) {
          $pr->merged_with = $O->merged_with;
        }
      }

      $this->opspr_events[] = $pr;
    }

    if ($E->entry_type == 'DOWNTIME') {
      $this->downtime['time_down'] = date('c', $E->downtime->time_down);
      $this->downtime['time_up'] = date('c', $E->downtime->time_up);
      $this->downtime['time_restored'] = date('c', $E->downtime->time_restored);
    }

    // Elog Flags to Logentry Tags
    if ($E->priority == 'README' || $E->priority == 'VIP') {
      $this->addTag('Readme');
    }

    if ($E->autolog == 'T') {
      $this->addTag('Autolog');
    }

    foreach ($E->elog_references as $R) {
      $this->addReference('logbook', $R->elog_id);
    }

    foreach ($E->psslog_references as $R) {
      $this->addReference('psslog', $R->psslog_id);
    }

    foreach ($E->atlis_references as $R) {
      $this->addReference('atlis', $R->task_id);
    }
  }

  /**
   * https://drupal.stackexchange.com/questions/56487/how-do-i-get-the-path-for-public
   * @return null
   */
  public static function publicPath(){
    $wrapper = file_stream_wrapper_get_instance_by_uri('public://');
    if ($wrapper) {
      return $wrapper->realpath();
    }
    return NULL;
  }
  
  /**
   * Constructs a Logentry from a Drupal node object.
   * @param type stdClass $node
   * @todo incorporate Comments
   */
  public function constructFromNode($node, $att_encode = 'url') {
    //mypr($node);
    //var_dump($node);
    
    $this->setTitle($node->title);
    $this->setSticky($node->sticky);
    if ($node->body) {
      $this->setBody($node->body[$node->language][0]['value'], $node->body[$node->language][0]['format']);
    }
    $this->setCreated($node->created);
    $this->setLognumber($node->field_lognumber[$node->language][0]['value']);
    // Find the author
    $user = user_load($node->uid);

    $this->setAuthor($user->name);

    foreach ($node->field_logbook[$node->language] as $arr) {
      $term = taxonomy_term_load($arr['tid']);
      $this->addLogbook($term->name);
    }

    if ($node->field_entrymakers) {
      foreach ($node->field_entrymakers[$node->language] as $arr) {
        $this->addEntrymaker($arr['value']);
      }
    }

    if ($node->field_references) {
      foreach ($node->field_references[$node->language] as $arr) {
        if ($arr['value'] > FIRST_LOGNUMBER) {
          //current logbook
          $this->addReference('logbook', $arr['value']);
        } else {
          //legacy elog
          $this->addReference('elog', $arr['value']);
        }
      }
    }

    if (isset($node->field_extern_ref)){
      if ($node->field_extern_ref) {
        foreach ($node->field_extern_ref[$node->language] as $delta => $arr) {
          foreach ($arr as $ref_name => $ref_value) {
            $this->addReference($arr['ref_name'], $arr['ref_id']);
          }
        }
      }
    }

    if ($node->field_tags) {
      foreach ($node->field_tags[$node->language] as $arr) {
        $term = taxonomy_term_load($arr['tid']);
        $this->addTag($term->name);
      }
    }

    // Drupal stores images and files separately, but to the 
    // elog API, they're both just "attachments
    if ($node->field_image) {
      foreach ($node->field_image[$node->language] as $arr) {
        if (stristr($arr['uri'], 'public:/') && $att_encode != 'base64') {
          //$url = str_replace('public:/',$GLOBALS['base_url']."/files", $arr['uri']);
          $url = file_create_url($arr['uri']);
          $this->addAttachmentURL($url, $arr['title'], $arr['filemime']);
        } else {
          $file = drupal_realpath($arr['uri']);
          $this->addAttachment($file, $arr['title'], $arr['filemime']);
        }
      }
    }

    if ($node->field_attach) {
      foreach ($node->field_attach[$node->language] as $arr) {
        if (stristr($arr['uri'], 'public:/') && $att_encode != 'base64') {
          //$url = str_replace('public:/',$GLOBALS['base_url']."/files", $arr['uri']);
          $url = file_create_url($arr['uri']);
          $this->addAttachmentURL($url, $arr['description'], $arr['filemime']);
        } else {
          $file = drupal_realpath($arr['uri']);
          $this->addAttachment($file, $arr['description'], $arr['filemime']);
        }
      }
    }

    if (isset($node->field_opspr)){
      if ($node->field_opspr) {
        foreach ($node->field_opspr[$node->language] as $arr) {
          $pr = new stdClass();
          foreach ($arr as $k => $v) {
            $pr->$k = $v;
          }
          $this->opspr_events[] = $pr;
        }
      }
    }

    if (isset($node->field_downtime)){
      if ($node->field_downtime) {
        $this->downtime = $node->field_downtime[$node->language];
      }
    }
    
    if (! empty($node->problem_report)){
      $this->pr = $node->problem_report;
    }

    // Fetch the Comments (if any)  
    $cids = comment_get_thread($node, COMMENT_MODE_FLAT, 1000);
    if (count($cids) > 0) {
      foreach ($cids as $cid) {
        $node_comment = comment_load($cid);
        //var_dump($node_comment);
        $lang = $node_comment->language;
        //print "1:".$this->lognumber."\n";
        //print "2:".$node_comment->comment_body[$lang][0]['value']."\n";
        $C = new Comment($this->lognumber, $node_comment->comment_body[$lang][0]['value']);
        $C->setAuthor($node_comment->name);   // Username of comment creator
        $C->setTitle($node_comment->subject);   
        $C->setCreated($node_comment->created);      

        //@see https://drupal.stackexchange.com/questions/56487/how-do-i-get-the-path-for-public
        $publicPath = $this->publicPath();
        
        if ($node_comment->field_image) {
          foreach ($node_comment->field_image[$node_comment->language] as $arr) {
            $file = str_replace('public:/', $publicPath, $arr['uri']);
            if (file_exists($file)) {
              $C->addAttachment($file, $arr['title'], $arr['filemime']);
            }
          }
        }

        if ($node_comment->field_attach) {
          foreach ($node_comment->field_attach[$node_comment->language] as $arr) {
            $file = str_replace('public:/', $publicPath, $arr['uri']);
            if (file_exists($file)) {
              $C->addAttachment($file, $arr['title'], $arr['filemime']);
            }
          }
        }

        //var_dump($C);
        $this->comments[] = $C;
      }
    }

    //mypr($this);
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
  function __get($prop) {
    // First check for a get_$prop method
    $getter = "get" . ucfirst($prop);
    if (method_exists($this, $getter)) {
      return $this->$getter($prop);
    } elseif (isset($this->$prop)) {
      return $this->$prop;
    } else {
      return false;
    }
  }

}

