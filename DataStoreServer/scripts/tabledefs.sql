DROP TABLE IF EXISTS dsFileType;
DROP TABLE IF EXISTS dsFile;
DROP TABLE IF EXISTS dsFileset;
DROP TABLE IF EXISTS dsProduct;

#
# 

CREATE TABLE dsProduct (
    prod_id     BIGINT(20) NOT NULL AUTO_INCREMENT PRIMARY KEY,
    prod_name   VARCHAR(64) NOT NULL UNIQUE,
    last_update datetime,
    last_fs     VARCHAR(64),
    type        VARCHAR(64),
    description VARCHAR(255),
    prod_col_0  VARCHAR(64),        # labels for product specific columns
    prod_col_1  VARCHAR(64),        # these appear in the header list
    prod_col_2  VARCHAR(64),
    prod_col_3  VARCHAR(64),
    prod_col_4  VARCHAR(64),
    prod_col_5  VARCHAR(64),
    prod_col_6  VARCHAR(64),
    prod_col_7  VARCHAR(64)
) Engine=InnoDB DEFAULT CHARSET=latin1;
   

CREATE TABLE dsFileset (
    prod_id     BIGINT(20),
    fileset_id  BIGINT(20) NOT NULL AUTO_INCREMENT PRIMARY KEY,
    fileset_name VARCHAR(64) NOT NULL,
    reg_time    datetime NOT NULL,
    hide        TINYINT DEFAULT 0,
    type        VARCHAR(64),
    prod_col_0  VARCHAR(64),        # values for product specific columns
    prod_col_1  VARCHAR(64),
    prod_col_2  VARCHAR(64),
    prod_col_3  VARCHAR(64),
    prod_col_4  VARCHAR(64),
    prod_col_5  VARCHAR(64),
    prod_col_6  VARCHAR(64),
    prod_col_7  VARCHAR(64),
    FOREIGN KEY  (prod_id) REFERENCES dsProduct(prod_id)
) Engine=InnoDB DEFAULT CHARSET=latin1;


CREATE TABLE dsFile (
    fileset_id  BIGINT(20),
    file_id     BIGINT(20) NOT NULL AUTO_INCREMENT PRIMARY KEY,
    file_name   VARCHAR(255) NOT NULL,
    bytes       BIGINT(20),
    md5sum      VARCHAR(255),
    type        VARCHAR(64),
    type_col_0  varchar(64),        # value for type specific columns
    type_col_1  varchar(64),
    type_col_2  varchar(64),
    type_col_3  varchar(64),
    type_col_4  varchar(64),
    type_col_5  varchar(64),
    type_col_6  varchar(64),
    type_col_7  varchar(64),
    FOREIGN KEY (fileset_id) REFERENCES dsFileset(fileset_id)
) Engine=InnoDB DEFAULT CHARSET=latin1;

#
# labels for the type specific columns for each Data Store file type
#

CREATE TABLE dsFileType (
    type        VARCHAR(64),
    type_col_0  varchar(64),
    type_col_1  varchar(64),
    type_col_2  varchar(64),
    type_col_3  varchar(64)
) Engine=InnoDB DEFAULT CHARSET=latin1;

# type specific column for type chip
INSERT INTO dsFileType (type, type_col_0) VALUES('chip', 'chipname');
INSERT INTO dsFileType (type, type_col_0) VALUES('dbinfo', 'component');
INSERT INTO dsFileType (type, type_col_0) VALUES('text', 'component');
INSERT INTO dsFileType (type, type_col_0) VALUES('tgz', 'component');

# none of these types have any type specific columns
#INSERT INTO dsFileType (type) VALUES('psrequest');
#INSERT INTO dsFileType (type) VALUES('psresults');
#INSERT INTO dsFileType (type) VALUES('pstamp');
