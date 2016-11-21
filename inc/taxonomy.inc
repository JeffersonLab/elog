<?php

/**
 * @file
 * taxonomy-related functions
 *
 * Functions that generate or help to generate code for retrieving and
 * displaying elog-specific taxonomies such as tags and logbooks.
 */
// If we don't include this, we can't fall back to default taxonomy_term_page
// call for terms we don't override
$taxopath = drupal_get_path('module', 'taxonomy') . '/taxonomy.pages.inc';
include ($taxopath);

/**
 * Provides specialized handlers for specific types of terms
 * @param stdClass $term
 * @return mixed
 */
function elog_taxonomy_router($term = NULL, $xtra=NULL) {
  
  if (is_object($term)) {
    switch ($term->vocabulary_machine_name) {
      case 'logbooks': $GLOBALS['conf']['cache'] = FALSE;
                       return book_listing($term, get_taxonomy_term_from_name($xtra,'Tags'));
                       break;
      case 'tags'    : $GLOBALS['conf']['cache'] = FALSE;
                       return tag_listing($term, get_taxonomy_term_from_name($xtra,'Logbooks'));
                       break;
      default : return taxonomy_term_page($term);
    }
  }
  return taxonomy_term_page($term);
}


function elog_form_taxonomy_form_term_alter(&$form, &$form_state) {
  mypr($form);
  $form['description']['#required'] = TRUE;
  $form['description']['#format'] = 'plain_text';
  $form['relations']['#collapsed'] = FALSE;
  $form['relations']['parent']['#title'] = 'Parent term';
  $form['relations']['parent']['#multiple'] = FALSE;
  //mypr($form);
  // Register a callback for after the form is built so that we can remove the text format
  // selections field
  $form['#after_build'][] = 'elog_taxonomy_form_term_after_build';
}


function elog_taxonomy_form_term_after_build($form){
  //mypr($form);
  unset($form['description']['format']);
  return $form;
}

/**
* Helper function to dynamically get a term object for
* a term_name from a named vocabulary.
*
* @param $term_name Term name (case-insensitive)
* @param $vocabulary_name Name of the vocabulary to search the term in
*
* @return Term id of the found term or else FALSE
*/
function get_taxonomy_term_from_name($term_name, $vocabulary_name) {
  if ($vid = get_taxonomy_vid_by_name($vocabulary_name)) {
    $tree = taxonomy_get_tree($vid);
    foreach ($tree as $term) {
      if (strtoupper($term->name) == strtoupper($term_name)) {
        return $term;
      }
    }
  }
  return FALSE;
}


/**
 * Get a hash table of taxonomy terms => tid for 
 * easier lookups
 * @param array $tree of stdClass taxonomy terms
 */
function get_taxonomy_name_hash($tree) {
  $hash = array();
  foreach ($tree as $term) {
    $hash[$term->name] = $term->tid;
  }
  return $hash;
}

/**
 * Returns the vid of the named vocabulary
 * @param string $name 
 * @return mixed integer or null
 */
function get_taxonomy_vid_by_name($name) {
  $vocabs = taxonomy_get_vocabularies();
  foreach ($vocabs as $vid => $obj) {
    if ($obj->name == $name) {
      return $vid;
    }
  }
  return NULL;
}

/**
 * Returns the vid of the named vocabulary
 * @param string $name 
 * @return mixed integer or null
 */
function get_taxonomy_terms_by_vocab($name) {
  $terms = array();
  $vid = get_taxonomy_vid_by_name($name);
  if ($vid) {
    $tree = taxonomy_get_tree($vid);
    $hash = get_taxonomy_name_hash($tree);
    $terms = array_flip($hash);
  }
  return $terms;
}

/**
 * Whether or not the specified name is a valid logbook
 * name.  Is it a member of the logbooks taxonomy vocab?
 * @param string $name
 * @return boolean 
 */
function get_logbook_tid($name) {
  $vid = get_taxonomy_vid_by_name('Logbooks');
  $taxo = taxonomy_get_tree($vid, 0, NULL, FALSE);
  $books = get_taxonomy_name_hash($taxo);
  if (array_key_exists(strtoupper($name), $books)) {
    return $books[$name];
  }
  return FALSE;
}


/**
 * Given an array of Taxonomy entity objects, return a properly themed
 * table or set of tables (for nested terms)
 * @param array $entities 
 */
function get_taxonomy_table_from_entity_array($entities) {
  $output = '';
  $header[] = array(
    'data' => 'Name',
    'class' => array('taxonomy-name'),
  );
  $header[] = array(
    'data' => 'Description',
    'class' => array('taxonomy-desc'),
  );

  $rows = array();
  foreach ($entities as $r) {
    if (count($r->children) > 0) {
      $term = taxonomy_term_load($r->tid);
      $output .= sprintf("<h2>%s</h2>", $term->name);
      $output .= get_taxonomy_table_from_entity_array($r->children);
    }
    else {
      $url = path_load("taxonomy/term/" . $r->tid);
      $book = l($r->name, $url['alias'], array());
      $rows[] = array(
        $book,
        $r->description,
      );
    }
  }

  if (count($rows) > 0) {
    $output .= theme('table', array(
      'header' => $header,
      'rows' => $rows,
      'attributes' => array('class' => array('taxonomy_listing')),
        )
    );
  }
  return $output;
}

/*
 * Generates a list of log entries.  
 * Examines the request_uri to extract URL query parameters
 * and use them as additional filters
 */

function taxonomy_terms_listing($vocab) {
  drupal_set_title($vocab);
  $vid = get_taxonomy_vid_by_name($vocab);
  $list = taxonomy_get_tree($vid, 0, NULL, TRUE);
  // We didn't really get a tree back.  We got a flat list that we'll
  // have to turn into hierarchy ourselves
  //mypr($list);
  foreach ($list as $obj) {
    $obj->children = array();
    if ($obj->depth == 0) {
      $terms[$obj->tid] = $obj;
    }
    else {
      // Not bothering to deal with multi-parent terms
      $terms[$obj->parents[0]]->children[] = $obj;
    }
  }
  //mypr($terms);
  $output = get_taxonomy_table_from_entity_array($terms);
  //mypr($output);
  return $output;
}

/**
 * Whether or not the specified name is a valid logbook
 * name.  Is it a member of the logbooks taxonomy vocab?
 * @param string $name
 * @return boolean 
 */
function is_valid_logbook($name) {
  $vid = get_taxonomy_vid_by_name('Logbooks');
  $taxo = taxonomy_get_tree($vid, 0, NULL, FALSE);
  $books = get_taxonomy_name_hash($taxo);
  if (array_key_exists(strtoupper($name), $books)) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Writes out an XSD file for tags or logbooks
 * @param type $file  - file to write
 * @param type $tag  - what the element name will be inside the file
 * @param type $vocab - The vocabulary name (not machine name) 
 */
function elog_taxonomy_write_xsd($file, $tag, $vocab){
 watchdog('elog', "Attempting to update schema file @file", 
      array('@file'=>$file), WATCHDOG_INFO);
 if ( is_writable($file) ) {
  $terms = get_taxonomy_terms_by_vocab($vocab);
  if (count($terms) > 0){
    $xsd = "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\n";
    $xsd .= "<xs:element name=\"$tag\">\n";
    $xsd .= "<xs:simpleType>\n";
    $xsd .= "<xs:restriction base=\"xs:string\">\n";
    foreach ($terms as $term){
      $xsd .= "\t<xs:enumeration value=\"$term\"/>\n";
    }
    $xsd .= "</xs:restriction>\n";
    $xsd .= "</xs:simpleType>\n";
    $xsd .= "</xs:element>\n";
    $xsd .= "</xs:schema>\n";
    if (! file_put_contents($file, $xsd)){
        watchdog('elog', "Failed writing schema file @file", 
              array('@file'=>$file), WATCHDOG_CRITICAL);
        return FALSE;
    }
  }else{
    watchdog('elog', "Unable to update Schema.  @vocab has no terms", 
           array('@vocab'=>$vocab), WATCHDOG_CRITICAL);
    return FALSE;
  }
      
 }else{
       watchdog('elog', "Unable to update Schema.  @file is not writable", 
           array('@file'=>$file), WATCHDOG_CRITICAL);
       return FALSE;
 }
 return TRUE;
}


function elog_taxonomy_update_schema(){
  $schemaDir = $_SERVER['DOCUMENT_ROOT'] . 'schema';
  $logbooksXsd = $schemaDir . '/LogbooksEnum.xsd';
  $tagsXsd = $schemaDir . '/TagsEnum.xsd';
  $success1 = elog_taxonomy_write_xsd($logbooksXsd, 'logbook', 'Logbooks');
  $success2 = elog_taxonomy_write_xsd($tagsXsd, 'tag', 'Tags');
  return ($success1 && $success2);
}