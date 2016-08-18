<?php

include_once("AbstractQuery.inc.php");

class LogentryQuery extends AbstractQuery {

  /**
   * An array of filters to be turned into
   * database where clauses.
   * @var type 
   */
  protected $filters = array();

  /**
   * An array of the output fields the user desires.
   */
  public $fields = array();

  /**
   * The tags to include in the query
   */
  public $tags;

  /**
   * The date shown in table view.
   * @var type 
   */
  public $table_date = 'created';

  /**
   * A list of tags to exclude from query results
   */
  public $exclude_tags = array();

  /**
   * A list of logbooks to exclude from query results
   */
  public $exclude_books = array();

  /**
   * The state of the sticky flag
   */
  public $sticky;

  /**
   * The logbooks to query
   */
  public $logbooks;

  /**
   * Extra columns that may be desired in table output
   */
  public $cols = array(
    'books' => FALSE,
    'tags' => FALSE,
  );

  /**
   * Group entries by (DAY, SHIFT, )
   */
  public $group_by = 'SHIFT';

  /**
   * Accepts an associative array and applies each 
   * @param var $settings
   */
  public function applyDisplaySettings($settings) {
    parent::applyDisplaySettings($settings);
    // hide_autologs requires special handling
    if (array_key_exists('hide_autologs', $settings)) {
      if ($settings['hide_autologs']) {
        $this->excludeTag('Autolog');
      }
    }
  }

  /**
   * Accepts an array of logbooks tids, names, or objects and
   * sets the query accordingly.
   * @param type $logbooks
   */
  public function setLogbooks($logbooks) {
    if (is_array($logbooks)) {
      foreach ($logbooks as $book) {
        $this->addLogbook($book);
      }
    }
    else {
      // Hand it off on assumption its a string
      $this->addLogbook($logbooks);
    }
  }

  /**
   * Accepts an array of logbooks tids, names, or objects and
   * sets the query accordingly.
   * @param type $logbooks
   */
  public function unSetLogbooks($logbooks) {
    if (is_array($logbooks)) {
      foreach ($logbooks as $book) {
        $this->excludeBook($book);
      }
    }
    else {
      // Hand it off on assumption its a string
      $this->excludeBook($logbooks);
    }
  }

  /**
   * Accepts an array of logbooks tids, names, or objects and
   * sets the query accordingly.
   * @param type $logbooks
   */
  public function setTags($tags) {
    if (is_array($tags)) {
      foreach ($tags as $tag) {
        $this->addTag($tag);
      }
    }
    else {
      // Hand it off on assumption its a string
      $this->addTag($tags);
    }
  }

  
    /**
   * Accepts an array of logbooks tids, names, or objects and
   * sets the query accordingly.
   * @param type $logbooks
   */
  public function unSetTags($tags) {
    if (is_array($tags)) {
      foreach ($tags as $tag) {
        $this->excludeTag($tag);
      }
    }
    else {
      // Hand it off on assumption its a string
      $this->excludeTag($tags);
    }
  }
  
  /**
   * Adds a logbook to the list of logbooks from which to query
   * @param mixed $book
   */
  public function addLogbook($book) {
    $term = null;
    
    
    if (is_object($book)) {
      $term = $book;
    }
    elseif (is_numeric($book)) {  // assume tid
      $term = taxonomy_term_load($book);
    }
    elseif (is_string($book)) {
      $tmp = taxonomy_get_term_by_name($book);
      $term = array_shift($tmp);
    }
    //mypr($term);
    if ($term) {
      $this->logbooks[$term->tid] = $term->name;
    }
  }

  /**
   * Adds a tag to the list of tags from which to query
   * @param mixed $tag
   */
  public function addTag($tag) {
    $term = null;
    if (is_object($tag)) {
      $term = $tag;
    }
    elseif (is_numeric($tag)) {  // assume tid
      $term = taxonomy_term_load($tag);
    }
    elseif (is_string($tag)) {
      $tmp = taxonomy_get_term_by_name($tag);
      $term = array_shift($tmp);
    }
    //mypr($term);
    if ($term) {
      $this->tags[$term->tid] = $term->name;
    }
  }

  /**
   * Adds a tag to the list of tags from which to query
   * @param mixed $tag
   */
  public function excludeTag($tag) {
    $term = null;
    if (is_numeric($tag)) {  // assume tid
      $term = taxonomy_term_load($tag);
    }
    elseif (is_string($tag)) {
      $tmp = taxonomy_get_term_by_name($tag);
      $term = array_shift($tmp);
    }
    //mypr($term);
    if ($term) {
      $this->exclude_tags[$term->tid] = $term->name;
    }
  }

  /**
   * Removes a book from the list of tags from which to query
   * @param mixed $book
   */
  public function excludeBook($book) {
    $term = null;
    if (is_numeric($book)) {  // assume tid
      $term = taxonomy_term_load($book);
    }
    elseif (is_string($book)) {
      $tmp = taxonomy_get_term_by_name($book);
      $term = array_shift($tmp);
    }
    if ($term) {
      $this->exclude_books[$term->tid] = $term->name;
    }
    //mypr($term);
  }

  /**
   * Accepts an associative array of the format produced
   * by php's $_GET or $_POST and pulls out values that
   * are recognized as valid filters.
   * @param array $var 
   */
//public function addFiltersFromArray($var){
//  parent::addFiltersFromArray($var);
//}

  /**
   * Generates a drupal db_select query built from the object properties
   * that have been set.
   * @todo use mysql greatest() function to return most recent of node ->changed
   *       or node->last_comment_timestamp;
   * @return SelectQuery
   * @see http://api.drupal.org/api/drupal/includes!database!select.inc/class/SelectQuery/7
   */
  public function getDBQuery() {

    if (empty($this->sort_field)) {
      $this->sort_field = $this->table_date;
    }

    //The drupal convention is to declare query extender right
    //up front. @see https://drupal.org/node/508796
    if ($this->entries_per_page > 0) {
      $query = db_select('node', 'n')->extend('PagerDefault');
      $query->limit($this->entries_per_page);
    }
    else {
      $query = db_select('node', 'n');    // Don't use a pager
    }

    $query->fields('n');
    $query->condition('n.status', 0, '>');  //published
    if (isset($this->sticky)) {
      $query->condition('n.sticky', $this->sticky);
    }

    //hmm.  last_comment_timestamp is a node property, but not a database column
    //We'd have to join to comment table and select the greatest comment changed
    //column.  A bit more complicated.
    //$query->addExpression('GREATEST(n.changed, n.last_comment_timestamp)','last_modified');
    // @todo We should also do a query against entry maker fields
    if ($this->uid) {
      $query->condition('n.uid', $this->uid);
    }

    $query->join('users', 'u', 'n.uid = u.uid'); //JOIN node with users
    $query->fields('u', array('uid', 'name'));

    $query->join('field_data_field_lognumber', 'l', 'n.nid = l.entity_id');
    $query->fields('l', array('lognumber' => 'field_lognumber_value'));

    $query->leftJoin('node_comment_statistics', 's', 'n.nid = s.nid');
    $query->fields('s', array('comment_count'));

    $query->leftJoin('elog_attachment_statistics', 'eas', 'n.nid = eas.nid');
    $query->addExpression('eas.file_count + eas.image_count', 'attachment_count');

    $query->leftJoin('field_data_field_logbook', 'b', 'n.nid = b.entity_id');
    $query->addExpression('GROUP_CONCAT(DISTINCT(b.field_logbook_tid))', 'book_tids');

    $query->leftJoin('field_data_field_tags', 't', 'n.nid = t.entity_id'); //JOIN node with tags
    $query->addExpression('GROUP_CONCAT(DISTINCT(t.field_tags_tid))', 'tag_tids');


    if (field_info_instance('node', 'field_downtime', 'logentry')){
      $query->leftJoin('field_data_field_downtime', 'dt', 'n.nid = dt.entity_id');
      $query->fields('dt', array('field_downtime_time_down'));
    }
    if (field_info_instance('node', 'field_extern_ref', 'logentry')) {
      $query->leftJoin('field_data_field_extern_ref', 'xr', "n.nid = xr.entity_id and field_extern_ref_ref_name = 'dtm'");
      $query->addExpression('COUNT(xr.field_extern_ref_ref_name)', 'dtm_refcount');
    }
    if (field_info_instance('node', 'field_opspr', 'logentry')) {
      //Legacy 6GeV OPS-PR field
      $query->leftJoin('field_data_field_opspr', 'pr6', 'n.nid = pr6.entity_id');
      $query->fields('pr6', array('field_opspr_component_id'));
    }

    // New 12GeV PR entity
    if (module_exists('elog_pr')) {
      $query->leftJoin('elog_pr', 'pr', 'n.nid = pr.prid');
      $query->fields('pr', array('needs_attention'));
    }

    //Free form entry makers
    $query->leftJoin('field_data_field_entrymakers', 'e', 'n.nid = e.entity_id');
    $query->addExpression('GROUP_CONCAT(DISTINCT(e.field_entrymakers_value))', 'entrymakers');




    $query->groupBy("l.field_lognumber_value");


    // For memory/performance reasons, we don't join and return the
    // body unless the field is being searched or has explicitly 
    // been resquested as output.
    if ($this->search_str || in_array('body', $this->fields)) {
      $query->leftJoin('field_data_body', 'bd', 'n.nid = bd.entity_id');
      if (in_array('body', $this->fields)) {
        $query->fields('bd', array('body_value', 'body_format'));
      }
    }


    if ($this->search_str) {
      //$query->leftJoin('field_data_body', 'bd', 'n.nid = bd.entity_id');
      $or = db_or();

      // @todo is there some sort of test to detect if text index exists?
      //$or->condition('n.title', '%'.$this->search_str.'%', 'LIKE');
      //$or->condition('bd.body_value', '%'.$this->search_str.'%', 'LIKE');

      $or->where("match(n.title) against (:str IN NATURAL LANGUAGE MODE)", array(':str' => $this->search_str));
      $or->where("match(bd.body_value) against (:str IN NATURAL LANGUAGE MODE)", array(':str' => $this->search_str));

      // Wildcard against username
      $or->condition('u.name', '%' . db_like($this->search_str) . '%', 'LIKE');

      // Wildcard against entrymakers
      $or->condition('e.field_entrymakers_value', '%' . db_like($this->search_str) . '%', 'LIKE');

      // Wildcard against user fields
      $or->condition('u.name', '%' . db_like($this->search_str) . '%', 'LIKE');
      
      
      //Important!
      $query->condition($or);
    }

    if (!empty($this->filters)) {
      $this->applyFilters($query);
    }

    if ($this->start_date) {
      if ($this->table_date == 'created') {
        $query->condition('n.created', $this->start_date, '>=');
      }
      else {
        $query->condition('n.changed', $this->start_date, '>=');
      }
    }

    if ($this->end_date) {
      if ($this->table_date == 'created') {
        $query->condition('n.created', $this->end_date, '<=');
      }
      else {
        $query->condition('n.changed', $this->end_date, '<=');
      }
    }



    // Since the field tags is a taxonomy_term_reference, 
    // we are looking for a "tid".
    if (count($this->logbooks) > 0) {
      $query->condition('b.field_logbook_tid', array_keys($this->logbooks), 'IN');
    }


    if (count($this->tags) > 0) {
      $query->condition('t.field_tags_tid', array_keys($this->tags), 'IN');
    }

    // Need a subquery to exclude tags
    if (count($this->exclude_tags) > 0) {
      $subquery = db_select('field_data_field_tags', 'f');
      $subquery->fields('f', array('entity_id'));
      $subquery->condition('f.field_tags_tid', array_keys($this->exclude_tags), 'IN');
      $query->condition('n.nid', $subquery, 'NOT IN');
    }

    // Need a subquery to exclude books
    if (count($this->exclude_books) > 0) {
      $subquery = db_select('field_data_field_logbook', 'f2');
      $subquery->fields('f2', array('entity_id'));
      $subquery->condition('f2.field_logbook_tid', array_keys($this->exclude_books), 'IN');
      $query->condition('n.nid', $subquery, 'NOT IN');
    }

    $query->orderBy($this->sort_field, $this->sort_direction);

    // Example of title search for word drupal
    // I promised, it's a node property and not a field.
    //->propertyCondition('title', 'drupal', 'CONTAINS')
    //mypr(array_keys($this->exclude_tags));
    //mypr($query->execute());
    //var_dump($query->execute());
    
    //useful devel module function to output query with
    //parameters mapped in.
    //dpq($query);
    return $query;
  }

  protected function applyFilters(&$query) {
    if (array_key_exists('title', $this->filters)) {
      foreach ($this->filters['title'] as $filter) {
        $query->condition('n.title', '%' . $filter . '%', 'LIKE');
      }
    }

    return $query;
  }

  /**
   * 
   * @param type $val
   */
  function setSortDirection($val) {
    if (strtolower($val) == 'desc' || strtolower($val) == 'asc') {
      $this->sort_direction = strtolower($val);
    }
  }

  /**
   * Sets the field on which to sort.
   * Note:  The incoming parameter matches the column heading
   *        the user click and needs to be mapped to a database column.
   * @param type $val
   */
  function setSortField($val) {

    /* Note:  The database column is tied to the query in getDBQuery() and
     *        in some cases will need to be prefixed with a column alias
     *        for columns that don't come from the base node table.
     *        (ex:: l.lognumber)
     */
    switch (strtolower($val)) {
      case 'date' :
      case 'posted' : $this->sort_field = $this->table_date;
        break;
      case 'author' :
      case 'submitter' :
      case 'submitted by' : $this->sort_field = 'u.name';
        $this->group_by = 'NONE';
        break;
      case 'title' : $this->sort_field = 'title';
        $this->group_by = 'NONE';
        break;

      case 'lognumber' : $this->sort_field = 'l.field_lognumber_value';
        break;
    }
  }

  protected function groupByDay($results) {
    $grouped_items = array();
    while ($r = $results->fetchAssoc()) {
      $key = date("l (d-M-Y)", $r[$this->table_date]);
      $grouped_items[$key][$r['field_lognumber_value']] = $r;
    }
    return $grouped_items;
  }

  protected function groupByShift($results) {
    $grouped_items = array();
    while ($r = $results->fetchAssoc()) {
      $hour = date('G', $r[$this->table_date]);
      $shift = $this->ops_shifts[$hour];
      if ($hour = 23) {
        $day = date("l (d-M-Y)", $r[$this->table_date] + 3601);
      }
      else {
        $day = date("l (d-M-Y)", $r[$this->table_date]);
      }
      $key = "$shift $day";
      $grouped_items[$key][$r['field_lognumber_value']] = $r;
    }
    return $grouped_items;
  }

  protected function groupByDefault($results) {
    $grouped_items = array();
    while ($r = $results->fetchAssoc()) {
      $grouped_items[0][$r['field_lognumber_value']] = $r;
    }
    return $grouped_items;
  }

  /**
   * Returns the array of data generated by getDBQuery properly grouped.
   * Most commonly will be to group them by Shift, but they 
   * could also be grouped by entry_type or logbook.  Grouping the entries
   * may cause some to be repeated.  For example if grouped by logbook, then
   * entries assigned to multiple logbooks will be repeated.
   *
   * @return array Returns a 2D Array ([group_by_key][lognumber] => $row)
   */
  function getDBQueryResults() {
    $query = $this->getDBQuery();
    $results = $query->execute();
    switch ($this->group_by) {
      case 'DAY' : $grouped_items = $this->groupByDay($results);
        break;
      case 'SHIFT' : $grouped_items = $this->groupByShift($results);
        break;
      default : $grouped_items = $this->groupByDefault($results);
    }
    return $grouped_items;
  }

  /**
   * Accepts an associative array of the format produced
   * by php's $_GET or $_POST and pulls out values that
   * are recognized as valid filters.
   * @param array $var 
   */
  public function addFiltersFromArray($var) {
    $added = 0;
    parent::addFiltersFromArray($var);

    // Remaining filters in arbitrary order.
    foreach ($var as $key => $val) {
      switch ($key) {
        case 'author' : $this->addAuthor($var['author']);
          break;
        case 'logbooks' : $this->setLogbooks($var['logbooks']);
          break;
        case 'exclude_logbooks' : $this->unSetLogbooks($var['exclude_logbooks']);
          break;
        case 'tags' : $this->setTags($var['tags']);
          break;
        case 'exclude_tags' : $this->unSetTags($var['exclude_tags']);
          break;
        case 'hide_autologs' : $this->excludeTag('Autolog');
          break;
      }
    }
    if ($added > 0) {
      $this->filter_count = $added;
    }
  }

  /**
   * 
   * @param type $type
   * @param type $arg
   */
  public function addFilter($type, $arg) {
    switch ($type) {
      case 'title' : $this->filters['title'][] = $arg;
        break;
      case 'book[]' :       //jquery names arrays in this format
      case 'book' : $this->addBookFilter($arg);
        break;
      case 'tag[]' :        //jquery names arrays in this format
      case 'tag' : $this->addTagFilter($arg);
        break;
    }
  }

  /**
   * Include or exclude entries of a specified logbook
   * @param type $arg
   */
  function addBookFilter($arg) {
    //If the argument begins with a - we will treat it as an exclude
    if (substr($arg, 0, 1) == '-') {
      $this->excludeBook(substr($arg, 1)); //strip 1st char
    }
    else {
      $this->addLogbook($arg);
    }
  }

  /**
   * Include or exclude entries with a specified tag
   * @param type $arg
   */
  function addTagFilter($arg) {
    //If the argument begins with a - we will treat it as an exclude
    if (substr($arg, 0, 1) == '-') {
      $this->excludeTag(substr($arg, 1));    //strip 1st char
    }
    else {
      $this->addTag($arg);
    }
  }

  /**
   * Specifies a field whose output is desired
   * @param type $type
   * @param type $arg
   */
  public function addField($arg) {
    if (!in_array($arg, $this->fields)) {
      $this->fields[] = $arg;
    }
  }

  /**
   * Applies a pager limit by setting entries_per_page
   * @param type $limit
   */
  public function setLimit($limit) {
    if (is_numeric($limit)) {
      $this->entries_per_page = $limit;
    }
  }

  /**
   * Accepts a URI query string and pulls out the recognized filters.
   * Parses the raw Query string b/c the PHP $_GET array doesn't handle
   * duplicate variables and we want user to be able to specify multiple
   * filters of the same type.
   * Some common urlencodes for reference
   *      = is %3D
   *      % is %25
   *      ? is %3F
   *
   * @param string $str Query string to be parsed for filters
   */
  public function addFiltersFromQueryStr($str) {
    $op = array();
    $pairs = explode("&", $str);
    foreach ($pairs as $pair) {
      if (strstr($pair, '=')){
      list($k, $v) = array_map("urldecode", explode("=", $pair));
      switch (strtolower($k)) {
        case 'title' : $this->addFilter('title', $v);
          break;
        case 'field[]' :
        case 'field' : $this->addField($v);
          break;
        case 'author' : $this->addAuthor($v);
          break;
        case 'limit' : $this->setLimit($v);
          break;
        case 'startdate' : $this->setStartDate($v);
          break;
        case 'enddate' : $this->setEndDate($v);
          break;
        default : $this->addFilter($k, $v);
      }//switch
      
    }
    }
  }

}

//class
