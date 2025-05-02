#!/bin/csh -f

## this script creates a database for testing dvopsps

if ($#argv < 3) goto usage;

if ("$1" == "user") then
  if ($#argv != 4) goto usage;
  set dbhost = $2
  set dbuser = $3
  set dbpass = $4

  mysql -h $dbhost -u root -p <<EOF
   grant all on *.* to $dbuser@"$dbhost" identified by '$dbpass';
EOF
    exit 0;
endif

if ("$1" == "create") then
  if ($#argv < 4) goto usage;
  set dbhost = $2
  set dbname = $3
  set dbuser = $4
  if ($#argv == 5) then
    set dbpass = "-p$5"
  else 
    set dbpass = "-p"
  endif

  mysql -h $dbhost -u $dbuser $dbpass <<EOF
   CREATE DATABASE $dbname;
   USE $dbname;
   CREATE TABLE dvoDetection (
               imageID INT,
               ippDetectID BIGINT,
               detectID BIGINT,
               ippObjID BIGINT,
               objID BIGINT,
               flags INT,
               zp REAL,
               zpErr REAL,
               airMass REAL,
               expTime REAL,
               ra FLOAT,
               dec_ FLOAT,
               raErr REAL,
               decErr REAL,
               PRIMARY KEY (imageID, ippDetectID) 
    );
EOF
    exit 0;
endif

if ("$1" == "delete") then
  if ($#argv < 4) goto usage;
  set dbhost = $2
  set dbname = $3
  set dbuser = $4
  if ($#argv == 5) then
    set dbpass = "-p$5"
  else 
    set dbpass = "-p"
  endif

  echo mysql -h $dbhost -u $dbuser $dbpass

  mysql -h $dbhost -u $dbuser $dbpass <<EOF > /dev/null
   USE $dbname;
   describe dvoDetection;
EOF

  if ($status) then
    echo "database does not contain dvoDetections, not deleting"
    exit 1;
  endif

  echo "database is valid, deleting"
  mysql -h $dbhost -u $dbuser $dbpass <<EOF
   drop database $dbname;
EOF

   exit 0;
endif

usage:
  echo "USAGE: dbadmin.sh (options)"
  echo ""
  echo "  dbadmin.sh create (dbhost) (dbname) (dbuser)"
  echo "      create a new test db"
  echo ""
  echo "  dbadmin.sh delete (dbhost) (dbname) (dbuser)"
  echo "      delete an existing test db (enter pass 2x)"
  echo ""
  echo "  dbadmin.sh user (dbhost) (dbuser) (password)"
  echo "      create a new user and password for the test db"
  echo ""
  exit 2
