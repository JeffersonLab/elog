<?php
include_once("AbstractQuery.inc.php");

/**
 * Group by lognumber/title
 * 10/page
 * Different/No display filters, book filters, tag filters
 */

class CommentQuery extends AbstractQuery{
  
  
public function __construct(){
  $this->group_by = 'LOGNUMBER';
  $this->entries_per_page = 10;
  $this->sort_field = 'created';
}

/**
 * Generates a drupal db_select query built from the object properties
 * that have been set.
 * @todo use mysql greatest() function to return most recent of node ->changed
 *       or node->last_comment_timestamp;
 * @return SelectQuery
 * @see http://api.drupal.org/api/drupal/includes!database!select.inc/class/SelectQuery/7
 */
function getDBQuery(){
  $query = db_select('comment', 'c');
  $query->fields('c');  
  $query->condition('c.status', 0,'>');  //published
  
  
  // @todo We should also do a query against entry maker fields
  if ($this->uid){
    $query->condition('c.uid',$this->uid);
  }
  
  $query->join('users', 'u', 'c.uid = u.uid'); //JOIN node with users
  $query->fields('u',array('uid','name'));
  
  $query->join('node', 'n', 'c.nid = n.nid'); //JOIN node with users
  $query->fields('n',array('title'));
  
  $query->join('field_data_field_lognumber', 'l', 'c.nid = l.entity_id'); 
  $query->fields('l',array('lognumber'=>'field_lognumber_value'));

  if ($this->start_date){
    $query->condition('c.changed',$this->start_date,'>=');
  }
  
  if ($this->end_date){
    $query->condition('c.changed',$this->end_date,'<=');
  }
  
  if ($this->entries_per_page > 0){
    $query = $query->extend('PagerDefault')->limit($this->entries_per_page);
  }
 
  $query->orderBy($this->sort_field, $this->sort_direction);

  return $query;
  
}


function getDBQueryResults(){
{
    $map = array();
    $grouped_items = array();
    $query = $this->getDBQuery();
    //mypr($query);
    $results = $query->execute();
    //mypr($results);
    while ($r = $results->fetchAssoc()){       
      switch ($this->group_by){
        case 'LOGNUMBER'    :   $key = $r['field_lognumber_value'].': '.$r['title'];
                                $map[$key] = $key;
                                $grouped_items[$key][$r['cid']] = $r;
                                break;
        case 'DAY'          :   $key = date("l (d-M-Y)",$r['changed']);
                                $map[$key] = $key;
                                $grouped_items[$key][$r['cid']] = $r;
                                break;
                              

        case 'SHIFT'        :   $hour = date('G', $r['changed']);
                                        $shift = $this->ops_shifts[$hour];
                                        if ($hour = 23) {
                                            $day = date("l (d-M-Y)",$r['changed']+3601);
                                        }else{
                                            $day = date("l (d-M-Y)",$r['changed']);
                                        }
                                        $key = "$shift $day";
                                        $map[$key] = $key;
                                        $grouped_items[$key][$r['cid']] = $r;
                                        break;


                default             :   $grouped_items[0][$r['cid']] = $r;
                                        //$this->column_date_fmt = 'm/d/y H:i';
            }
        }       
    //mypr($grouped_items);
    return $grouped_items;
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
  switch(strtolower($val)){
    case 'posted' : $this->sort_field = 'changed'; break;
    case 'submitter'    :
    case 'submitted by' : $this->sort_field = 'u.name'; 
                          $this->group_by = 'NONE';
                          break;
    case 'title' : $this->sort_field = 'subject';
                   $this->group_by = 'NONE';
                   break;
    
    //case 'lognumber' : $this->sort_field = 'l.field_lognumber_value'; break;
  
  }
}



}//class
