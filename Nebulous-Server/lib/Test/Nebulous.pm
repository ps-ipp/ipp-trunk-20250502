# Copyright (C) 2004  Joshua Hoblitt
#
# $Id: Nebulous.pm,v 1.4 2008-09-11 22:35:52 jhoblitt Exp $

package Test::Nebulous;

use strict;

our $VERSION = '0.01';

use base qw( Exporter );

use Cache::Memcached;
use DBI;
use File::Path qw( mkpath rmtree );
use File::Temp qw( tempdir );
use Nebulous::Server::SQL;

our @EXPORT = qw( $NEB_DB $NEB_USER $NEB_PASS $NEB_MEMCACHED_SERVERS);

# require use of the test database server:

our $NEB_DB     = "DBI:mysql:database=test:host=localhost:mysql_socket=/var/run/mysqld/mysqld.sock";
our $NEB_USER   = "test";
our $NEB_PASS   = '';
our $NEB_MEMCACHED_SERVERS = ['127.0.0.1:11211'];

my $dbh = DBI->connect( $NEB_DB, $NEB_USER, $NEB_PASS );
my $sql = Nebulous::Server::SQL->new;

# suppress uninitalized warnings
my $dir1 = "";
my $dir2 = "";
my $dir3 = "";
my $dir4 = "";
my $dir5 = "";
my $dir6 = "";
my $dir7 = "";
my $dir8 = "";

sub show_setup {
    my $self = shift;

    print "DB: $NEB_DB, USER: $NEB_USER, PASS: $NEB_PASS\n";
}


sub setup {
    my $self = shift;

    $self->cleanup;

    # create directories after cleanup
    $dir1 = tempdir( CLEANUP => 0 );
    $dir2 = tempdir( CLEANUP => 0 );
    $dir3 = tempdir( CLEANUP => 0 );
    $dir4 = tempdir( CLEANUP => 0 );
    $dir5 = tempdir( CLEANUP => 0 );
    $dir6 = tempdir( CLEANUP => 0 );
    $dir7 = tempdir( CLEANUP => 0 );
    $dir8 = tempdir( CLEANUP => 0 );

    foreach my $statement (@{ $sql->get_db_schema }) {
        $dbh->do( $statement );
    }

    # generate 2 cabinets so we can replicate to a valid location
    $dbh->do(qq{ INSERT INTO cabinet (name, site_id, location) values ("cab01", 1, "main") });
    $dbh->do(qq{ INSERT INTO cabinet (name, site_id, location) values ("cab02", 1, "main") });
    $dbh->do(qq{ INSERT INTO cabinet (name, site_id, location) values ("cab03", 2, "back") });
    $dbh->do(qq{ INSERT INTO cabinet (name, site_id, location) values ("cab04", 2, "back") });

    # node01 : allocate = TRUE (cab01, site 1)
    $dbh->do(qq{ INSERT INTO volume (vol_id, cab_id, name, host, path, mountpoint, allocate, available) VALUES (1, 1, 'node01', 'node01', ?, '/', TRUE, TRUE) }, undef, $dir1);
    $dbh->do(qq{ INSERT INTO mountedvol SELECT vol_id,name,host,path,allocate,available,xattr,mountpoint, 10e10, 10e7, note FROM volume WHERE vol_id = ? }, undef, 1);

    # node02 : allocate = TRUE (cab02, site 1)
    $dbh->do(qq{ INSERT INTO volume (vol_id, cab_id, name, host, path, mountpoint, allocate, available) VALUES (2, 2, 'node02', 'node02', ?, '/', TRUE, TRUE) }, undef, $dir2);
    $dbh->do(qq{ INSERT INTO mountedvol SELECT vol_id,name,host,path,allocate,available,xattr,mountpoint, 10e10, 10e8, note FROM volume WHERE vol_id = ? }, undef, 2);

    # node03: allocate = TRUE (cab03, site 2)
    $dbh->do(qq{ INSERT INTO volume (vol_id, cab_id, name, host, path, mountpoint, allocate, available) VALUES (3, 3, 'node03', 'node03', ?, '/', TRUE, TRUE) }, undef, $dir3);
    $dbh->do(qq{ INSERT INTO mountedvol SELECT vol_id,name,host,path,allocate,available,xattr,mountpoint, 10e10, 10e8, note FROM volume WHERE vol_id = ? }, undef, 3);

    # node04: allocate = FALSE, available = FALSE
    $dbh->do(qq{ INSERT INTO volume (vol_id, cab_id, name, host, path, mountpoint, allocate, available) VALUES (4, 1, 'node04', 'node04', ?, '/', FALSE, FALSE) }, undef, $dir4);
    $dbh->do(qq{ INSERT INTO mountedvol SELECT vol_id,name,host,path,allocate,available,xattr,mountpoint, 10e10, 10e8, note FROM volume WHERE vol_id = ? }, undef, 4);

    # node05: allocate = FALSE, available = TRUE
    $dbh->do(qq{ INSERT INTO volume (vol_id, cab_id, name, host, path, mountpoint, allocate, available) VALUES (5, 1, 'node05', 'node05', ?, '/', FALSE, TRUE) }, undef, $dir5);
    $dbh->do(qq{ INSERT INTO mountedvol SELECT vol_id,name,host,path,allocate,available,xattr,mountpoint, 10e10, 10e8, note FROM volume WHERE vol_id = ? }, undef, 5);

    # node06: allocate = TRUE, available = FALSE
    $dbh->do(qq{ INSERT INTO volume (vol_id, cab_id, name, host, path, mountpoint, allocate, available) VALUES (6, 1, 'node06', 'node06', ?, '/', TRUE, FALSE) }, undef, $dir6);
    $dbh->do(qq{ INSERT INTO mountedvol SELECT vol_id,name,host,path,allocate,available,xattr,mountpoint, 10e10, 10e8, note FROM volume WHERE vol_id = ? }, undef, 6);

    # node07: full
    $dbh->do(qq{ INSERT INTO volume (vol_id, cab_id, name, host, path, mountpoint, allocate, available) VALUES (7, 1, 'node07', 'node07', ?, '/', TRUE, TRUE) }, undef, $dir7);
    $dbh->do(qq{ INSERT INTO mountedvol SELECT vol_id,name,host,path,allocate,available,xattr,mountpoint, 10e10, 10e10, note FROM volume WHERE vol_id = ? }, undef, 7);
    
    # node08: allocate = TRUE, (cab04, site 2)
    $dbh->do(qq{ INSERT INTO volume (vol_id, cab_id, name, host, path, mountpoint, allocate, available) VALUES (8, 4, 'node08', 'node08', ?, '/', TRUE, TRUE) }, undef, $dir8);
    $dbh->do(qq{ INSERT INTO mountedvol SELECT vol_id,name,host,path,allocate,available,xattr,mountpoint, 10e10, 10e8, note FROM volume WHERE vol_id = ? }, undef, 8);

#   $dbh->do(qq{ call getmountedvol() });
}

sub cleanup {
    # memcached needs to be emptied between test runs
    Cache::Memcached->new(servers => $NEB_MEMCACHED_SERVERS)->flush_all;
    # if flush_all() turns out to be useless as feared
#   my $memd = Cache::Memcached->new(servers => $NEB_MEMCACHED_SERVERS);
#   my $query = $dbh->prepare("SELECT ext_id FROM storage_object");
#   if ($query->execute) {
#        while (my $row = $query->fetchrow_hashref) {
#            my $ext_id = $row->{ext_id};
#            warn "found key $ext_id\n" if $memd->get($ext_id);
#            warn "deleted $ext_id\n" if $memd->delete($ext_id);
#        }
#   }
#   $query->finish;

    
    foreach my $statement (@{ $sql->get_db_clear }) {
        $dbh->do( $statement );
    }

    # on the first call to setup the $dir[12] will be ""
    rmtree([$dir1], 0, 1) if -e $dir1;
    rmtree([$dir2], 0, 1) if -e $dir2;
    rmtree([$dir3], 0, 1) if -e $dir3;
    rmtree([$dir4], 0, 1) if -e $dir4;
    rmtree([$dir5], 0, 1) if -e $dir5;
    rmtree([$dir6], 0, 1) if -e $dir6;
    rmtree([$dir7], 0, 1) if -e $dir7;
    rmtree([$dir8], 0, 1) if -e $dir8;
}

# function to change state of one node for tests:
sub switch_node_state {
    my $self = shift;

    $dbh->do(qq{ UPDATE volume set available = FALSE where name = 'node02'});
    $dbh->do(qq{ UPDATE mountedvol set available = FALSE where name = 'node02'});
}

sub get_node_dirs {
    my $self = shift;

    return ($dir1, $dir2, $dir3, $dir4, $dir5, $dir6, $dir7, $dir8);
}

1;

__END__
