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

**For the sake of simplicity, these instructions assume that SELinux is disabled or in permissive mode.**

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
sudo systemctl start mariadb
sudo mysql_secure_installation
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
sudo rpm -Uvh https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
sudo rpm -Uvh https://mirror.webtatic.com/yum/el7/webtatic-release.rpm
````

Install PHP 7.1 packages
````
sudo yum install -y php71w-common php71w-opcache php71w php71w-cli
sudo yum install -y php71w-opcache php71w-devel php71w-mysqlnd
sudo yum install -y php71w-pecl-xdebug
sudo yum install -y php71w-process php71w-xml php71w-pdo
sudo yum install -y php71w-mcrypt php71w-mbstring php71w-ldap
sudo yum install -y php71w-gd php71w-soap
sudo yum install -y yum-plugin-replace
sudo yum install -y php-pear-Mail-Mime
````

## Install Drupal

### Create a Database
````mysql
create database logbooks
grant all privileges on logbooks.* to "logbooks_owner"@"localhost" identified by "YourPassword";
````

### Download & Install Core Drupal 7
In this example, Drupal will be installed in a version-specific directory beneath /var/www.  A symbolic link 
named _html_ will point to the current version as the web server's document root.

````bash
cd /var/www
sudo mv html html.dist
sudo wget https://ftp.drupal.org/files/projects/drupal-7.58.tar.gz
sudo tar xf drupal-7.58.tar.gz 
sudo ln -s drupal-7.58 html
cd drupal-7.58
ln -s ../files files
cd sites/default
ln -s ../../../files files
````

Create a directory independent of Drupal version to contain file content.
````bash
cd /var/www
sudo mkdir files
sudo chown apache files
````

Create a settings.php file
````bash
cd /var/www/drupal-7.58/sites/default
cp default_settings.php settings.php
````
Now edit the settings.php and enter the settings for the database created earlier.

````bash
# Need Example here
````

Now install Drupal using its web-based installer which can be accessed via http://yourhost/install.php.

Choose the "Standard" install profile.

*Important*: Be sure that "clean urls" are enabled and working after you complete the Drupal installation.
See the Drupal documentation: https://www.drupal.org/docs/7/configuring-clean-urls/enable-clean-urls

### Download & Install Drupal Modules

#### Drush

The preferred way to download and enable modules is using the Drupal Shell (aka drush), which can be installed as follows:
````bash
sudo wget https://github.com/drush-ops/drush/releases/download/8.1.13/drush.phar
sudo mv drush.phar /usr/local/bin/drush
sudo chmod 755 /usr/local/bin/drush
````
Note that in order to use drush from behind a proxy, you can something equivalent to the following to ~/.wgetrc
````
use_proxy = on
https_proxy = http://proxy.your.org:8081
http_proxy = http://proxy.your.org:8081
````

#### Drupal Libraries
````bash
# download
cd /var/www/drupal-7.58/sites/all/libraries
wget https://github.com/twbs/bootstrap/releases/download/v3.3.7/bootstrap-3.3.7-dist.zip
wget https://github.com/select2/select2/archive/3.5.4.tar.gz
# unpack
unzip bootstrap-3.3.7-dist.zip
tar xf 3.5.4.tar.gz
# Make library symlinks
ln -s bootstrap-3.3.7-dist bootstrap
ln -s select2-3.5.4 select2
````

#### Third-party Modules & Themes

````bash
# Download
drush pm-download admin_menu bootstrap_library collapsiblock ctools
drush pm-download date devel diff field_group filefield_paths
drush pm-download jquery_update libraries pathauto serial 
drush pm-download taxonomy_menu token
drush pm-download htmlmail mimemail mailsystem
drush pm-download htmlawed 
drush pm-download js 
drush pm-download bootstrap

# Enable
drush pm-enable admin_menu bootstrap_library collapsiblock ctools
drush pm-enable date date_popup devel diff field_group filefield_paths
drush pm-enable jquery_update libraries pathauto serial 
drush pm-enable taxonomy_menu token
drush pm-enable htmlmail mimemail mailsystem mailmime
drush pm-enable htmLawed 
````

#### Logbook Modules & Themes
````bash
cd /var/www/drupal-7.58/sites/all/modules
git clone https://github.com/JeffersonLab/elog.git
cd /var/www/drupal-7.58/sites/all/themes
git clone https://github.com/JeffersonLab/bootstrap_logbooks.git
drush pm-enable elog 
````

**The Logbook Server is now ready to be configured using the Drupal Admin interface.**  
See the [Wiki](https://github.com/JeffersonLab/elog/wiki) for information about configuring the application.
