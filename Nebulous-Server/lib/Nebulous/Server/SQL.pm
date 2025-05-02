# Copyright (c) 2004  Joshua Hoblitt
#
# $Id: SQL.pm,v 1.78 2008-12-14 22:54:25 eugene Exp $

package Nebulous::Server::SQL;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '0.04';

use base qw( Class::Accessor::Fast );

my %sql = (
    set_transaction_model   => qq{
        SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED
    },
    update_instance_uri => qq{
        UPDATE instance
        SET vol_id = ?, uri = ?
        WHERE ins_id = ?
    },
    last_insert_id      => qq{
        SELECT LAST_INSERT_ID()
    },
    new_object          => qq{
        INSERT INTO storage_object
        (so_id, ext_id, ext_id_basename, type, dir_id)
        VALUES (?, ?, ?, 'REG_FILE', ?)
    },
    new_object_attr  => qq{
        INSERT INTO storage_object_attr
        (so_id, read_lock, write_lock)
        VALUES (?, 0, NULL)
    },
    delete_object       => qq{
        DELETE FROM storage_object
        WHERE storage_object.so_id = ?
    },
    new_object_instance => qq{
        INSERT INTO instance 
        (so_id, vol_id, uri)
        VALUES (LAST_INSERT_ID(), ?, 'error')
    },
    new_instance        => qq{
        INSERT INTO instance
        (so_id, vol_id, uri)
        VALUES (?, ?, 'error')
    },
    get_object          => qq{
        SELECT
            so_id,
            ext_id,
            read_lock,
            write_lock,
            epoch,
            mtime
        FROM storage_object
        JOIN storage_object_attr
        USING (so_id)
        WHERE ext_id = ?
    },
    get_directory       => qq{
        SELECT
            dir_id
        FROM directory
        WHERE parent_id = ?
            AND dirname = ?
        LOCK IN SHARE MODE
    },
    new_directory       => qq{
        INSERT INTO directory
        (dirname, parent_id)
        VALUES (?, ?)
    },
    check_object_name => qq{
        SELECT
            so_id,
            ext_id
        FROM storage_object
        WHERE ext_id = ?
    },
    stat_object          => qq{
        SELECT
            so.so_id,
            so.ext_id,
            attr.read_lock,
            attr.write_lock,
            attr.epoch,
            attr.mtime,
            SUM(available) as available,
            COUNT(instance.so_id) as instances
        FROM storage_object as so
        JOIN storage_object_attr as attr
            USING (so_id)
        JOIN instance
            USING (so_id)
        LEFT JOIN mountedvol
            USING(vol_id)
        WHERE ext_id = ?
        GROUP BY so.so_id
    },
    # Note: this sets an update lock
    get_object_locks    => qq{
        SELECT
            storage_object.so_id,
            read_lock,
            write_lock 
        FROM storage_object
        JOIN storage_object_attr
        USING (so_id)
        WHERE ext_id = ?
        FOR UPDATE
    },
    new_object_xattr  => qq{
        INSERT INTO storage_object_xattr
            SELECT
                so_id,
                ?,
                ?        
            FROM storage_object
            WHERE ext_id = ?
    },
    replace_object_xattr  => qq{
        REPLACE INTO storage_object_xattr
            SELECT
                so_id,
                ?,
                ?        
            FROM storage_object
            WHERE ext_id = ?
    },
    list_object_xattr    => qq{
        SELECT storage_object_xattr.name
        FROM storage_object
        JOIN storage_object_xattr
        USING (so_id)
        WHERE ext_id = ?
    },
    get_object_xattr    => qq{
        SELECT storage_object_xattr.value
        FROM storage_object
        JOIN storage_object_xattr
        USING (so_id)
        WHERE ext_id = ?
            AND name = ?
    },
    remove_object_xattr    => qq{
        DELETE FROM storage_object_xattr
        WHERE so_id = (SELECT so_id from storage_object where ext_id = ?)
            AND name = ?
    },
    set_write_lock      => qq{
        UPDATE storage_object_attr
        JOIN storage_object
        USING(so_id)
        SET write_lock = 'write'
        WHERE storage_object.ext_id = ?
    },
    delete_write_lock   => qq{
        UPDATE storage_object_attr
        JOIN storage_object
        USING(so_id)
        SET write_lock = NULL
        WHERE ext_id = ?
    },
    increment_read_lock => qq{
        UPDATE storage_object_attr
        JOIN storage_object
        USING(so_id)
        SET read_lock = read_lock + 1
        WHERE ext_id = ?
    },
    decrement_read_lock => qq{
        UPDATE storage_object_attr
        JOIN storage_object
        USING(so_id)
        SET read_lock = read_lock - 1
        WHERE ext_id = ?
    },
    delete_instance_by_ins_id => qq{
        DELETE FROM instance
        WHERE ins_id = ?
    },
    get_object_from_uri   => qq{
        SELECT so_id
        FROM instance
        WHERE uri = ?
    },
    get_instance_by_uri   => qq{
        SELECT ins_id
        FROM instance
        WHERE
            so_id = ?
            AND uri = ?
    },
    get_instance_count   => qq{
        SELECT count(ins_id)
        FROM instance
        WHERE so_id = ?
    },
    get_instance_count_by_ext_id   => qq{
        SELECT 
            so_id,
            count(ins_id) as total,
            sum(available) as available
        FROM instance
        JOIN storage_object
            USING(so_id)
        LEFT JOIN mountedvol
            USING(vol_id)
        WHERE
            ext_id = ?
        GROUP BY so_id
    },
    copy_dead_instances_to_deleted => qq{
        INSERT INTO deleted 
        SELECT vol_id, uri, NULL
        FROM instance
        LEFT JOIN mountedvol
            USING(vol_id)
        WHERE
            so_id = ?
            AND (mountedvol.available IS NULL || mountedvol.available = 0)
    },
    find_dead_instances_by_so_id => qq{
        SELECT so_id, ins_id, vol_id, uri
        FROM instance
        LEFT JOIN mountedvol
            USING(vol_id)
        WHERE
            so_id = ?
            AND (mountedvol.available IS NULL || mountedvol.available = 0)
    },
    get_object_instances    => qq{
        SELECT
            storage_object.so_id,
            uri,
            mountedvol.available,
            vol_id,
            cab_id
        FROM storage_object
        JOIN instance
            USING (so_id)
        JOIN mountedvol
            USING(vol_id)
        JOIN volume
            USING(vol_id)
        WHERE ext_id = ?
            AND mountedvol.available = ?
    },
    get_object_instances_by_proximity  => qq{
        SELECT
            storage_object.so_id,
            uri,
            mountedvol.available,
            vol_id,
            cab_id,
            (ABS(vol_id - ?) + 20 * ABS(cab_id - ?) + 100 * ABS(site_id - ?)) AS vol_idx
        FROM storage_object
        JOIN instance
            USING (so_id)
        JOIN mountedvol
            USING(vol_id)
        JOIN volume
            USING(vol_id)
        JOIN cabinet
            USING(cab_id)
        WHERE ext_id = ?
          AND mountedvol.available = ?
        ORDER BY vol_idx ASC
    },
    get_object_instances_by_vol_name => qq{
        SELECT
            storage_object.so_id,
            uri,
            vol_id,
            cab_id
        FROM storage_object
        JOIN instance
            USING (so_id)
        JOIN mountedvol
            USING(vol_id)
        JOIN volume
            USING(vol_id)
        WHERE ext_id = ?
            AND mountedvol.name = ?
            AND mountedvol.available = ?
    },
    get_ext_id_by_vol_name => qq{
        SELECT
            ext_id
        FROM instance
        JOIN storage_object
            USING (so_id)
        JOIN mountedvol
            USING(vol_id)
        JOIN volume
            USING(vol_id)
        WHERE volume.name = ?
            AND mountedvol.available = ?
        LIMIT ?
    },
    # volume handler
    get_storage_volume_by_name   => qq{
        SELECT
            vol_id,
            host,
            path,
            xattr,
            total - used as free
        FROM mountedvol
        WHERE
            used / total < ?
            AND name = ?
            AND available = ?
            AND allocate = ?
        ORDER BY free DESC
        LIMIT 1
    },
    # volume handler
    get_cabinets_for_ext_id            => qq{
        SELECT DISTINCT 
            volume.cab_id, site_id
        FROM instance 
        JOIN volume ON (instance.vol_id = volume.vol_id)
        JOIN cabinet ON (volume.cab_id  = cabinet.cab_id)
        JOIN storage_object USING(so_id) 
        WHERE ext_id = ?
    },
    get_replication_volume_for_ext_id    => qq{
        SELECT * FROM (
        SELECT
            m.vol_id,
            m.host,
            m.path,
            m.xattr,
            total - used as free
        FROM mountedvol AS m
        JOIN volume AS v USING(vol_id)
        JOIN cabinet AS c USING(cab_id)
        LEFT JOIN (
                   SELECT
                       instance.vol_id,
                       so_id
                   FROM instance
                   JOIN volume 
                   ON instance.vol_id = volume.vol_id
		   WHERE so_id = (
				  SELECT so_id
				  FROM storage_object
				  WHERE ext_id = ?
				 )
                  ) AS i
            ON m.vol_id = i.vol_id
         WHERE
             i.vol_id IS NULL
             AND used / total < ?
             AND m.available = ?
             AND m.allocate = ?
--             AND m.xattr = 0
             AND ( (v.cab_id IS NULL) ||
                   (v.cab_id != ?) )
             AND ( (c.site_id IS NULL) ||
                   (c.site_id != ?) )
         ORDER BY free DESC
         LIMIT ?) as topfew
         ORDER BY RAND()
         LIMIT 1
    },
    # volume handler
    # This has a hack to get around the lack of location awareness by using xattr.
    get_storage_volume          => qq{
        SELECT * from (
        SELECT
            vol_id,
            host,
            path,
            xattr,
            total - used as free
        FROM mountedvol
        WHERE
            used / total < ?
            AND available = ?
            AND allocate = ?
            AND xattr = 0
        ORDER BY free DESC
        LIMIT ?) as topfew
        ORDER BY RAND()
        LIMIT 1
    },
    new_cabinet         => qq{
        INSERT INTO cabinet (name, location, site_id, cab_id)
        VALUES (?, ?, ?, NULL)
    },
    update_cabinet      => qq{
        UPDATE cabinet SET 
           location = ?,
           name     = ?,
           site_id  = ?
        WHERE cab_id = ?
    },
    new_volume          => qq{
        INSERT INTO volume (name, host, path, allocate, available, xattr, mountpoint, cab_id, note)
        VALUES (?, ?, ?, TRUE, TRUE, FALSE, ?, NULL, ?)
    },
    new_alias          => qq{
        INSERT INTO aliasvol (alias_id, alias, name, vol_id)
        VALUES (NULL, ?, ?, ?)
    },
    update_alias       => qq{
        UPDATE alias SET
          vol_id = ?,
          name   = ?
        WHERE alias_id = ?
        AND   alias    = ?
    },
    get_volume_by_name => qq{
        SELECT vol_id, name, host, path
        FROM volume
        WHERE name = ?
    },
    get_volume_by_alias => qq{
        SELECT vol_id, name, host, path
        FROM aliasvol
        JOIN volume USING(vol_id,name)
        WHERE alias = ?
    },
    get_site_info_by_name => qq{
        SELECT vol_id, cab_id, site_id
        FROM volume
        JOIN cabinet USING(cab_id)
        WHERE volume.name = ?
    },
    get_volumes => qq{
        SELECT
            v.vol_id,
            v.name,
            v.host,
            v.path,
            v.allocate,
            v.available,
            v.xattr,
            mountedvol.vol_id IS NOT NULL as mounted,
            v.cab_id,
            v.last_modified,
            v.note
        FROM volume AS v
        LEFT JOIN mountedvol
            USING(vol_id)
    },
    find_object_by_ext_id => qq{
        SELECT so_id, ext_id, ext_id_basename
        FROM storage_object
        WHERE ext_id = ?
    },
    find_object_by_dir_id => qq{
        SELECT ext_id, ext_id_basename
        FROM storage_object
        WHERE dir_id = ?
    },
    find_dir_by_parent_id => qq{
        SELECT dir_id, dirname
        FROM directory
        WHERE parent_id = ?
    },
    rename_object => qq{
        UPDATE storage_object
        SET ext_id = ?, ext_id_basename = ?, dir_id = ?
        WHERE ext_id = ?
    },
    find_objects_with_unavailable_instances => qq{
        SELECT
            storage_object.so_id,
            ext_id,
            count(ins_id) as instances,
            volume.name as volume_name,
            volume.host as volume_host,
            count(mymountedvol.vol_id) as available_instances,
            count(mymountedvol.vol_id) > 0 as recoverable,
            storage_object_xattr.value as copies
        FROM storage_object
        JOIN instance
            USING(so_id)
        JOIN volume
            USING(vol_id)
        LEFT JOIN storage_object_xattr
            ON storage_object.so_id = storage_object_xattr.so_id
        JOIN mymountedvol
            USING(vol_id)
        WHERE mymountedvol.available = 1
--        WHERE storage_object_xattr.name = 'user.copies'
        GROUP BY so_id
        HAVING available_instances < instances OR instances < copies
    },
    find_objects_with_extra_instances_by_xattr => qq{
        SELECT
            so.so_id,
            so.ext_id,
            count(ins_id) as instances,
            mv.name as volume_name,
            mv.host as volume_host,
            count(mv.vol_id) as available_instances,
            xattr.value as copies
        FROM storage_object AS so
        JOIN storage_object_xattr as xattr
            ON so.so_id = xattr.so_id
            AND xattr.name = 'user.copies'
        JOIN instance AS i
            ON so.so_id = i.so_id
        JOIN mountedvol AS mv
            USING(vol_id)
        WHERE
            mv.available = 1
        GROUP BY so_id
        HAVING available_instances > copies
        limit 5;
    },
    find_objects_with_extra_instances => qq{
        SELECT
            storage_object.so_id,
            ext_id,
            count(ins_id) as instances,
            volume.name as volume_name,
            volume.host as volume_host,
            count(mymountedvol.vol_id) as available_instances,
            count(mymountedvol.vol_id) > 0 as recoverable,
            storage_object_xattr.value as copies
        FROM storage_object
        JOIN instance
            USING(so_id)
        JOIN volume
            USING(vol_id)
        LEFT JOIN storage_object_xattr
            ON storage_object.so_id = storage_object_xattr.so_id
        JOIN mymountedvol
            USING(vol_id)
        WHERE
            mymountedvol.available = 1
            AND storage_object_xattr.name = 'user.copies'
        GROUP BY so_id
        HAVING available_instances > copies
    },
    get_mounted_volumes => qq{
        SELECT mountpoint, total, used, vol_id, name, host, path, allocate, available, xattr FROM mountedvol ORDER BY host, name
    },
);

{
    my @schema;

    local $/ = '###';

    foreach my $statement (<DATA>) {
        last unless ( $statement =~ /\S+/ );
        $statement =~ s/###//g;
        push @schema, $statement;
    }

    $sql{get_db_schema} = \@schema;
}

{
    my @clear = split /;/, <<END;
SET FOREIGN_KEY_CHECKS=0;
DROP TABLE IF EXISTS storage_object;
DROP TABLE IF EXISTS storage_object_attr;
DROP TABLE IF EXISTS storage_object_xattr;
DROP TABLE IF EXISTS instance;
DROP TABLE IF EXISTS lock_record;
DROP TABLE IF EXISTS volume;
DROP TABLE IF EXISTS mountedvol;
DROP TABLE IF EXISTS log;
DROP TABLE IF EXISTS directory;
DROP TABLE IF EXISTS deleted;
DROP TABLE IF EXISTS cabinet;
DROP TABLE IF EXISTS aliasvol;
DROP TABLE IF EXISTS lost_instances;
SET FOREIGN_KEY_CHECKS=1
END
#DROP PROCEDURE IF EXISTS getmountedvol;
    $sql{get_db_clear} = \@clear;
}

__PACKAGE__->mk_ro_accessors( keys %sql );

sub new {
    my $class = shift;

    my $self = \%sql;

    bless $self, $class || ref $class;

    return $self;
}

1;

# ###
# 
# CREATE PROCEDURE getmountedvol() DETERMINISTIC
# BEGIN
#     DECLARE done BOOLEAN DEFAULT FALSE;
#     DECLARE vol_idvar INT;
#     DECLARE namevar VARCHAR(255);
#     DECLARE hostvar VARCHAR(255);
#     DECLARE pathvar VARCHAR(255);
#     DECLARE allocatevar BOOLEAN;
#     DECLARE availablevar BOOLEAN;
#     DECLARE xattrvar BOOLEAN;
#     DECLARE trans_level VARCHAR(255);
#     DECLARE key_checks BOOLEAN;
#     DECLARE cur1 CURSOR FOR SELECT vol_id, name, host, path, allocate, available, xattr FROM myvolume;
#     DECLARE CONTINUE HANDLER FOR SQLSTATE '02000' SET done = TRUE;
# 
#     -- store the okey checking state
# --    SELECT @@FOREIGN_KEY_CHECKS INTO key_checks; 
#     -- disable foregin check checks to prevent deadlocks on the mountedvol table
# --    SET FOREIGN_KEY_CHECKS=0;
# 
#     -- make sure the temp table does not already exist... this can happy if the
#     -- stored proc fails for some reason
#     DROP TABLE IF EXISTS myvolume;
#     CREATE TEMPORARY TABLE myvolume LIKE volume;
#     INSERT INTO myvolume SELECT * FROM volume;
# 
#     -- store the current transaction level
# --    SELECT @@session.tx_isolation INTO trans_level; 
#     -- set trans level to repeatable-read so the volume table does not change
#     -- out from under our cursor
# --    SET @@session.tx_isolation = 'REPEATABLE-READ';
# 
#     -- iterate over the volume table finding the coresponding entry in the
#     -- mount table and inserting union of the volume & mount row into the
#     -- mountedvol table
#     OPEN cur1;
# 
#     myloop: LOOP
#         FETCH cur1 INTO vol_idvar, namevar, hostvar, pathvar, allocatevar, availablevar, xattrvar;
#         IF `done` THEN LEAVE myloop; END IF;
#         REPLACE INTO mountedvol
#             SELECT mountpoint, total, used, vol_idvar, namevar, hostvar, pathvar, allocatevar, availablevar, xattrvar
#             FROM
#                 (SELECT *, INSTR(pathvar, mountpoint) = 1 as substring
#                 FROM mount
#                 HAVING substring = 1
#                 ORDER BY substring DESC, LENGTH(mountpoint) DESC
#                 LIMIT 1) as bar;
#     END LOOP myloop;
#     
#     CLOSE cur1;
# 
#     -- restore the original transaction level
# --    SET @@session.tx_isolation = trans_level;
# 
#     -- restore the original key checking state
# --    SET @@FOREIGN_KEY_CHECKS = key_checks; 
# 
#     DROP TABLE IF EXISTS myvolume;
# 
# --    SET FOREIGN_KEY_CHECKS=1;
# 
#     COMMIT;
# END 

__DATA__
CREATE TABLE directory (
    dir_id BIGINT NOT NULL AUTO_INCREMENT,
    dirname CHAR(255) NOT NULL,
    parent_id BIGINT NOT NULL,
    FOREIGN KEY(parent_id) REFERENCES directory(dir_id),
    PRIMARY KEY(dir_id),
    KEY(parent_id),
    KEY(dirname),
    UNIQUE(dirname, parent_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

INSERT INTO directory (dir_id, dirname, parent_id) VALUES (1, '/', 1);

###

CREATE TABLE storage_object (
    so_id BIGINT NOT NULL AUTO_INCREMENT,
    ext_id VARCHAR(255) NOT NULL,
    ext_id_basename VARCHAR(255) NOT NULL,
    dir_id BIGINT NOT NULL,
    FOREIGN KEY(dir_id) REFERENCES directory(dir_id),
    type enum('REG_FILE'),
    PRIMARY KEY(so_id),
    UNIQUE KEY(ext_id),
    KEY(dir_id),
    KEY(type)
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE storage_object_attr (
    so_id BIGINT NOT NULL AUTO_INCREMENT,
    FOREIGN KEY(so_id) REFERENCES storage_object(so_id) ON DELETE CASCADE,
    read_lock TINYINT DEFAULT 0 NOT NULL,
    write_lock ENUM( 'write' ),
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    mtime TIMESTAMP,
    PRIMARY KEY(so_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE storage_object_xattr (
    so_id BIGINT NOT NULL AUTO_INCREMENT,
    FOREIGN KEY(so_id) REFERENCES storage_object(so_id) ON DELETE CASCADE,
    name VARCHAR(255),
    value BLOB,
    PRIMARY KEY(so_id, name),
    KEY(so_id),
    KEY(name(64))
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE lock_record (
    so_id BIGINT NOT NULL,
    FOREIGN KEY(so_id) REFERENCES storage_object(so_id),
    type ENUM( 'read', 'write' ) NOT NULL,
    epoch TIMESTAMP,
    KEY(so_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE cabinet (
    cab_id INT NOT NULL AUTO_INCREMENT,
    name VARCHAR(255) NOT NULL,
    site_id INT NOT NULL DEFAULT 0,
    location VARCHAR(255),
    PRIMARY KEY(cab_id),
    UNIQUE KEY(name),
    KEY (site_id),
    KEY (location)
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE volume (
    vol_id INT NOT NULL AUTO_INCREMENT,
    name VARCHAR(255) NOT NULL,
    host VARCHAR(255) NOT NULL,
    path VARCHAR(255) NOT NULL,
    allocate BOOLEAN DEFAULT FALSE,
    available BOOLEAN DEFAULT FALSE,
    xattr BOOLEAN DEFAULT FALSE,
    mountpoint VARCHAR(255) NOT NULL,
    cab_id INT,
    note VARCHAR(255),
    last_modified TIMESTAMP NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP, # SC: Added by Haydn with ALTER TABLE(?) on 2012-12-03
    PRIMARY KEY(vol_id),
    UNIQUE KEY(name),
    UNiQUE KEY(path),
    KEY(host(16)),
    KEY(allocate),
    KEY(available),
    FOREIGN KEY(cab_id) REFERENCES cabinet(cab_id),
    KEY(mountpoint(255))
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE mountedvol(
    vol_id INT NOT NULL,
    FOREIGN KEY(vol_id) REFERENCES volume(vol_id) ON DELETE CASCADE,
    name VARCHAR(255) NOT NULL,
    host VARCHAR(255) NOT NULL,
    path VARCHAR(255) NOT NULL,
    FOREIGN KEY(path) REFERENCES volume(path) ON DELETE CASCADE,
    allocate BOOLEAN DEFAULT FALSE,
    available BOOLEAN DEFAULT FALSE,
    xattr BOOLEAN DEFAULT FALSE,
    mountpoint VARCHAR(255) NOT NULL,
    FOREIGN KEY(mountpoint) REFERENCES volume(mountpoint) ON DELETE CASCADE,
    total BIGINT NOT NULL,
    used BIGINT NOT NULL,
    note VARCHAR(255),
    PRIMARY KEY(vol_id),
    KEY(name),
    KEY(host),
    KEY(path),
    KEY(allocate),
    KEY(available),
    KEY(xattr),
    KEY(mountpoint(255))
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE aliasvol (
    alias_id INT NOT NULL AUTO_INCREMENT,
    alias VARCHAR(255) NOT NULL,
    name  VARCHAR(255) NOT NULL,
    vol_id INT NOT NULL,
    PRIMARY KEY(alias_id),
    KEY(alias),
    KEY(name),
    FOREIGN KEY(vol_id) REFERENCES volume(vol_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE instance (
    ins_id BIGINT NOT NULL AUTO_INCREMENT,
    so_id BIGINT NOT NULL,
    FOREIGN KEY(so_id) REFERENCES storage_object(so_id) ON DELETE CASCADE,
    vol_id INT NOT NULL,
    FOREIGN KEY(vol_id) REFERENCES volume(vol_id),
    uri VARCHAR(255) NOT NULL,
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    mtime TIMESTAMP,
    PRIMARY KEY(ins_id),
    KEY(so_id),
    KEY(vol_id),
    KEY(uri(40))
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE lost_instances (
    id BIGINT NOT NULL AUTO_INCREMENT,
    ins_id BIGINT NOT NULL DEFAULT -1,
    so_id BIGINT NOT NULL DEFAULT -1,
    vol_id INT NOT NULL DEFAULT -1,
    uri CHAR(255) NOT NULL DEFAULT 'undefined',
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    mtime TIMESTAMP,
    PRIMARY KEY(id),
    KEY(so_id),
    KEY(vol_id),
    KEY(uri(40))
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE log (
    timestamp TIMESTAMP,
    hostname VARCHAR(255),
    level VARCHAR(255),
    sub VARCHAR(255),
    message VARCHAR(2048) NOT NULL,
    PRIMARY KEY(timestamp)
) ENGINE=innodb DEFAULT CHARSET=latin1;

###

CREATE TABLE deleted (
    vol_id INT NOT NULL,
    FOREIGN KEY(vol_id) REFERENCES volume(vol_id),
    uri VARCHAR(255) NOT NULL,
    timestamp TIMESTAMP,
    PRIMARY KEY(vol_id, uri),
    KEY(vol_id),
    KEY(uri)
) ENGINE=innodb DEFAULT CHARSET=latin1;
