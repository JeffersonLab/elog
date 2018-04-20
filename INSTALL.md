# Drupal 7 Logbooks Install Guide

## Overview
This guide details the steps for installing and configuring a logbooks service using Drupal 7 and the JeffersonLab elog modules.
The steps are documented for a Centos/RHEL 7 Linux distribution.  Adapting the instructions to another platform such as Ubuntu
is left as an excercise to the reader.

## Prerequisites
This guide assumes the availability of a dedicated Centos/RHEL 7 Linux Server.  Some useful resources include:

* https://www.digitalocean.com/community/tutorials/initial-server-setup-with-centos-7
* https://www.digitalocean.com/community/tutorials/additional-recommended-steps-for-new-centos-7-servers
* https://www.digitalocean.com/community/tutorials/how-to-install-linux-apache-mysql-php-lamp-stack-on-centos-7

## Install Updated MariaDB 
The MariaDB 5.5 version distributed with Centos 7 does not support full-text indexes on innodb tables.  

The steps below are to install MariaDB 10.2


Copy and paste the text below into a file named /etc/yum.repos.d/MariaDB.repo.
````
# MariaDB 10.2 CentOS repository list
# http://downloads.mariadb.org/mariadb/repositories/
[mariadb]
name = MariaDB
baseurl = http://yum.mariadb.org/10.2/centos7-amd64
gpgkey=https://yum.mariadb.org/RPM-GPG-KEY-MariaDB
gpgcheck=1
enable=1
````
After the file is in place, install MariaDB with
````
sudo yum install MariaDB-server MariaDB-client
````

## Install PHP 7 (optional)
While Drupal 7 and the elog module are compatible with PHP versions as old a 5.3, upgrading to PHP 7 will offer improved
performance.  The instructions below are used to replace the Centos-provided PHP 5.4 with PHP 7.1 using the Webtatic 
(https://webtatic.com/packages/php71/) repo.

Remove the Centos provided version of php:
````
# Removing php-common should also remove all dependencies.
yum remove php-common
````

Create a Webtatic repo:
````
rpm -Uvh https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
rpm -Uvh https://mirror.webtatic.com/yum/el7/webtatic-release.rpm
````

Install PHP 7.1 packages
````
yum install -y  php71w-common php71w-opcache php71w php71w-cli
yum install -y  php71w-opcache php71w-devel php71w-mysqlnd
yum install -y  php71w-pecl-xdebug
yum install -y  php71w-process php71w-xml php71w-pdo
yum install -y  php71w-mcrypt php71w-mbstring php71w-ldap
yum install -y  php71w-gd php71w-soap
yum install -y  yum-plugin-replace
yum install php-pear-Mail-Mime
````

## Install Drupal

