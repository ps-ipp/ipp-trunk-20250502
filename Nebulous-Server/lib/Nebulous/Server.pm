# Copyright (c) 2004-2008  Joshua Hoblitt
#
# $Id: Server.pm,v 1.96 2008-12-14 22:54:25 eugene Exp $

package Nebulous::Server;

use strict;
use warnings FATAL => qw( all );
no warnings qw( uninitialized );

our $VERSION = '0.18';

use base qw( Class::Accessor::Fast );

use Carp;

use Cache::Memcached;
use DBI;

## for gentoo:
# use Digest::SHA1 qw( sha1_hex );

## for ubuntu:
use Digest::SHA qw( sha1_hex );

use Fcntl ':mode';
use File::Basename qw( basename dirname fileparse );
use File::ExtAttr qw( setfattr );
use File::Path;
use File::Spec;
use Log::Log4perl qw( :levels );
use Nebulous::Key qw( parse_neb_key parse_neb_volume );
use Nebulous::Server::Config;
use Nebulous::Server::Log;
use Nebulous::Server::SQL;
use Params::Validate qw( validate validate_pos SCALAR SCALARREF UNDEF BOOLEAN );
use URI::file;

__PACKAGE__->mk_accessors(qw( log sql config cache ));

use constant SUBPATH_DEPTH  	=> 2;
use constant NFS_RETRIES    	=> 100;
use constant NFS_RETRY_WAIT 	=> 1;
use constant TRANS_RETRY_WAIT 	=> 1;

# This is the umask hack.
umask(0002);

# This determines how many entries from the list of volumes sorted by free space are randomized.
my $topfew_count = 15;
my $max_used_space = 0.98;
# transaction restart/retry regex
my $trans_regex = qr/Deadlock Found|Lock wait timeout exceeded|try restarting transaction|Can't connect to MySQL server/i;

sub new
{
    my $class = shift;

    # let Nebulous::Server::Config validate our params
    my $config = Nebulous::Server::Config->new( @_ );

    return $class->new_from_config($config);
}


sub new_from_config
{
    my $class = shift;

    my ($config) = @_;

    # log4perl is not available until we call init()
    Nebulous::Server::Log->init($config);
    my $log = Log::Log4perl::get_logger( "Nebulous::Server" );
    $log->level($config->trace);
    $log->debug( "entered - @_" );

    my $sql = Nebulous::Server::SQL->new;

    my $self = bless {}, ref $class || $class;
    $self->log($log);
    $self->sql($sql);
    $self->config($config);
    $self->cache(
        Cache::Memcached->new({
	    servers => $config->memcached_servers,
			      })
	);
#    $self->cache->set("foo", "bar") or die "set failed";
#    $self->cache->get("foo") or die "get failed";
    $log->logdie("at least one database must be defined") unless $config->n_db;

    # cause a db session to be started
    $self->_db_for_index(0);

    $log->debug( "leaving" );

    return $self;
}

# EAM : In the 'distributed' version of Nebulous, there is a collection of N databases
# the db_index uniquely defines the db used by a given key
sub _db_index_for_key
{
    my $self = shift;

    my $log     = $self->log;
    $log->debug( "entered - @_" );

    my ($key) = @_;

    my $config  = $self->config;

    my $db_index = 0;
    $log->logdie("key not defined") unless defined $key;

    # hash the key to select the correct database instance
    # only use the first 8 hex chars... have to be careful to avoid an int
    # overflow here

    # hash only the directory component of the path and not the filename
    my $path = dirname($key->path);
    $db_index = unpack("h8", sha1_hex($path)) % $config->n_db;

    $log->debug("index is $db_index");
    $log->debug("leaving");

    return $db_index;
}

sub _db_for_index
{
    my $self = shift;

    my $log     = $self->log;
    $log->debug( "entered - @_" );

    my ($db_index) = @_;

    my $sql     = $self->sql;
    my $config  = $self->config;

    # lookup to see if we have a stored dbh for this database
    my $dbh = $self->{dbs}[$db_index];
    # if the dbh is still alive, return it
    if (defined $dbh and $dbh->ping) {
        $log->debug("db handle is still alive");
        return $dbh;
    }
    # otherwise create a new connection
    $log->debug("db handle is dead/unopened");

    # lookup database info
    my $db_config = $config->db($db_index);
    $log->logdie("can't find database configuration info for db # $db_index")
        unless $db_config;

    # if we're running under mod_perl & Apache::DBI is loaded we want to
    # reconnect to the database everytime the dbh is requested.  The rational is
    # that if we're running under mod_perl this is probably a log running
    # processes and the database might have gone away on us.  Apache::DBI will
    # take care of getting a valid dbh back.
    TRANS: while (1) {
        eval {
            $dbh = DBI->connect_cached(
                $db_config->dsn,
                $db_config->dbuser,
                $db_config->dbpasswd,
                {
                    RaiseError => 1,
                    PrintError => 0,
                    AutoCommit => 0,
                },
            );

            $dbh->do( $sql->set_transaction_model );
            $log->debug( "connected to database: ", sub { $dbh->data_sources; } );
            $dbh->commit;
            $log->debug("commit");
        };
        if ($@) {
            $dbh->rollback if $dbh;
            $log->debug("rollback") if $dbh;
            if ($@ =~ qr/Can't connect to MySQL server/) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie( "database error: $@" );
        }
        last;
    }

    $self->{dbs}[$db_index] = $dbh;

    $log->debug("leaving");

    return $dbh;
}

sub db
{
    my $self = shift;

    my $log     = $self->log;
    $log->debug( "entered - @_" );

    my ($key) = validate_pos(@_,
        {
            isa => 'Nebulous::Key',
        },
    );

    my $sql     = $self->sql;
    my $config  = $self->config;

    $log->logdie("key not defined") unless defined $key;
    my $db_index = $self->_db_index_for_key($key);

    my $dbh = $self->_db_for_index($db_index);

    return $dbh;
}

my $get_storage_volume_calls = 0;

sub create_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ($key, $vol_name) = validate_pos(@_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR|UNDEF,
#            callbacks   => {
#                # check that the volume requested is valid
#                'is valid volume name' => sub {
#                    return 1 if not defined $_[0];
#                    $self->_is_valid_volume_name($_[0])
#                },
#            },
            optional    => 1,
        },
    );

    my $sql = $self->sql;

    # vol_name overrides the key implied volume
    eval {
        $key = parse_neb_key($key, $vol_name);
    };
    $log->logdie("$@") if $@;
    $vol_name = $key->volume;

    my $db  = $self->db($key);

    # the key's volume can't be validiated on input for this method so we have
    # to check it after parsing the key
    if (defined $vol_name and not $self->_is_valid_volume_name($key, $key->volume)) {
        unless ($key->hard_volume) {
	    # EAM : lower verbosity level
            $log->debug( "$vol_name is not a known volume name" );
            $vol_name = undef;
        } else {
           $log->logdie("$vol_name is not a valid volume name");
        }
    }
        
    $get_storage_volume_calls = 0;

    my ($vol_id, $vol_host, $vol_path, $vol_xattr)
        = $self->_get_storage_volume($key, $vol_name, $key->hard_volume);

    my $parent_id = $self->_resolve_dir_parent_id(key => $key, create => 1);

    my $uri;
TRANS: while (1) {
        eval {
            {
                # create storage_object
                my $query = $db->prepare_cached( $sql->new_object ); 
# bad syntax    $query->execute('NULL', $key->path, basename($key->path), $parent_id);
                $query->execute(0, $key->path, basename($key->path), $parent_id);
                $query->finish;
            }

            my $so_id;
            {
                # get object ID
                my $query = $db->prepare_cached( $sql->last_insert_id );
                $query->execute;
                ($so_id) = $query->fetchrow_array;
                # XXX finish seems to be required when using LAST_INSERT_ID() or we
                # get a warning about the stmt handling still be active the next
                # time LAST_INSERT_ID() is invoked
                $query->finish;
            }

            {
                # create storage_object_attr
                my $query = $db->prepare_cached( $sql->new_object_attr ); 
                $query->execute($so_id);
                $query->finish;
            }

            {
                
                # create instance with no URI
#            my $query = $db->prepare_cached( $sql->new_instance );
                my $query = $db->prepare_cached( $sql->new_object_instance );
                $query->execute($vol_id);
                $query->finish;
            }

            my $ins_id;
            {
                # get instance ID
                my $query = $db->prepare_cached( $sql->last_insert_id );
                $query->execute;
                ($ins_id) = $query->fetchrow_array;
                # XXX finish seems to be required when using LAST_INSERT_ID() or we
                # get a warning about the stmt handling still be active the next
                # time LAST_INSERT_ID() is invoked
                $query->finish;
            }

            # Unfortunately, since we want to use the instance row's ID as part of the
            # actual on disk file name we can't try to create the file until after
            # we've create both a new storage_storage object and instance.

            # TODO add some stuff here to retry if unsucessful
            $uri = $self->_create_empty_instance_file($key, $so_id, $ins_id, $vol_path, $vol_xattr);
            $log->debug("created $uri on volume ID: $vol_id");

            {
                # update the instance with URI & vol_id that the file is on
                my $query = $db->prepare_cached( $sql->update_instance_uri );
                # vol_id, uri, ins_id
                $query->execute($vol_id, "$uri", $ins_id);
                $query->finish;
            }

            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
#        and not $key->soft_volume
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("error: $@");
        }
        last;
    }

    # add new key to the cache
    $self->cache->set($key->path, 1) if defined $self->cache;
    $log->debug( "key added to cache" );

    $log->debug("leaving");

    return "$uri";
}


sub _resolve_dir_parent_id
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my %p = validate(@_,
        {
            key     => {
                isa         => 'Nebulous::Key',
            },
            create  => {
                type        => BOOLEAN,
                optional    => 1,
                default     => undef,
            },
            # return dir_id instead of parent_id
            dir_id  => {
                type        => BOOLEAN,
                optional    => 1,
                default     => undef,
            },
        }
    );

    my $key = $p{key};

    my $sql = $self->sql;
    my $db  = $self->db($key);

    # no path means '/', which has a dir_id & parent_id of 1
    return 1 if $key->path eq '';

    # resolve parent directory
    my @dirs;

    # File::Spec->splitpath was causing ->splitdir to always an extra dir
    # named "" because of a trailing /
    unless ($p{dir_id}) {
        @dirs = File::Spec->splitdir(dirname($key->path));
    } else {
        @dirs = File::Spec->splitdir($key->path);
    }
    # dirname returns "." if there is no dir component to the path, we have
    # to filter this out
    @dirs = grep(!/^\.$/, @dirs);

    $log->debug("looking for dirs - ", join(" : ", @dirs), "\n");

    # start at the root dir; '/' == 1
    my $parent_id = 1;
    my $dir_id;
TRANS: while (1) {
        eval {
            foreach my $dir (@dirs) {
                $dir_id = undef;
                {
                    my $query = $db->prepare_cached($sql->get_directory); 
                    $query->execute($parent_id, $dir);
                    if ($query->rows) {
                        $dir_id = $query->fetchrow_hashref->{'dir_id'};
                        $log->debug("resolved $dir to dir_id: $dir_id");
                    }
                    $query->finish;
		    $db->commit;
                }

                # if we found a dir_id, a row for this directory already exists
                if (defined $dir_id) {
                    $parent_id = $dir_id;
                    # note that you can't exit an eval {} with next
                    next;
                }

                # else dir doesn't exist
                unless ($p{create}) {
                    # resolution failed
                    $parent_id = undef;
                    last;
                }

                {
                    # dir doesn't exist, create it
                    my $query = $db->prepare_cached($sql->new_directory);
                    $query->execute($dir, $parent_id);
                    $query->finish;
		    $db->commit;
                }

                # get the dir_id of the new directory entry 
                {
                    my $query = $db->prepare_cached($sql->last_insert_id);
                    $query->execute();

                    # the new dir_id will be the parent_id of the next
                    # descendent directory
                    ($parent_id) = $query->fetchrow_array;
                    $query->finish;
		    $db->commit;
                }
                die("failed to get LAST_INSERT_ID()")
                    unless $parent_id;
            }
        };
        if ($@) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                $parent_id = 1;
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            if ($@ =~ qr/Duplicate entry/) {
                $log->warn("Duplicate database entry, retrying transaction: $@");
                $parent_id = 1;
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("error: $@");
        }
        last;
    }

    $log->debug("leaving");

    return $p{dir_id} ? $dir_id : $parent_id;
}


sub rename_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $newkey) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        # make sure the "newkey" doesn't already exist
        {
            type        => SCALAR,
            callbacks   => {
                'is not valid object key' => sub { not $self->_is_valid_object_key($_[0]) },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volumes
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;
    eval {
        $newkey = parse_neb_key($newkey);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    # XXX this may require database migration in the future
    unless ($self->_db_index_for_key($key)
         == $self->_db_index_for_key($newkey)) {
        $log->logdie("can not rename objects across distributed database boundaries");
    }

TRANS: while (1) {
        eval {
            # rename storage_object
            my $query = $db->prepare_cached($sql->rename_object); 
            # this SQL statment takes the new key name as the first param
            my $rows = $query->execute($newkey->path, basename($newkey->path), $self->_resolve_dir_parent_id(key => $newkey, create => 1), $key->path);

            # if we affected more then one row something very bad has happened.
            unless ($rows == 1) {
                $query->finish;
                die("affected row count is $rows instead of 1");
            }

            $self->cache->delete($key->path) if defined $self->cache;
            $self->cache->set($newkey->path, 1) if defined $self->cache;

            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("database error: $@");
        }
        last;
    } 

    $log->debug("leaving");

    return $newkey;
}

sub swap_objects
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key1, $key2) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volumes
    eval {
        $key1 = parse_neb_key($key1);
    };
    $log->logdie("$@") if $@;
    eval {
        $key2 = parse_neb_key($key2);
    };
    $log->logdie("$@") if $@;

    my $dbidx1 = $self->_db_index_for_key($key1);
    my $dbidx2 = $self->_db_index_for_key($key2);
    $log->logdie("cannot swap keys not stored on the same database")
        unless ($dbidx1 == $dbidx2);

    my $dbh1 = $self->_db_for_index($dbidx1);
    my $dbh2 = $self->_db_for_index($dbidx2);
    $log->logdie("different db handles for the same db?")
        unless ($dbh1 == $dbh2);

    # order of operations for the swap with a single db is:
    # key1 -> key1.swap
    # key2 -> key1
    # key1.swap -> key2

    my $db = $dbh1;
  TRANS: while (1) {
        eval {
            {
              # key1 -> key1.swap
              my $query = $db->prepare_cached($sql->rename_object); 
              # this SQL statment takes the new key name as the first param
              # XXX currently using a bogus dir_id -- this may cause a problem
              # someday but it's unlikley as it's contained entirely in the
              # transaction
              my $rows = $query->execute($key1->path . ".swap", basename($key1->path) . ".swap", 1, $key1->path);

              # if we affected more then one row something very bad has happened.
              unless ($rows == 1) {
                  $query->finish;
                  die("affected row count is $rows instead of 1");
              }
          }

          {
              # key2 -> key1
              my $query = $db->prepare_cached($sql->rename_object); 
              # this SQL statment takes the new key name as the first param
              my $rows = $query->execute($key1->path, basename($key1->path), $self->_resolve_dir_parent_id(key => $key1, create => 1), $key2->path);

              # if we affected more then one row something very bad has happened.
              unless ($rows == 1) {
                  $query->finish;
                  die("affected row count is $rows instead of 1");
              }
          }

          {
              # key1.swap -> key2
              my $query = $db->prepare_cached($sql->rename_object); 
              # this SQL statment takes the new key name as the first param
              my $rows = $query->execute($key2->path, basename($key2->path), $self->_resolve_dir_parent_id(key => $key2, create => 1), $key1->path . ".swap");

              # if we affected more then one row something very bad has happened.
              unless ($rows == 1) {
                  $query->finish;
                  die("affected row count is $rows instead of 1");
              }
          }

            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("database error: $@");
        }
      last;
  }

    $log->debug("leaving");

    return 1;
}

# EAM : from JH, below is a possible option to swap instances across
# dbs.  I recommend that this action be disallowed, and instead such
# operations be implemented only as a combination of copy and delete

# order of operations for the swap between two dbs is:
# key1 start transaction
# key1 -> read all instances
# key1 -> remove all instances
# key2 start transaction
# key2 -> read all instances
# key2 -> remove all instances
# key1 -> insert key 2 instances
# key2 -> insert key 1 instances
# key1,2 commit

sub replicate_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $dest_vol_name) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key'
                    => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
    );

    my $sql = $self->sql;

    # if a volume name is explicity specified then we should make the
    # replication onto that volume (even if there is already an instance on that
    # volume) if at all possible and throw an error if we can not.

    # if a volume name IS NOT specified then we should make the replication
    # onto any (hopefully the best) volume that DOES NOT already have an
    # instance on it.  If all available volumes already have an instance on them
    # then we should throw an error 

    # volume names implied as part of the key are *IGNORED* as the source and
    # *SHOULD NOT* be used as the destination either

    eval {
	# EAM 2019.10.31 : if I supply dest_vol_name here, then
	# $key->volume will point at that volume and the section below
	# will correctly be testing the validity of dest_vol_name
        $key = parse_neb_key($key, $dest_vol_name);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    ## Old comment:
    # puke if the source volume is bogus, we may want to actually use this as
    # the instance to be copied later

    # EAM 2019.10.31 : in the past, the key-implied volume was
    # supplied here, which was inconsistent since above it says the
    # key-implied volume is ignored. By supplying $dest_vol_name to
    # parse_neb_key above, the key->volume now refers to the
    # dest_volume
    if (defined $key->volume and not $self->_is_valid_volume_name($key, $key->volume)) {
        unless ($key->hard_volume) {
	    # reduce verbosity
            $log->debug($key->volume . " not a known volume name");
        } else {
           $log->logdie("$key is not a valid volume name");
        }
    }

    # puke if the destination volume is bogus
    ## XXX if I supply dest_vol_name to parse_neb_key above, this section below is redundant with the above
    ## if (defined $dest_vol_name
    ##     and not $self->_is_valid_volume_name($key, $dest_vol_name)) {
    ##        $log->logdie($key->volume . " is not a valid volume name");
    ## }

    $get_storage_volume_calls = 0;

    my ($vol_id, $vol_host, $vol_path, $vol_xattr);
    if (defined $dest_vol_name) {
	# if we supplied dest_vol_name, then the call to parse_neb_key above
	# will have set $key->volume to that value and hard_volume to match
        ($vol_id, $vol_host, $vol_path, $vol_xattr)
            = $self->_get_storage_volume($key, $key->volume, $key->hard_volume);
    } else {
        ($vol_id, $vol_host, $vol_path, $vol_xattr)
            = $self->_get_replication_volume($key);
    }

    # print "selected target: $vol_id, $vol_host\n";

    my $uri;
TRANS: while (1) {
        eval {
            my $so_id;
            {
                # verify that at least one instance is currently available
                my $query = $db->prepare_cached( $sql->get_object_instances );
                my $rows = $query->execute($key->path, 1);

                unless ( $rows > 0 ) {
                    $query->finish;
                    die( "storage object does not exist" );
                }

                $so_id = $query->fetchrow_hashref->{ 'so_id' };
                $query->finish;
		$db->commit;
            }

            {
                my $query = $db->prepare_cached( $sql->new_instance );
                $query->execute($so_id, $vol_id);
            }

            my $ins_id;
            {
                my $query = $db->prepare_cached( $sql->last_insert_id );
                $query->execute();
                ($ins_id) = $query->fetchrow_array;
                # XXX finish seems to be required when using LAST_INSERT_ID() or we
                # get a warning about the stmt handling still being active the next
                # time LAST_INSERT_ID() is invoked
                $query->finish;
		$db->commit;
            }

            # Unfortunately, since we want to use the instance row's ID as part of
            # the actual on disk file name we can't try to create the file until
            # after we've create both a new storage_storage object and instance.

            # TODO add some stuff here to retry if unsucessful
            # XXX if this fails, it should try to generate the
            # instance on another volume (unless !$soft_volume) 
            $uri = $self->_create_empty_instance_file($key, $so_id, $ins_id, $vol_path, $vol_xattr);

            {
                my $query = $db->prepare_cached( $sql->update_instance_uri );
                # vol_id, uri, ins_id
                $query->execute($vol_id, $uri, $ins_id);
            }

            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
            $db->rollback;
            # handle soft volumes
            if (defined $dest_vol_name and not defined $key->hard_volume) {
                $log->debug("retrying with 'any' volume");
                return $self->replicate_object($key->path, 'any');
            }
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("error: $@");
        }
        last;
    }

    # check to see if the user.mode xattr exists
    eval {
        my $mode = $self->getxattr_object("$key", 'user.mode');
        if (defined $mode && $mode ne "") {
	    $self->chmod_object("$key", $mode);
        }
    };
    if ($@) {
        unless ($@ =~ qr/user.mode does not exist/) {
            $log->logdie("error: $@");
        }
    }

    $log->debug("leaving");
    return "$uri";
}


sub prune_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key'
                    => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
    );

    my $sql = $self->sql;

    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    my $rows_removed = 0;
TRANS: while (1) {
        eval {
            # remove key from cache
            $self->cache->delete($key->path) if defined $self->cache;

            my $so_id;
            {
                my $query = $db->prepare_cached( $sql->find_object_by_ext_id );
                $query->execute( $key->path );
                $so_id = $query->fetchrow_hashref->{'so_id'};
                $query->finish;
            }

            # record the path of the innaccesible files for deferred
            # deletion
            my $rows_copied;
            {
                my $query = $db->prepare_cached( $sql->copy_dead_instances_to_deleted );
                $rows_copied = $query->execute( $so_id );
            }

            # check to see if there is anything to be done
            unless ($rows_copied > 0) {
                $db->rollback;
                return;
            }

            # In MySQL you can't select from a table you're deleting rows from so
            # we first have to get a list of instances to be removed, and then
            # remove them.
            my $rows_found;
            {
                my $query = $db->prepare_cached( $sql->find_dead_instances_by_so_id );
                $rows_found = $query->execute( $so_id );
		my $i = 0;
		while (my $row = $query->fetchrow_hashref) {
                    # remove dead instances
#		    $log->warn("copied: $rows_copied found: $rows_found removed: $rows_removed");
                    my $query = $db->prepare_cached( $sql->delete_instance_by_ins_id);
                    $rows_removed += $query->execute( $row->{ins_id} );
                }
                $query->finish;
            }
#	    $log->warn("copied: $rows_copied found: $rows_found removed: $rows_removed");
            # sanity check
            die("instances inaccessible ($rows_copied) != instances removed ($rows_removed)")
                unless $rows_copied == $rows_removed;
            
            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("error: $@");
        }
        last;
    }

    $log->debug("leaving");

    return $rows_removed;
}


sub lock_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ( $key, $type ) = validate_pos( @_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR,
            callbacks   => {
                'is read or write' => sub { $_[0] =~ /^(?:read|write)$/ },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    my $so_id;
    my $read_lock;
    my $write_lock;

TRANS: while (1) {
        eval {
            {
                # this will set update locks
                my $query = $db->prepare_cached( $sql->get_object_locks );
                my $rows = $query->execute( $key->path );
                unless ( $rows == 1 ) {
                    $query->finish;
                    die( "storage object does not exist" );
                }

                my $row = $query->fetchrow_hashref;
                $query->finish;

                $so_id      = $row->{ 'so_id' };
                $read_lock  = $row->{ 'read_lock' };
                $write_lock = $row->{ 'write_lock' };
            }

            if ($type eq 'write') {
                # can't set a write lock twice and
                # can't set a write lock if there are read locks
                if ($write_lock) {
                    die("can not write lock twice -- retry");
                }
                
                if ($read_lock > 0) {
                    die("can not write lock after read lock -- retry");
                }

                {
                    my $query = $db->prepare_cached( $sql->set_write_lock );
                    my $rows = $query->execute($key->path);
                
                    # if we affected more then one row something very bad has happened.
                    unless ($rows == 1) {
                        die("affected row count is $rows instead of 1");
                    }

                }
            } elsif ($type eq 'read') {
                # can't set a read lock if there's a write lock
                if ($write_lock) {
                    die("can not read lock after write lock -- retry");
                }

                {
                    my $query = $db->prepare_cached( $sql->increment_read_lock );
                    my $rows = $query->execute($key->path);
                
                    # if we affected more then one row something very bad has happened.
                    unless ($rows == 1) {
                        die("affected row count is $rows instead of 1");
                    }
                }
            }

            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("error: $@");
        }
        last;
    }

    $log->debug("leaving");

    return 1;
}


sub unlock_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ( $key, $type ) = validate_pos( @_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR,
            callbacks   => {
                'is read or write' => sub { $_[0] =~ /^(?:read|write)$/ },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    my $so_id;
    my $read_lock;
    my $write_lock;

TRANS: while (1) {
        eval {
            {
                # this will set update locks
                my $query = $db->prepare_cached( $sql->get_object_locks );
                my $rows = $query->execute($key->path);
                unless ($rows == 1) {
                    $query->finish;
                    die("storage object does not exist");
                }

                my $row = $query->fetchrow_hashref;
                $query->finish;

                $so_id      = $row->{ 'so_id' };
                $read_lock  = $row->{ 'read_lock' };
                $write_lock = $row->{ 'write_lock' };
            }

            if ($type eq 'write') {
                # can't remove a write lock if it doesn't exist
                if ($read_lock) {
                    die("can not have a write lock under a read lock");
                }

                unless ($write_lock) {
                    die("can not remove non-existant write lock");
                }

                {
                    my $query = $db->prepare_cached( $sql->delete_write_lock );
                    my $rows = $query->execute($key->path);
                
                    # if we affected more then one row something very bad has happened.
                    unless ($rows == 1) {
                        die("affected row count is $rows instead of 1");
                    }
                }
            } elsif ($type eq 'read') {
                # can't remove a read lock if there's a write lock and
                # can't remove a read lock if there aren't any
                if ($write_lock) {
                    die("can not have a read lock under a write lock");
                }
                   
                if ($read_lock == 0) {
                    die("can not remove non-existant read lock");
                }

                {
                    my $query = $db->prepare_cached( $sql->decrement_read_lock );
                    my $rows = $query->execute($key->path);
                
                    # if we affected more then one row something very bad has happened.
                    unless ($rows == 1) {
                        die("affected row count is $rows instead of 1");
                    }

                }
            }
            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("error: $@");
        }
        last;
    }

    $log->debug( "leaving" );

    return 1;
}


sub setxattr_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $name, $value, $flags) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR,
            callbacks   => {
                'xattr is in user. namespace'
                    => sub { ($_[0]) =~ qr/^user\./ },
            },
        },
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
            callbacks   => {
                'is read or write' => sub { $_[0] =~ /^(?:create|replace)$/i },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

TRANS: while (1) {
        eval {
            my $query;

            if ($flags eq 'create') {
                $query = $db->prepare_cached( $sql->new_object_xattr );
            } else {
                # replace
                $query = $db->prepare_cached( $sql->replace_object_xattr );
            }

            # name, value, ext_id
            my $rows = $query->execute($name, $value, $key->path);
            $query->finish;

            # if we affected more then one row something very bad has happened.
            if ($flags eq 'create') {
                unless ($rows == 1) {
                    die( "affected row count is $rows instead of 1" );
                }
            } else {
                # replace_object_xattr can effect either 1 or 2 rows.  2 rows in
                # the case of a replace and 1 if the xattr didn't already exist.
                unless ($rows == 1 or $rows == 2) {
                    die( "affected row count is $rows instead of 2" );
                }
            }

            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("database error: $@");
        }
        last;
    }

    $log->debug("leaving");

    return 1;
}


sub getxattr_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $name) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR,
            callbacks   => {
                'xattr is in user. namespace'
                    => sub { ($_[0]) =~ qr/^user\./ },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    my $value;
    eval {
        my $query = $db->prepare_cached( $sql->get_object_xattr );
        # ext_id, name
        my $rows = $query->execute($key->path, $name);

        # no rows returned means that the xattr does not exist
        if ($rows == 0) {
            $query->finish;
            die( "xattr $key:$name does not exist" );
        }

        # if we go more then one row bad something very bad has happened.
        unless ($rows == 1) {
            $query->finish;
            die( "affected row count is $rows instead of 1" );
        }

        my $row = $query->fetchrow_hashref;
        # XXX: DBI bug? ->finish is needed here even though $query is going out
        # of scope
        $query->finish;
        $value = $row->{ 'value' };
    };
    $db->commit;

    if ($@) {
        if ($@ =~ /user\..*? does not exist/) {
            # do not log xattr does not exist messages
	    # NOTE: the client is using the reported message to interpret the errors
            die $@;
        }
        $log->logdie("database error: $@") if $@;
    }

    $log->debug("leaving");

    return $value;
}


sub listxattr_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    my @xattrs;
    eval {
        my $query = $db->prepare_cached( $sql->list_object_xattr );
        # ext_id
        my $rows = $query->execute($key->path);

        while (my $row = $query->fetchrow_hashref) {
            push @xattrs, $row->{ 'name' };
        }
    };
    $log->logdie("database error: $@") if $@;
    $db->commit;

    $log->debug("leaving");

    return \@xattrs;
}


sub removexattr_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $name) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR,
            callbacks   => {
                'xattr is in user. namespace'
                    => sub { ($_[0]) =~ qr/^user\./ },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

TRANS: while (1) {
        eval {
            my $query = $db->prepare_cached( $sql->remove_object_xattr );
            # ext_id, name
            my $rows = $query->execute($key->path, $name);
            $query->finish;

            # no rows affected means the xattr did not exist
            if ($rows == 0) {
                die( "xattr $key:$name does not exist" );
            }

            # if we affected more then one row something very bad has happened.
            if ($rows > 1) {
                die( "affected row count is $rows instead of 1" );
            }
            $db->commit;
            $log->debug("commit");
        };
        if ($@) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie("database error: $@");
        }
        last;
    } 

    $log->debug("leaving");

    return 1;
}

# loop over all db_index values, passing db_index to each call
sub find_objects
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ($pattern) = validate_pos( @_,
        {
            type        => SCALAR,
            optional    => 1,
        },
    );


    eval {
        $pattern = parse_neb_key($pattern) if defined $pattern;
    };
    $log->logdie("$@") if $@;

    unless (defined $pattern) {
        $log->debug( "leaving" );
        $log->logdie("no keys found");
    }

    my @keys = ();
    my $n_dbs = $self->config->n_db();
    for (my $index = 0; $index < $n_dbs; $index ++) {
        my $newkeys = $self->_find_objects_for_index($index, $pattern);
        push @keys, @$newkeys;
    }
    $log->logdie("no keys found") unless ( scalar @keys );

    if (defined $self->cache) {
    	foreach my $path (@keys) {
            $self->cache->set($path, 1);
        	$log->debug("key added to cache as: $path");
	}
    }

    $log->debug( "leaving" );

    return \@keys;
}

sub find_objects_wildcard
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ($pattern) = validate_pos( @_,
        {
            type        => SCALAR,
            optional    => 1,
        },
    );

    my $sql = $self->sql;
    my $db  = $self->_db_for_index(0);
    
    # validate that we have a key to deal with
    eval {
        $pattern = parse_neb_key($pattern) if defined $pattern;
    };
    $log->logdie("$@") if $@;

    unless (defined $pattern) {
        $log->debug( "leaving" );
        $log->logdie("no keys found");
    }

    # parse out the directory we're working in, and decide if we are a directory
    my $dir_id = $self->_resolve_dir_parent_id(key => $pattern, dir_id => 1);
    my $work_dir;
    my $file_pattern;
    if (defined $dir_id) {
	$work_dir = $pattern;
	$file_pattern = '%';
    } else {
	my $dir_plain = dirname($pattern);
	if ($dir_plain eq 'neb:') {
	    $dir_plain = 'neb:///';
	}
	if ($dir_plain) {
	    $work_dir = parse_neb_key($dir_plain);
	}
	else {
	    $work_dir = parse_neb_key('/');
	}
	$file_pattern = basename $pattern;
	unless ($file_pattern) {
	    $file_pattern = '%';
	}
	
	$dir_id = $self->_resolve_dir_parent_id(key => $work_dir, dir_id => 1);
	unless (defined $dir_id) {
	    # $log->logdie("pattern $work_dir does not match any key or directory");
	    die("pattern $work_dir does not match any key or directory");
	}
    }
    
    # find dirs under dir
    my @dir_keys;

    eval {
        $log->debug("looking for directories under dir: $dir_id");
        my $query = $db->prepare_cached( $sql->find_dir_by_parent_id . " AND dirname LIKE ? ");
        $query->execute( $dir_id, $file_pattern );
	$db->commit;

        while ( my $row = $query->fetchrow_hashref ) {
            next if $row->{'dir_id'} == 1;
            my $dir = $row->{'dirname'};
            if ($dir_id == 1) {
                push @dir_keys, $dir . '/' if $dir;
            } else {
                push @dir_keys, $work_dir->path . '/' . $dir . '/' if $dir;
            }
            $log->debug( "matched $dir" ) if $dir;
        }
    };
    $log->logdie("database error: $@") if $@;

    # find files under dir
    my @keys;
    eval {
        $log->debug("looking for objects under dir: $dir_id");
        my $query = $db->prepare_cached( $sql->find_object_by_dir_id . " AND ext_id_basename LIKE ? ");
        $query->execute( $dir_id , $file_pattern);
	$db->commit;

        while ( my $row = $query->fetchrow_hashref ) {
            my $key = $row->{ 'ext_id' };
            push @keys, $key if $key;
            $log->debug( "matched $key" ) if $key;
        }
    };
    $log->logdie("database error: $@") if $@;

    $log->debug( "leaving" );
    
    return [sort(@dir_keys), sort(@keys)];

}

# find matching objects from the given server
sub _find_objects_for_index
{

    my $self    = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my $index   = shift;
    my $key     = shift;

    my $sql = $self->sql;
    my $db  = $self->_db_for_index($index);

    # first check to see if the key is an exact match
    my @keys;
    eval {
        $log->debug("trying for an exact key match with $key");
        my $query = $db->prepare_cached( $sql->find_object_by_ext_id );
        $query->execute( $key->path );
        if ($query->rows) {
            my $ext_id = $query->fetchrow_hashref->{'ext_id'};
            $log->debug( "pattern has an exact match" );
            push @keys, $ext_id;
        } else {
            $log->debug("no exact match for key");
        }
        $query->finish;
    };
    if ($@) {
        $db->rollback;
        $log->logdie("database error: $@");
    }
    $db->commit;

    if (scalar @keys) {
        # it was an exact match, so stop here
        $log->debug("leaving");

        return \@keys;
    }

    # else, assume it's a directory
    my $dir_id = $self->_resolve_dir_parent_id(key => $key, dir_id => 1);
    unless (defined $dir_id) {
        # $log->logdie("pattern $key does not match any key or directory");
	# EAM : reduce verbosity
        $log->debug("pattern $key does not match any key or directory");
	die ("pattern $key does not match any key or directory");
    }

    # find dirs under dir
    my @dir_keys;
    eval {
        $log->debug("looking for directories under dir: $dir_id");
        my $query = $db->prepare_cached( $sql->find_dir_by_parent_id);
        $query->execute( $dir_id );

        while ( my $row = $query->fetchrow_hashref ) {
            next if $row->{'dir_id'} == 1;
            my $dir = $row->{'dirname'};
            if ($dir_id == 1) {
                push @dir_keys, $dir . '/' if $dir;
            } else {
                push @dir_keys, $key->path . '/' . $dir . '/' if $dir;
            }
            $log->debug( "matched $dir" ) if $dir;
        }
    };
    $log->logdie("database error: $@") if $@;
    $db->commit;

    # find files under dir
    eval {
        $log->debug("looking for objects under dir: $dir_id");
        my $query = $db->prepare_cached( $sql->find_object_by_dir_id );
        $query->execute( $dir_id );

        while ( my $row = $query->fetchrow_hashref ) {
            my $key = $row->{ 'ext_id' };
            push @keys, $key if $key;
            $log->debug( "matched $key" ) if $key;
        }
    };
    $log->logdie("database error: $@") if $@;
    $db->commit;

    $log->debug( "leaving" );
    
    return [sort(@dir_keys), sort(@keys)];
}

sub find_ext_id_by_volume
{
    my $self = shift;
    my $log = $self->log;
    $log->debug("entered - @_");
    my ($vol_name,$limit) = validate_pos(@_,
					{
					    type      => SCALAR,
# # 					    callbacks => {
# # 						'is valid volume name' => sub {
# # 						    return 1 if not defined $_[0];
# # 						    $self->_is_valid_volume_name($_[0])
# # 						},
# 					    },
					},
					{
					    type      => SCALAR|UNDEF,
					    optional => 1,
					},
	);
    unless (defined($limit)) {
	$limit = 50000;
    }

    my $sql = $self->sql;
    my @ext_ids;
    my $db = $self->_db_for_index(0);
    eval {
	my $query;
	$query = $db->prepare_cached( $sql->get_ext_id_by_vol_name );
	my $rows = $query->execute($vol_name, 1,$limit);
	unless ($rows > 0) {
	    $query->finish;
	    $log->logdie("no instances on storage volume or volume is not avaiable for volume: $vol_name");
	}
        while (my $row = $query->fetchrow_hashref) {
            my $ext_id = $row->{ 'ext_id' };
            push @ext_ids, $ext_id if $ext_id;
        }
    };
    if ($@) {
        $db->rollback;
        $log->logdie("database error: $@");
    }

    # XXX remove this?
    $log->logdie("no ext_ids found") unless (scalar @ext_ids);

    $log->debug("found: \@ext_ids");

    $log->debug("leaving");

    return \@ext_ids;
}

# sub find_instances
sub find_instances_old
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $vol_name, $find_invalid) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
	{
            # find_invalid
	    type        => SCALAR|UNDEF,
            optional    => 1,
        }, 
    );

    my $sql = $self->sql;

    # default value for find_invalid is false
    if (not defined $find_invalid) { $find_invalid = 0; }

    # vol_name overrides the key implied volume
    eval {
        $key = parse_neb_key($key, $vol_name);
    };
    $log->logdie("$@") if $@;
    $vol_name = $key->volume;

    my $db  = $self->db($key);

    # the key's volume can't be validiated on input for this method so we have
    # to check it after parsing the key
    if (defined $vol_name
        and not $self->_is_valid_volume_name($key, $key->volume)) {
        if ($key->hard_volume) {
            $log->logdie("$vol_name is not a valid volume name");
        } else {
	    # EAM : reduce verbosity
            $log->debug( "$vol_name is not a known volume name" );
            $vol_name = undef;
        }
    }

    my @locations;
    eval {
        my $query;
        if ($vol_name) {
            $query = $db->prepare_cached( $sql->get_object_instances_by_vol_name );
            # ext_id, name, available
            my $rows = $query->execute($key->path, $vol_name, 1);
            unless ($rows > 0) {
                $query->finish;
                die("no instances on storage volume or volume is not avaiable for key: $key volume: $vol_name");
            }
        } else {
            $query = $db->prepare_cached( $sql->get_object_instances );
	    my $rows;
            # ext_id, available
	    if (defined($find_invalid) and $find_invalid) {
		# XXX returns instances which are NOT available
		$rows = $query->execute($key->path, 0);
	    }
	    else {
		$rows = $query->execute($key->path, 1);
	    }
            unless ($rows > 0) {
                $query->finish;
                die("no instances available for key: $key");
            }
        }

        while (my $row = $query->fetchrow_hashref) {
            my $instance = $row->{ 'uri' };
            push @locations, $instance if $instance;
        }
    };
    if ($@) {
        $db->rollback;
        # handle soft volumes
        if (defined $vol_name and not defined $key->hard_volume) {
            $log->debug("retrying with 'any' volume");
            return $self->find_instances($key->path, 'any');
        }
        $log->logdie("database error: $@");
    }

    # XXX remove this?
    $log->logdie("no instances found") unless (scalar @locations);

    $log->debug("found: @locations");

    $log->debug("leaving");

    return \@locations;
}

# sub find_instances_by_proximity
sub find_instances
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $vol_name, $find_invalid) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
	{
            # find_invalid
	    type        => SCALAR|UNDEF,
            optional    => 1,
        }, 
    );
    my $sql = $self->sql;

    if (not defined $find_invalid) { $find_invalid = 0; }

    if ($find_invalid eq "find them all") {
	$log->logdie("old neb-cull --one_only, neb-rm --invalid both disabled by new Server.pm");
    }

    # vol_name overrides the key implied volume
    my ($h_vol_id, $h_cab_id, $h_site_id);
    eval {
        $key = parse_neb_key($key, $vol_name);
    };
    $log->logdie("$@") if $@;
    $vol_name = $key->volume;

    my $db  = $self->db($key);

    # Convert possible alias to real volume.
    if (defined $vol_name) {
	my ($tmp_vol_id, $tmp_name, $tmp_host, $tmp_path);
	eval {
	    my $query = $db->prepare_cached( $sql->get_volume_by_alias );
	    $query->execute( $vol_name );
	    ($tmp_vol_id, $tmp_name, $tmp_host, $tmp_path) = $query->fetchrow_array;
	    $query->finish;
	};
	$log->logdie("$@") if $@;
#	$log->warn("CZW: find_instance: deref alias: $vol_name => $tmp_vol_id $tmp_name $tmp_host $tmp_path (key vol: $key" . $key->volume . ")");
	if (defined $tmp_vol_id and defined $tmp_name and defined $tmp_host and defined $tmp_path) {
	    if ($tmp_name ne $vol_name) {
		$vol_name = $tmp_name;
#		$key->volume = $vol_name;
	    }
	}
    }

    # the key's volume can't be validiated on input for this method so we have
    # to check it after parsing the key
    if (defined $vol_name
        and not $self->_is_valid_volume_name($key, $vol_name)) {
        if ($key->hard_volume) {
            $log->logdie("$vol_name is not a valid volume name");
        } else {
	    # comment this out (too verbose)
            # $log->warn( "$vol_name is not a known volume name" );
            $vol_name = undef;
        }
    }

    # Get the host volume information encoded in the vol_name.
    # I am unhappy that we have three different if(defined($vol_name)) entries, but I don't see a better way.
#    my ($h_vol_id, $h_cab_id, $h_site_id);
    if (defined $vol_name) {
	eval {
	    my $query = $db->prepare_cached( $sql->get_site_info_by_name );
	    $query->execute( $vol_name );
	    ($h_vol_id, $h_cab_id, $h_site_id) = $query->fetchrow_array;
	    $query->finish;
	};
	$log->logdie("$@") if $@;

	unless(defined $h_vol_id and defined $h_cab_id and defined $h_site_id) {
	    $vol_name = undef;
	}
    }

    my @locations;
    eval {
        my $query;
        if ($vol_name && $h_vol_id && $h_cab_id && $h_site_id) {
            $query = $db->prepare_cached( $sql->get_object_instances_by_proximity );
            # ext_id, name, available
	    # host_vol_id host_cab_id host_site_id ext_id available

	    my $rows;
	    if (defined($find_invalid) and $find_invalid) {
		# returns instances which are NOT available (volume not available)
		$rows = $query->execute($h_vol_id, $h_cab_id, $h_site_id, $key->path, 0);
	    } else {
		$rows = $query->execute($h_vol_id, $h_cab_id, $h_site_id, $key->path, 1);
	    }		
            unless ($rows > 0) {
                $query->finish;
                die("no instances on storage volume or volume is not available for key: $key volume: $vol_name");
            }
        } else {
            $query = $db->prepare_cached( $sql->get_object_instances );
            # ext_id, available

	    my $rows;
	    if (defined($find_invalid) and $find_invalid) {
		# returns instances which are NOT available (volume not available)
		$rows = $query->execute($key->path, 0);
	    } else {
		$rows = $query->execute($key->path, 1);
	    }
            unless ($rows > 0) {
                $query->finish;
                die("no instances available for key: $key");
            }
        }
	$db->commit;
	## if we do not call commit here, the transaction stays
	## open blocking some operations below. 

        while (my $row = $query->fetchrow_hashref) {
            my $instance = $row->{ 'uri' };
            push @locations, $instance if $instance;

	    # Carp::carp ("instance 2: $row->{ 'uri' }, $row->{ 'vol_id' }, $row->{ 'cab_id' }, $row->{ 'vol_idx' }\n");
        }
    };
    if ($@) {
        $db->rollback;
        # handle soft volumes
        if (defined $vol_name and not defined $key->hard_volume) {
            $log->debug("retrying with 'any' volume");
            return $self->find_instances($key->path, 'any', $find_invalid);
        }
        $log->logdie("database error: $@");
    }

    # XXX remove this?
    $log->logdie("no instances found") unless (scalar @locations);

    $log->debug("found: @locations");

    $log->debug("leaving");

    return \@locations;
}

# this method returns instances on the specified volume
# or a list of all available instances
# it does NOT choose instances based on proximity
# and it does NOT return invalid instances
sub find_instances_for_cull
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $vol_name) = validate_pos(@_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR|UNDEF,
#            callbacks   => {
#                # check that the volume name requested is valid
#                'is valid volume name' => sub {
#                    return 1 if not defined $_[0];
#                    $self->_is_valid_volume_name($_[0])
#                },
#            },
            optional    => 1,
        },
    );

    my $sql = $self->sql;

#    unless ($key) {
#        $log->warn("key was undefined after validate_pos(), trying again...");
#        return $self->find_instances(@_);
#    }

    # vol_name overrides the key implied volume
    eval {
        $key = parse_neb_key($key, $vol_name);
    };
    $log->logdie("$@") if $@;
    $vol_name = $key->volume;

    my $db  = $self->db($key);

    ## XXX this method is missing the alias deference section (see find_instances)
    ## if aliases are intended to be compute nodes in the same cabinet / site as
    ## the storage nodes, then this is not needed.

    # the key's volume can't be validiated on input for this method so we have
    # to check it after parsing the key
    if (defined $vol_name
        and not $self->_is_valid_volume_name($key, $key->volume)) {
        if ($key->hard_volume) {
            $log->logdie("$vol_name is not a valid volume name");
        } else {
	    # EAM : reduce verbosity
            $log->debug( "$vol_name is not a known volume name" );
            $vol_name = undef;
        }
    }

    my @locations;
    eval {
        my $query;
        if ($vol_name) {
            $query = $db->prepare_cached( $sql->get_object_instances_by_vol_name );
            # ext_id, name, available
            my $rows = $query->execute($key->path, $vol_name, 1);
            unless ($rows > 0) {
                $query->finish;
                die("no instances on storage volume or volume is not available for key: $key volume: $vol_name");
            }
        } else {
            $query = $db->prepare_cached( $sql->get_object_instances );
            # ext_id, available
            my $rows = $query->execute($key->path, 1);
            unless ($rows > 0) {
                $query->finish;
                die("no instances available for key: $key");
            }
        }
	
        while (my $row = $query->fetchrow_hashref) {
#	    my $instance_hash  = { uri => $row->{ 'uri' }, 
#				   vol_id => $row->{ 'vol_id' }, 
#				   cab_id => $row->{ 'cab_id'} };
	    my $instance = $row->{ 'uri' };
            push @locations, $row if $instance;
        }
    };
    if ($@) {
        $db->rollback;
        # handle soft volumes
	# we need to call find_intances_for_cull here so the returned structure 
	# is the same (find_instances just returns an array of strings)
        if (defined $vol_name and not defined $key->hard_volume and ($vol_name ne "any")) {
            $log->debug("retrying with 'any' volume");
            return $self->find_instances_for_cull($key->path, 'any');
        }
        $log->logdie("database error: $@");
    }
    $db->commit;

    # XXX remove this?
    $log->logdie("no instances found") unless (scalar @locations);

    $log->debug("found: @locations");

    $log->debug("leaving");

    return \@locations;
}


sub delete_instance
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ($key, $uri) = validate_pos( @_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type => SCALAR|SCALARREF,
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

TRANS: while (1) {
        eval {
            my $total;
            my $available;
            my $so_id;
            my $ins_id;
            # find so_id for key and get count of instances
            {
                my $query = $db->prepare_cached( $sql->get_instance_count_by_ext_id );
                my $rows = $query->execute($key->path);
                unless ( $rows > 0 ) {
                    $query->finish;
                    die( "$key has no associated instances - this should not happen" );
                }

                my $record = $query->fetchrow_hashref;
                $so_id      = $record->{ 'so_id' };
                $total      = $record->{ 'total' };
                $available  = $record->{ 'available' };
                $query->finish;
            }

            # find ins_id for uri
            {
                my $query = $db->prepare_cached( $sql->get_instance_by_uri );
                my $rows = $query->execute($so_id, $uri);
                unless ( $rows > 0) {
                    $query->finish;
                    die( "no instance is associated with uri" );
                }

                $ins_id = $query->fetchrow_hashref->{ 'ins_id' };
                $query->finish;
            }

            # remove instance
            {
                my $query = $db->prepare_cached( $sql->delete_instance_by_ins_id );
                my $rows = $query->execute( $ins_id );
                $query->finish;
                
                # if we affected something other then one row something very
                # bad has happened
                unless ( $rows == 1 ) {
                    die( "affected row count is $rows instead of 1" );
                }
                
            }

            # if we just deleted the last 'available' instance associated with
            # a storage object remove it too
            if ( $available == 1 ) {
                # remove key from cache
                $self->cache->delete($key->path) if defined $self->cache;
                
                if ($total > $available) {
                    $self->prune_object("$key");
                }
                
                # delete the storage object, remaining instances should be
                # removed via cascading delete
                my $query = $db->prepare_cached( $sql->delete_object );
                my $rows = $query->execute( $so_id );
                $query->finish;

                # TODO: this will have to be changed in order to support hardlinks
                unless ( $rows == 1 ) {
                    die( "affected row count is $rows instead of 2" );
                }
            }
            $db->commit;
            $log->debug("commit");
        };
        if ( $@ ) {
            $db->rollback;
            $log->debug("rollback");
            if ($@ =~ $trans_regex) {
                $log->warn("database error, retrying transaction: $@");
                sleep TRANS_RETRY_WAIT;
                redo TRANS;
            }
            $log->logdie( "database error: $@" );
        }
        last;
    }

    $log->debug( "leaving" );

    return 1;
}


sub stat_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ( $key ) = validate_pos( @_,
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    my $stat;
    eval {
        my $query = $db->prepare_cached( $sql->stat_object );
        my $rows = $query->execute($key->path);

        unless ($rows == 1) {
            die("no storage object found");
        }

        $stat = $query->fetchrow_arrayref;
        $query->finish;
	$db->commit;
    };
    $log->logdie("database error: $@") if $@;

    $log->debug("leaving");

    return $stat;
}

# this should have a 'db_index' as an argument
sub mounts
{
    # XXX: this will only pull the mounts from one db
    # XXX: loop over db_index and generate a single unique list
    # XXX: or report mount info for each db server
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    validate_pos(@_); 

    my $sql = $self->sql;
    my $db  = $self->_db_for_index(0); # XXX fix as above

    my $stats;
    my $query;
    eval {
        $query = $db->prepare_cached( $sql->get_mounted_volumes );
        $query->execute();

        # suck that table into an AoA
        $stats = $query->fetchall_arrayref;

        $query->finish;
    };
    $log->logdie("database error: $@") if $@;
    $db->commit;

    $log->logdie("no mounted volumes found") unless (scalar @$stats);

    $log->debug("leaving");

    return $stats;
}

sub chmod_object
{
    my $self = shift;

    my $log = $self->log;
    $log->debug("entered - @_");

    my ($key, $mode) = validate_pos( @_, 
        {
            type        => SCALAR,
            callbacks   => {
                'is valid object key' => sub { $self->_is_valid_object_key($_[0]) },
            },
        },
        {
            type        => SCALAR,
            regex       => qr/\d{3,4}/,
            callbacks   => {
                'is allowable mode' => sub {
                    $_[0] == (S_IRUSR | S_IRGRP)
                },
            },
        },
    );

    my $sql = $self->sql;

    # ignore volume
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    my $db  = $self->db($key);

    # find all instances of this object
    my $locations;
    eval {
        $locations = $self->find_instances("$key");
    };
    $log->logdie("error: $@") if $@;

    # update each instances
    foreach my $inst (@$locations) {
        my $path = URI->new($inst)->path;

        $self->_retry(sub { chmod($mode, $path) })
            or $log->logdie("can not chmod() $path: $!");

        # XXX I'm assuming that it's OK to fsync() a filehandle that's only
        # open for reading?  Opening as w/rw here can fail if the chmod removes
        # write permissions.
        my $fh;
        $self->_retry(sub { open($fh, '<', $path) })
            or $log->logdie("can not open() $path: $!");

        # fsync(3c)
        $self->_retry(sub { $fh->sync() })
            or $log->logdie("can not sync() $path: $!");

        $self->_retry(sub { close($fh) })
            or $log->logdie("can not close() $path: $!");
    }

    # stick an xattr on this object with the mode
    # XXX this would probably be better as a field in the storage_object_attr
    # table but since we're not planning to use this for very many objects (as
    # a %) it may not be worth adding the extra field at this time.
    eval {
        $self->setxattr_object("$key", 'user.mode', $mode, 'replace');
    };
    $log->logdie("error: $@") if $@;

    $log->debug("leaving");

    return $mode;
}

sub _get_storage_volume
{
    my $self = shift;

    my $log = $self->log;
    no warnings qw( uninitialized );
    $log->debug( "entered - @_" );
    use warnings;

    my ($key, $name, $hard_volume) = @_;
    
    # track the number of calls to this function and
    # give up after 10 attempts
    $get_storage_volume_calls ++;

#   $log->warn("_g_s_v: key:>$key< name:>$name< hard_vol:>$hard_volume<");
    my $sql = $self->sql;
    my $db  = $self->db($key);

    my ($vol_id, $vol_host, $vol_path, $xattr);
    eval {
        my $query;
        my $rows;
        if ( $name ) {
            $query = $db->prepare_cached( $sql->get_storage_volume_by_name );
            # %free, name, avaiable, allocate
            $rows = $query->execute($max_used_space, $name, 1, 1);
            # XXX distinguish between non-existant and unavailable
            unless ($rows > 0) {
                $query->finish;
		$db->commit;

                # if a volume name was specified, and is soft, and we failed to
                # find it, fall back to any volume
                unless ($hard_volume) {
                    ($vol_id, $vol_host, $vol_path, $xattr) = $self->_get_storage_volume($key);
                    return; # this just returns out of the eval not from the subroutine
                }
                die("storage volume: $name is not available");
            }
            # when matching by name we shouldn't ever match more than once
            if ($rows > 1) {
                $query->finish;
		$db->commit;
                die("affected row count is $rows instead of 1");
            }
        } else {
            $query = $db->prepare_cached( $sql->get_storage_volume );
            # %free, avaiable, allocate
            $rows = $query->execute($max_used_space, 1, 1, $topfew_count);
#	    $log->warn("Storage_volume: $rows $topfew_count");
            # there has to be at least one storage volume
            unless ($rows > 0) {
                $query->finish;
		$db->commit;
		# prevent failure in the die due to undefined variables
		my $hard_volume_str = defined $hard_volume ? $hard_volume : "undefined";
		my $name_str = defined $name ? $name : "undefined";
                die("no storage volume is available for key: $key volume: $name_str hard_volume: $hard_volume_str");
            }
        }

        my $free;
        ($vol_id, $vol_host, $vol_path, $xattr, $free) = $query->fetchrow_array;
        $query->finish;
    };
    if ($@) {
        if ($@ =~ qr/no storage volume is available/) {
            # this should not happen unless all volumes are full
            $log->error($@);
            $log->debug("retrying...");
	    if ($get_storage_volume_calls < 10) {
		return $self->_get_storage_volume(@_);
	    }
	    # else
	    $log->logdie("no available storage volume: $@");
        } 
        # else
        $log->logdie("database error: $@");
    }

    $log->logdie("failed to find a suitable volume" )
        unless defined $vol_id and defined $vol_path;

    $log->debug( "leaving" );

    return ($vol_id, $vol_host, $vol_path, $xattr);
}


sub _get_replication_volume
{
    my $self = shift;

    my $log = $self->log;
    no warnings qw( uninitialized );
    $log->debug( "entered - @_" );
    use warnings;

    my $key = shift;

    my $sql = $self->sql;
    my $db  = $self->db($key);

    my ($vol_id, $vol_host, $vol_path, $xattr, $forbidden_cabinet, $forbidden_site);
    eval {
        my $rows;
	
	my $query = $db->prepare_cached( $sql->get_cabinets_for_ext_id );
	$rows = $query->execute($key->path);
	unless ($rows > 0) {
	    $query->finish;
	    die("Requested key $key does not exist (or cabinet is undefined)");
	}
	if ($rows == 1) {
	    ($forbidden_cabinet, $forbidden_site) = $query->fetchrow_array;
	    unless (defined($forbidden_cabinet)) {
		$forbidden_cabinet = 0;
	    }
	    unless (defined($forbidden_site)) {
		$forbidden_site = 0;
	    }
	    $query->finish;
	} else {
	    # if instances are spread across multiple cabinets, allow the next instance to go anywhere
	    # NOTE: this does not prevent the new instance from going to the same volume as one of the existing instances
	    $forbidden_cabinet = 0;
	    $forbidden_site    = 0;
	    $query->finish;
	}
	$db->commit;

        $query = $db->prepare_cached( $sql->get_replication_volume_for_ext_id );
        # ext_id, %free, available, allocate

        $rows = $query->execute($key->path, $max_used_space, 1, 1, $forbidden_cabinet, $forbidden_site, $topfew_count);
        # XXX distinguish between non-existant and unavailable
        unless ($rows > 0) {
            $query->finish;
	    # CZW: 2016-08-23 This wasn't right.  If we don't get an entry, we may have been too strict.
	    #      I'm not fully convinced this is complete, as we may want to back out the cabinet criterion as well.
	    #      In any case, I don't think we're generally in the situation where replication can't find a host.
	    $rows = $query->execute($key->path, $max_used_space, 1, 1, $forbidden_cabinet, 0, $topfew_count);
	    unless ($rows > 0) {
		die("can't find a suitable storage volume to replicate $key to");
	    }
        }
        # when matching by name we shouldn't ever match more than once
        if ($rows > 1) {
            $query->finish;
            die("affected row count is $rows instead of 1");
        }

        my $free;
        ($vol_id, $vol_host, $vol_path, $xattr, $free) = $query->fetchrow_array;
        $query->finish;
	$db->commit;
    };
    $log->logdie("database error: $@") if $@;

    $log->logdie("failed to find a suitable volume" )
        unless defined $vol_id and defined $vol_path;

    $log->debug( "leaving" );

    return ($vol_id, $vol_host, $vol_path, $xattr);
}


sub _is_valid_object_key
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ($key) = @_;

    my $sql = $self->sql;

    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;

    # check cache first
    my $cached = $self->cache->get($key->path) if defined $self->cache;
    if (defined $cached) {
        $log->debug( "key $key found in cache as ", $key->path );
        $log->debug( "leaving" );
        return 1;
    } else {
        $log->debug( "key $key not found in cache" );
    }

    my $db  = $self->db($key);

    my $ext_id;
    eval {
        my $query = $db->prepare_cached( $sql->check_object_name ); 
        $query->execute($key->path);
        ($ext_id) = $query->fetchrow_array;
        $query->finish;
        $db->commit;
	## if we do not call commit here, the transaction stays open blocking some operations below
    };
    if ($@) {
        $db->rollback;
        $log->debug("rollback");
        $log->logdie( "database error: $@" );
    }

    if (defined $ext_id) {
        $log->debug( "key found in db" );
    	# add key to cache
        $self->cache->set($key->path, 1) if defined $self->cache;
        $log->debug( "key added to cache as ", $key->path );
        $log->debug( "leaving" );
        return 1;
    } 

    $log->debug( "key not found in db" );
    $log->debug( "leaving" );

    return;
}

# in the past, _is_valid_volume_name returned 'undef'.  
# now it either returns 1 (success) or 0 (failure)
sub _is_valid_volume_name
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ($key, $vol_name) = @_;

    my $sql = $self->sql;

    # the volume name implied by the key is ignored.  $key is only needed to
    # select a database connection
    eval {
        $key = parse_neb_key($key);
    };
    $log->logdie("$@") if $@;
    my $volume_info = parse_neb_volume($vol_name);

    my $db  = $self->db($key);

    $vol_name = $volume_info->{volume};

    # handle "any" volume
    if (($vol_name eq 'any')||($vol_name eq 'any.0')) {
        $log->debug( "found volume name $vol_name" );
        $log->debug( "leaving" );
        return 1;
    }

    my ($vol_id, $vol_path);
    eval {
        my $query = $db->prepare_cached( $sql->get_volume_by_name ); 
        $query->execute( $vol_name );
        my $free;
        ($vol_id, $vol_path, $free) = $query->fetchrow_array;
        $query->finish;
        $db->commit;
    };
    $log->logdie("database error: $@") if $@;

#    $log->warn("CZW: $vol_id $vol_path for >>$vol_name<<");
    if (defined $vol_id and defined $vol_path) {
        $log->debug( "found volume name $vol_name" );
        $log->debug( "leaving" );
#	$log->warn("CZW: $vol_id $vol_path for >>$vol_name<<");
        return 1;
    } 

    $log->debug( "volume name $vol_name not found" );
    $log->debug( "leaving" );

    return 0;
}


sub _create_empty_instance_file
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ($key, $so_id, $ins_id, $vol_path, $xattr) =  @_;

    my $sql = $self->sql;
    my $db  = $self->db($key);

    my $uri;
    eval {
        my $storage_path = $self->_generate_storage_path($key->path, $vol_path);
        my $storage_filename = $self->_generate_storage_filename($key->path, $ins_id);
        unless (-d $storage_path) {
            $self->_retry(sub { mkpath([$storage_path], 0, 0775) })
                or die("can't create storage path: $storage_path");
        }
        # check to make sure at least the parent directory has the proper
        # permissions
        my $mode = [$self->_retry(sub { stat($storage_path) } )]->[2] & 07777;
        unless ($mode == 0775) {
            # XXX: this problem is so common that it's flooding the logs
            $log->debug("$storage_path has the wrong permissions of: 0", sprintf("%o", $mode));
            $self->_retry(sub { chmod(0775, $storage_path) })
                or die("can not chmod() $storage_path: $!");
        }

        my $fqpn = File::Spec->catfile($storage_path, $storage_filename);
        $uri = URI::file->new($fqpn);
        $log->debug("generated uri $uri");
        $self->_create_empty_file($uri->file);
    };
    if ($@) {
        if (defined $uri and -e $uri->file) {
            unlink($uri->file)
                or $log->error("failed to unlink() $uri: $!");
        }
        $log->logdie($@);
    }

#     if ($xattr) {
#         my $path = $uri->file;
#         $log->logdie("can not set xattr on $path: $!")
#             unless (setfattr($path, 'user.nebulous_key', $key->path));
#     }

    return $uri;
}


sub _create_empty_file
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my ($path) = @_;

    # perl's open() can't do an O_CREAT | O_EXCL
    # XXX is it possible to tell if this system call failed?
    -e $path
        and $log->logdie("file $path already exists");

    my $fh;
    $self->_retry(sub { open($fh, '>', $path) })
        or $log->logdie("can not open() $path: $!");

    # chmod before fsync() to make sure the changed perms hit the disk too
    $self->_retry(sub { chmod(0664, $path) })
        or $log->logdie("can not chmod() $path: $!");

    $self->_retry(sub { $fh->sync() })
        or $log->logdie("can not sync() $path: $!");

    $self->_retry(sub { close($fh) })
        or $log->logdie("can not close() $path: $!");

    return $path;
}


sub _generate_storage_filename
{
    my $self = shift;

    my ($key, $ins_id) = @_;

    my $filename = $key;
    # mangle '/'s into ':'
    $filename =~ s|/|:|g;

    return "$ins_id.$filename"
}


sub _generate_storage_path
{
    my $self = shift;

    my ($key, $vol_path) = @_;

    # taken and modified from Cache::File::cache_file_path()
    # Copyright (C) 2003-2006 Chris Leishman.  All Rights Reserved.
    my $shakey = sha1_hex($key);
    my (@path) = unpack('A2' x SUBPATH_DEPTH, $shakey);

    return File::Spec->catdir($vol_path, @path);
}


sub _retry
{
    my $self = shift;

    my $log = $self->log;
    $log->debug( "entered - @_" );

    my $func = shift;

    my @ret;
    for (my $i = 0; $i < NFS_RETRIES; $i++) {
        eval {
            @ret = $func->(@_);
        };
        if ($@) {
            $log->logdie($@);
            sleep NFS_RETRY_WAIT;
            next;
        }

        last;
    }

    # if the loop ended and $@ is set, rethrow the error
    if ($@) {
        $log->logdie($@);
    }

    return @ret;
}


sub DESTROY
{
    my $self = shift;

    my $log = $self->log;
    my $sql = $self->sql;
#    my $db  = $self->db;

    $log->debug( "entered" );

# XXX do we need to loop over db_index?
#    $self->db->disconnect;        

#    $log->debug( "disconnected from database: ", sub { $db->data_sources; } );

    $log->debug( "leaving" );
}


1;

__END__
