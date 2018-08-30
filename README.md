# Jefferson Lab Electronic Logbook Drupal Module

## Overview

This module implements the overall functionality of the logbook application, allowing users to post, view, and search entries.  When installed, it creates a new node bundle type named Logentry as well as two taxonomy vocabularies named Logbooks and Tags. 

## Requirements

* A Linux Server with PHP, MySQL, and Apache Web Server
  * The version of mySQL must be 5.6 or newer (MariaDB >= 10.1) to support full text indexes on innodb tables. 
  * Minimum PHP version is 5.3, but version of 7.0 or newer is highly recommended
* The latest version of Drupal 7.x  

## Installation

Install this module like any other Drupal 7 contributed module by cloning or downloading it to sites/all/modules/elog.  
A step-by-step guide to installation on Centos 7 along with all pre-requisites is available in [INSTALL.md](INSTALL.md).
After module installation, additional configuration steps via the Drupal Admin interface are documented in the wiki.





