#!/usr/bin/env perl

# query to check for duplicate directory entries
# select count(*) from (select so.so_id, so.ext_id, so.dir_id from storage_object as so join (select dir_id, dirname, parent_id, count(*) from directory group by dirname, parent_id  having count(*) > 1) as foo on so.dir_id = foo.dir_id) as foo;

use strict;
use warnings;

use Nebulous::Key qw( parse_neb_key );
use Nebulous::Server;
use Nebulous::Server::Config;
use File::Basename qw( basename );

my $config = Nebulous::Server::Config->new(
    trace       => 'warn',
);
$config->add_db(
    dbindex     => 0,
    dsn         => 'DBI:mysql:database=nebulous:host=localhost',
    dbuser      => 'nebulous',
    dbpasswd    => '@neb@',
);

my $neb = Nebulous::Server->new_from_config($config);

my $db = $neb->_db_for_index(0);

my $n = 0;
#{
#    my $query = $db->prepare("SELECT COUNT(*) as n FROM storage_object");
#    $query->execute;
#    $n = $query->fetchrow_hashref->{'n'};
#}

# repair directory duplication
#my $query = $db->prepare_cached("select so.so_id, so.ext_id, so.dir_id from storage_object as so join (select dir_id, dirname, parent_id, count(*) from directory group by dirname, parent_id  having count(*) > 1) as foo on so.dir_id = foo.dir_id limit 1000");

# initial directory fill in
my $work_query = $db->prepare_cached("SELECT so_id, ext_id, dir_id FROM storage_object AS so WHERE so.dir_id = 0 LIMIT 1000");

my $update_query = $db->prepare_cached("UPDATE storage_object SET ext_id_basename = ?, dir_id = ? WHERE so_id = ?");

# turn of fkeys, otherwise we can change storage_object.dir_id to a
# non-existant value
$db->do("SET FOREIGN_KEY_CHECKS=0");
#$db->do("UPDATE storage_object SET dir_id = 0");

# completely reset the directory table
#$db->do("DELETE FROM directory");
#$db->do("ALTER TABLE directory AUTO_INCREMENT = 1");
# make sure these duplicates can't happen again
#$db->do("alter table directory add unique key(dirname,parent_id)");
# seed the root ('/') directory
#$db->do("INSERT INTO directory VALUES(1, '/', 1)");

my $i = 0;
while ($work_query->execute and $work_query->rows) {
    while (my $row = $work_query->fetchrow_hashref) {
        $i++;
        my $key = parse_neb_key($row->{'ext_id'});
        my $parent_id = $neb->_resolve_dir_parent_id(key => $key, create => 1);

#printf("dirizing %20s basename: %20s parent_id %10d\n", $key, basename($row->{'ext_id'}), $parent_id);
#        printf("$i dirizing %s\n", $key->path);

        $update_query->execute(basename($row->{'ext_id'}), $parent_id, $row->{'so_id'});
    }
    $db->commit;
    printf("### COMMIT ###\n");
    $work_query->finish;
    printf("dirized $i\n");
}

$db->do("SET FOREIGN_KEY_CHECKS=1");
$db->commit;
