<?php
/**
* File implementing OPSPR class.
*/


/**
* Logentry class.  
*
* Logentry is the most basic type of logentry.
*/
class OPSPR {


/** 
* The logentry portion of the OPS-PR
*
* @var Logentry
*/
protected $logentry;


/** 
* The OPS-PR (problem tracking) events history
* @var array of OPSPREvent
*/
protected $events = array();


/**
* Instantiates a logentry
*
* Constructor is overloaded and accepts as an argument either:
*   1) A DOMDocument object in Logentry.xsd format
*   2) An Elog object from the current elog system
*   4) A string to be the title of a new logentry
* 
*/
public function __construct(){
    $args = func_get_args();
    if (count($args) == 1){
        if (is_a($args[0], 'DOMDocument')){
            $this->constructFromDom($args[0]);
        }elseif (is_a($args[0], 'Elog')){    
            $this->constructFromElog($args[0]);
        }elseif (is_a($args[0], 'StdClass')){    
            $this->constructFromNode($args[0]);    
        }
    }else{
           $this->logentry = new Logentry();
    }
    if (count($args) > 1){
        throw new Exception ("Too many arguments");
    }
}

/**
* Accepts a DOMDocument and uses xpath queries to extract the elements
* of interest into object properties.
*
* @param DOMDocument/DomElemet $dom
*/
protected function constructFromDom($dom){   

    $nodes = $dom->getElementsByTagName('Logentry');
    if ($nodes->length == 1) {
        $domElem = $nodes->item(0);
        $logentry = new Logentry($domElem);
        $this->setLogentry($logentry);
    }else{
        throw new Exception("Unexpected Logentry count in OPS-PR");
    }
        
    $xpath = new DOMXPath($dom);
    $query = 'OPSPREvents/OPSPREvent';
    $entries = $xpath->query($query);
    foreach ($entries as $entry) {
        $this->events[] = $this->OPSPREventFromDom($entry);
    }
}


public function setLogentry(Logentry $logentry){
    $this->logentry = $logentry;
}


public function OPSPREventFromDom(DOMElement $dom){
    $event = new stdClass();
    foreach ($dom->childNodes as $node){
        $key = $node->nodeName;
        $event->$key = $node->nodeValue;        
    }
    return $event;
}

/**
* Adds an OPSPREvent
*    
* @param OPSPREvent $event
*/
public function addOPSPREvent(OPSPREvent $event){
    $this->events[] = $event;
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
function getXML($name='', $excludes=NULL)
{
    if (! $name) {$name = get_class($this);}
    if (! is_array($excludes)) { 
      $exclude = array();
    }
    $xw = new xmlWriter();
    $xw->openMemory();
    $xw->setIndent(true);
    $xw->startElement($name);
    
 
    foreach (get_object_vars($this) as $n => $var)
    {
        if (in_array($n,$excludes)) {
          continue;
        }
          

        if ($n == 'logentry'){
            $xw->writeRaw($var->getXML());
            continue;
        }

        
        if ($n == 'events'){
            $xw->startElement('OPSPREvents');
            foreach ($this->events as $event){
                $xw->startElement('OPSPREvent');
                foreach (get_object_vars($event) as $key => $val) {
                    $xw->writeElement($key, $val);
                }
                $xw->endElement();
            }
            $xw->endElement();
            continue;
        }

   }

    
    $xw->endElement();
    return $xw->outputMemory(true);
}




/**
* Accepts a legacy Elog object from which to construct
* properties.
*/
protected function constructFromElog(Elog $E){   

    $this->logentry = new Logentry($E);
    
    foreach ($E->opspr_events as $event){
        var_dump($event);
    }
    
}

/**
 * Constructs a Logentry from a Drupal node object.
 * @param type stdClass $node
 * @todo incorporate Comments
 */
public function constructFromNode($node){
    $this->logentry = new Logentry($node);
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

}// OPSPR