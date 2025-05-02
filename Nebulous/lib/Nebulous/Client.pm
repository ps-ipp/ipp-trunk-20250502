# Copyright (c) 2004-2008  Joshua Hoblitt
#
# $Id: Client.pm,v 1.64 2008-12-14 22:48:35 eugene Exp $

package Nebulous::Client;

use strict;
use warnings FATAL => qw( all );
no warnings qw( uninitialized );

our $VERSION = '0.18';

use Digest::MD5;
use File::Copy qw();
use Log::Log4perl qw(get_logger :levels);
use Nebulous::Client::Log;
use Nebulous::Util qw( :standard );
use Params::Validate qw( validate validate_pos SCALAR UNDEF BOOLEAN );
use Sys::Hostname;
use SOAP::Lite;
# use SOAP::Lite +trace => [ transport => sub { print $_[0]->as_string } ];
# use SOAP::Lite +trace => [qw( debug )];
use Time::HiRes qw( sleep );
use URI;

use constant LOCK_INTERVAL  => 1;
use constant LOCK_DEFAULT   => 10;    # default to a 10s lock timeout

$SOAP::Constants::PATCH_HTTP_KEEPALIVE = 1;
my $log;

# TODO remove most of the logdies


sub import
{
    my $class = shift;

	my %args = validate( @_,
        {
            trace => {
                type        => SCALAR,
                optional    => 1,
                default     => 'fatal',
                callbacks   => {
                    'is valid level' => sub {
                        defined $LEVELS{ lc $_[0] };
                    },
                },
            },
        },
    );

    Nebulous::Client::Log->init;
    $log = get_logger( "Nebulous::Client" );
    $log->level( $LEVELS{ lc $args{ 'trace' } } );

    # this has to be after the log level is set
    $log->debug( "args: @_" );

    return 1;
}


sub new
{
    my $class = shift;

    my %args = validate( @_,
        {
            proxy   => {
                type        => SCALAR,
            },
            uri     => {
                type        => SCALAR,
                optional    => 1,
            },
        },
    );

    Nebulous::Client::Log->init;
    $log = get_logger( "Nebulous::Client" );
    $log->level( $LEVELS{ lc $args{ 'trace' } } );

    # this has to be after the log level is set
    $log->debug( "args: @_" );

    $log->debug( "entered - @_" );

    my $lite = SOAP::Lite->new(
	proxy => $args{ 'proxy' },
	uri   => $args{ 'uri' } || "urn:Nebulous/Server/SOAP",
#       outputxml => 1,
# uncomment the above to get a dump of the returned xml
    );

    $log->logdie( "can not create SOAP::Lite object" ) unless $lite;

    my $self = bless( { server => $lite }, ref $class || $class );

    $log->debug( "leaving" );

    return $self;
}


sub create
{
    my $self = shift;

    my ( $key, @params ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
    );

    $log->debug( "entered - @_" );

    # how should already existing files be handled?

    my $response = $self->{ 'server' }->create_object( $key, @params );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        unless ($response->faultstring =~ /Duplicate entry/) {
            $log->logdie("unhandled fault - ", $self->err);
        }

        $log->debug( "leaving" );

        return;
    }

    my $res = $response->result;
    $log->debug( "server response: $res" );

    my $uri = URI->new($res);
    $log->debug( "URI is: $uri" );

    $log->debug( "leaving" );

    return $uri;
}


sub open_create
{
    my $self = shift;

    my ( $key, @params ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
    );

    $log->debug( "entered - @_" );

    # how should already existing files be handled?

    my $response = $self->{ 'server' }->create_object( $key, @params );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        unless ($response->faultstring =~ /Duplicate entry/) {
            $log->logdie("unhandled fault - ", $self->err);
        }

        $log->debug( "leaving" );

        return;
    }

    my $res = $response->result;
    $log->debug( "server response: $res" );

    my $uri = URI->new($res);
    $log->debug( "URI is: $uri" );

    my $fh;
    eval {
        $fh = _open_uri( $uri, '+<' );
    };
    $log->logdie( $@ ) if $@;

    $log->debug( "leaving" );

    return $fh;
}


sub replicate
{
    my $self = shift;

    my ( $key, @params ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            # volume
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
    );
    
    $log->debug( "entered - @_" );

    # We have to open the instance that we're going to copy from first.  If
    # we don't do this, it's possible that open will find & open the new
    # instance that we're in the process of creating
    my $fh  = $self->open( $key, 'read' );
    unless ( $fh ) {
        $log->debug( "can't open $key" );
        $log->debug( "leaving" );

        return;
    }

    # ask the server for a new instance attached to our key
    my $response = $self->{ 'server' }->replicate_object( $key, @params );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        $log->logdie("unhandled fault - ", $self->err);
    }

    my $res = $response->result;
    $log->debug( "server response: $res" );

    my $uri = URI->new($res);
    $log->debug( "URI is: $uri" );

    my $new_fh;
    eval {
        # must open read/write so we can check the md5sum
        $new_fh = _open_uri( $uri, '+>' );
    };
    $log->logdie( $@ ) if $@;

    my $success = File::Copy::copy( $fh, $new_fh );
    unless ($success) {
        # if the copy failed we now have a zero length instances floating
        # around that must be removed
        unless ($self->delete_instance($key, "$uri")) {
            $log->logdie( "can not copy instance $uri AND FAILED TO CLEANUP EMPTY INSTANCE" );
        }
        $log->logdie( "can not copy instance $uri" );
    } 

    # check md5sum
    my $src_md5 = Digest::MD5->new->addfile($fh)->hexdigest;
    $log->debug( "md5sum src instance: $src_md5" );
    my $dst_md5 = Digest::MD5->new->addfile($new_fh)->hexdigest;
    $log->debug( "md5sum dst instance: $dst_md5" );

    unless ($src_md5 eq $dst_md5) {
        $self->delete_instance($key, "$uri");
        $log->logdie( "md5sum mismatch" );
    }

    $log->debug( "copied instance" );

    # Force a flush to disk
    {
	my $disk_file_name = $uri->path;
#	print ">> $disk_file_name <<\n";
	system("glockfile $disk_file_name xcld 0");
	unless (($? >> 8) == 0) {
	    $log->logdie( "can not create a lock on $key at $uri" );
	}
    }
    close( $fh ) or $log->logdie( "can not close $key" );
    close( $new_fh ) or $log->logdie( "can not close $key" );

    $log->debug( "leaving" );

    return $uri;
}


sub cull
{
    my $self = shift;

    my ($key, $vol_name, $min_copies) = validate_pos(@_,
        {
            type => SCALAR,
        },
        {
            # volume
            type        => SCALAR,
            optional    => 1,
        },
        {
            # min copies
            type        => SCALAR,
            optional    => 1,
        },
    );

    $min_copies ||= 1;

    $log->debug( "entered - @_" );

    my $stats = $self->stat($key);
    my $instances = $stats->[6];

    if ((not defined $instances) or ($instances == 0)) {
        $self->set_err("can not cull - no instances");
        $log->debug("leaving");
        return;
    }
    if ($instances <= $min_copies) {
        $self->set_err("can not cull - not enough instances");
        $log->debug("leaving");
        return;
    }

    # if vol_name is specified we need to stat the file to find out how many
    # instances there are on all volumes.  Otherwise we wouldn't know if it's
    # safe to remove the last instance on the specified volume.
    # XXX We need some way to determine which is the best instance to remove
    my $locations;
    if (defined $vol_name) {
        $locations = $self->find_instances_for_cull($key, $vol_name);
    } else {
        $locations = $self->find_instances_for_cull($key);
    }

    unless (defined $locations) {
        $log->debug( "leaving" );
        return;
    }
    
    # Calculate which is the best instance to remove. First, see if we have two copies
    # on one volume, if so, delete the first of those copies. 
    my $delete_index = -1;

    for (my $i = 0 ; $i <= $#{ $locations }; $i++) {
	for (my $j = $i + 1; $j <= $#{ $locations }; $j++) {
	    if (@$locations[$i]->{vol_id} eq @$locations[$j]->{vol_id}) {
		$delete_index = $i;
	    }
	}
    }

    # If we don't have two copies on one volume, see if we have two copies on a single 
    # cabinet. If so, delete the first of those copies.
    if ($delete_index == -1) {
	for (my $i = 0 ; $i <= $#{ $locations }; $i++) {
	    for (my $j = $i + 1; $j <= $#{ $locations }; $j++) {
		if (@$locations[$i]->{cab_id} eq @$locations[$j]->{cab_id}) {
		    $delete_index = $i;
		}
	    }
	}
    }

    # Fail-safe. We didn't have any duplicates (the instances are "well-mixed"), so 
    # delete the first.
    if ($delete_index == -1) {
	$delete_index = 0;
    }    
    my $uri = $self->delete_instance($key, @$locations[$delete_index]->{uri});

    eval {
        _nuke_file(_get_file_path(@$locations[$delete_index]->{uri}));
    };
    if ($@) {
        $log->logdie($@);
    }

    $log->debug("leaving");

    return $uri;
}


sub there_can_be_only_one
{
    my $self = shift;

    my ($key, $vol_name) = validate_pos(@_,
        {
            type => SCALAR,
        },
        {
            # volume
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
    );

    $log->debug( "entered - @_" );

    my $locations;
    my $removed = 0;
    eval {
        # first - strip off any inaccesible instances
	# these are not counted in the 'removed' number
        if (not defined $self->prune($key)) {
            # use the prune() method to also determine if $key is valid
            die "invalid key: $key";
        }
	
        # check to see if there is an instance on $vol_name that
        # should be the sole survivor.  This is a bit dangerous : what
        # if the only instance is on an invalid volume?  what if an
        # invalid volume is specified?

        if (defined $vol_name) {
            $locations = $self->find_instances($key, $vol_name);
            if (not defined $locations) {
                die "no instances on requested node";
            }
            if (scalar @$locations == 0) {
                die "no instances on requested node";
            }
        }

        # set the number of user copies to 1 to prevent replication
        $self->setxattr($key, 'user.copies', 1, 'replace');

        if (defined $locations) {
	    print "dev: looking for undef volume instances\n";
	    # first delete the valid instances (except the one we own)
            my $instances_valid = $self->find_instances($key, undef, 0); # find all valid instances
            if (not defined $instances_valid) {
		# there should have been at least one valid instance?
                die "no instances";
            }
            if (scalar @$instances_valid == 0) {
		# there should have been at least one valid instance?
                die "no instances";
            }

	    # we have more than one instance, delete the others
            if (scalar @$instances_valid > 1) {
		foreach my $victim (@$instances_valid) {
		    next if $victim eq $locations->[0];
		    $self->delete_instance($key, $victim);
		    $removed ++;
		}
	    }
	    # the prune above should have deleted any invalid instances, so we only
	    # need to worry about the valid instances;
        } else {
            # nuke whatever
            my $stats = $self->stat($key);
            # start at one so cull() is called one less time then the # of
            # instances
            if ($stats->[6] ==  1) {
                # only one instance nothing to do
                return 0;
            }
            if ($stats->[6] == 0) {
                die "no instances";
            }
            for (my $i = 1; $i < $stats->[6]; $i++) {
                $self->cull($key, "any", 1);
                $removed++;
            }
        }
    };
    if ($@) {
        if ($@ =~ qr/invalid key/) {
            $self->set_err($@);
            return;
        }
        $log->logdie($@);
    }

    $log->debug("leaving");

    return $removed;
}

sub storage_object_exists
{
    my $self = shift;

    # HI!

    my ($key) = validate_pos(@_,
        {
            type => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $found = 0;
    eval {
        my $objects = $self->find_objects($key);
        if  ($objects) {
            $found = (scalar @$objects) > 0;
        }
    };
    if ($@) {
        if ($@ =~ qr/invalid key/) {
            $self->set_err($@);
            return;
        }
        $log->logdie($@);
    }

    $log->debug("leaving");

    return $found;
}

sub lock
{
    my $self = shift;

    my ( $key, $type, $timeout ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
            callbacks   => {
                'is read or write' => sub { $_[0] =~ /^(?:read|write)$/ },
            },
        },
        {
            type        => SCALAR,
            default     => LOCK_DEFAULT,
            optional    => 1,
            callbacks   => {
                'is + integer' => sub { $_[0] =~ /^\d+$/ },
            },
        },
    );

    my $endtime = time() + $timeout;

    $log->debug( "entered - @_" );

    my $locked;

    while ( time() < $endtime ) {
        $log->debug( "trying to get a $type lock..." );

        my $response = $self->{ 'server' }->lock_object( $key, $type );
        if ( $response->fault ) {
            $self->set_err($response->faultstring);

            # "retry" means we couldn't get a lock because one was already held
            # and we should try again
            if ( $response->faultstring =~ /retry/ ) {
                $log->debug( "failed to get a $type lock..." );
                next;
            }

            if ($response->faultstring =~ /is valid object key/) {
                $log->debug( "leaving" );
                return;
            }

            $log->logdie("unhandled fault - ", $self->err);
        }

        $locked = $response->result;
        if ( $locked ) {
            $log->debug( "got a $type lock" );
            last;
        }
    } continue {
        $log->debug( "sleeping..." );
        sleep LOCK_INTERVAL;
    }

    $log->debug( "can not get a lock" ) unless $locked;

    $log->debug( "leaving" );
    
    $locked ? return 1 : return;
}


sub unlock
{
    my $self = shift;

    my ( $key, $type ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
            callbacks   => {
                'is read or write' => sub { $_[0] =~ /^(?:read|write)$/ },
            },
        },
    );

    $log->debug( "entered - @_" );

    my $unlocked;

    $log->debug( "trying to release a $type lock..." );

    my $response = $self->{ 'server' }->unlock_object( $key, $type );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

        # key doesn't exist
        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
            return;
        }

        # lock doesn't exist
        if ($response->faultstring =~ /non-existant read lock/) {
            $log->debug( "leaving" );
            return;
        }

        if ($response->faultstring =~ /non-existant write lock/) {
            $log->debug( "leaving" );
            return;
        }

        if ($response->faultstring =~ /can not have a read lock under a write lock/) {
            $log->debug( "leaving" );
            return;
        }

        if ($response->faultstring =~ /can not have a write lock under a read lock/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    $unlocked = $response->result;
    if ( $unlocked ) {
        $log->debug( "released a $type lock" );
    } else {
        $log->debug( "can not release $type lock" );
    }

    $log->debug( "leaving" );
    
    $unlocked ? return 1 : return;
}


sub setxattr
{
    my $self = shift;

    my ($key, $name, $value, $flags) = validate_pos(@_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
            callbacks   => {
                'is create or replace' => sub { $_[0] =~ /^(?:create|replace)$/i },
            },
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->setxattr_object( $key, $name, $value, $flags);
    if ($response->fault) {
        $self->set_err($response->faultstring);

	## invalid object key
        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
	    die "invalid key $key, fails 'is valid object key' test\n";
        }

	## invalid object key
        if ($response->faultstring =~ /xattr is in user. namespace/) {
            $log->debug( "leaving" );
	    die "invalid xattr key $name : fails 'xattr is in user. namespace' test\n";
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    $log->debug( "leaving" );
    
    return 1;
}


sub getxattr
{
    my $self = shift;

    my ($key, $name) = validate_pos(@_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->getxattr_object( $key, $name );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

	## invalid object key
        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
	    die "invalid key $key, fails 'is valid object key' test\n";
        }

	## invalid xattr key (not in user. namespace)
        if ($response->faultstring =~ /xattr is in user. namespace/) {
            $log->debug( "leaving" );
	    die "xattr $name is invalid : xattr is in user. namespace\n";
        }

	## invalid xattr key
        if ($response->faultstring =~ /user\..*? does not exist/) {
            $log->debug( "leaving" );
	    die "xattr key $name not found : xattr $name does not exist\n";
        }
        $log->logdie("unhandled fault - ", $self->err);
    }

    my $res = $response->result;
    $log->debug( "server response: $res" );

    $log->debug( "leaving" );
    
    return $res;
}


sub listxattr
{
    my $self = shift;

    my ($key) = validate_pos(@_,
        {
            type        => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->listxattr_object( $key );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        $log->logdie("unhandled fault - ", $self->err);
    }

    my $res = $response->result;
    $log->debug( "server response: $res" );

    $log->debug( "leaving" );
    
    return $res;
}


sub removexattr
{
    my $self = shift;

    my ($key, $name) = validate_pos(@_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->removexattr_object( $key, $name );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

	## invalid object key
        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
	    die "invalid key $key, fails 'is valid object key' test\n";
        }

	## invalid xattr key (not in user. namespace)
        if ($response->faultstring =~ /xattr is in user. namespace/) {
            $log->debug( "leaving" );
	    die "xattr $name is invalid : xattr is in user. namespace\n";
        }

	## invalid xattr key
        if ($response->faultstring =~ /user\..*? does not exist/) {
            $log->debug( "leaving" );
	    die "xattr key $name not found : xattr $name does not exist\n";
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    $log->debug( "leaving" );
    
    return 1;
}


sub find_objects
{
    my $self = shift;

    my @args = validate_pos( @_,
        {
            type        => SCALAR,
            optional    => 1,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->find_objects( @args );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

        if ($response->faultstring =~ /no keys found/) {
            $log->debug( "leaving" );
            return;
        }
        if ($response->faultstring =~ /does not match any key or directory/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    my $keys = $response->result;

    $log->debug( "server found: @$keys" );

#    foreach my $path ( @{ $uris } ) {
#        $path = _get_file_path( $path );
#    }

    $log->debug( "leaving" );

    return $keys;
}

sub find_objects_wildcard
{
    my $self = shift;

    my @args = validate_pos( @_,
        {
            type        => SCALAR,
            optional    => 1,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->find_objects_wildcard( @args );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

        if ($response->faultstring =~ /no keys found/) {
            $log->debug( "leaving" );
            return;
        }
        if ($response->faultstring =~ /does not match any key or directory/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    my $keys = $response->result;

    $log->debug( "server found: @$keys" );

#    foreach my $path ( @{ $uris } ) {
#        $path = _get_file_path( $path );
#    }

    $log->debug( "leaving" );

    return $keys;
}


sub find_instances
{
    my $self = shift;

    my ( $key, @params ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            #volume
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
        {
	    #find_invalid
	    type        => SCALAR|UNDEF,
            optional    => 1,
	},
    );

    $log->debug( "entered - @_" );
    
    unless(defined($params[0])) {
	$params[0] = hostname() . ".0";
	# print STDERR "Setting host to $params[0]\n";
    }

    my $response = $self->{ 'server' }->find_instances( $key, @params );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

        # check to see if this failure is because $key doesn't exist
        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
            return;
        }
        # key is valid but no instances are on the specified volume
        if ($response->faultstring =~ /no instances on storage volume/) {
            $log->debug( "leaving" );
            return;
        }
        # check to see if this failure is volume is unknown
        if ($response->faultstring =~ /is not a valid volume name/) {
            $log->debug( "leaving" );
            return;
        }
        # key is valid but no instances are on the specified volume
        if ($response->faultstring =~ /no instances available for key/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    my $uris = $response->result;

    $log->debug( "server found: @$uris" );

    $log->debug( "leaving" );

    return $uris;
}

sub find_ext_id_by_volume
{
    my $self = shift;
    my ($vol_name, $limit) = validate_pos( @_,
					   { 
					       type => SCALAR,
					   },
					   {
					       type => SCALAR|UNDEF,
					       optional => 1,
					   },
	);
    
    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->find_ext_id_by_volume( $vol_name, $limit);
    if ( $response->fault ) {
	$self->set_err($response->faultstring);
	if ($response->faultstring =~ /no instances on storage volume/) {
	    $log->debug( "leaving" );
	    return;
	}

	$log->logdie("unhandled fault - ", $self->err);
    }

    my $ext_ids = $response->result;
    
    $log->debug( "server found @$ext_ids" );
    $log->debug( "leaving" );
    
    return($ext_ids);
}

sub find_instances_for_cull
{
    my $self = shift;

    my ( $key, @params ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            #volume
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->find_instances_for_cull( $key, @params );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        # check to see if this failure is because $key doesn't exist
	my $string = $response->faultstring;
        if ($response->faultstring =~ /is valid object key/) {
	    $self->set_err("invalid object key");
            $log->debug( "leaving" );
            return;
        }
        # key is valid but no instances are on the specified volume
        if ($response->faultstring =~ /no instances on storage volume/) {
	    $self->set_err("no instances on storage volume or volume is not available");
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    my $uris = $response->result;

    $log->debug( "server found: @$uris" );

    $log->debug( "leaving" );

    return $uris;
}


## this is used by PS:IPP:Config nebulous functions
sub find
{
    my $self = shift;

    my ( $key, @params ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            #volume
            type        => SCALAR|UNDEF,
            optional    => 1,
        },
    );

    $log->debug( "entered - @_" );

    my $find_volume = hostname() . ".0";
    $params[0] = $find_volume;
    my $locations = $self->find_instances( $key, @params );
    unless (defined $locations) {
        unless ($self->err =~ /no instances on storage volume/) {
            return;
        }

        # then fall back to looking for isntances on any volume
        $locations = $self->find_instances( $key, 'any');
        unless (defined $locations) {
            return;
        }
    }

    my $path;
    eval {
        $path = _get_file_path( $locations->[0] );
    };
    $log->logdie( $@ ) if $@;

    $log->debug( "leaving" );

    return $path;
}

sub open
{
    my $self = shift;

    my ( $key, $type ) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            type        => SCALAR,
            callbacks   => {
                'is read or write' => sub { $_[0] =~ /^(?:read|write|>)$/ },
            },
        },
    );

    $log->debug( "entered - @_" );

    my $fh;

    # need to figure out which is the most optimal copy to open
    my $locations = $self->find_instances( $key );

    my $path;
    eval {
        $path = _get_file_path( $locations->[0] );
    };
    $log->logdie( $@ ) if $@;

    unless ( $path ) {
        $log->debug( "no instances" );

        if ( $type eq 'write' ) {
            $log->debug( "creating a new storage object" );

            my $ret = $self->open_create( $key );

            $log->debug( "leaving" );

            return $ret;
        } else {
            $log->debug( "leaving" );
            return;
        }
    }

    if ( $type eq 'write' or $type eq '>' ) {
        my $num_instances = scalar @$locations;

        if ( $num_instances > 1 ) {
            $log->warn( "write not allowed with multiple instances" );
            $log->debug( "leaving" );
            return;
        }

        eval {
            $fh = _get_filehandle( $path, '+<' );
        };
        $log->logdie( $@ ) if $@;

        if ($type eq '>') {
            truncate($fh, 0) or $log->logdie("truncate failed: $!");
        }
    } elsif ( $type eq 'read' ) {
        eval {
            $fh = _get_filehandle( $path, '<' );
        };
        $log->logdie( $@ ) if $@;
    }

    $log->debug( "leaving" );

    return $fh;
}


sub delete
{
    my $self = shift;

    my ($key, $force, $invalid) = validate_pos( @_,
        {
            type        => SCALAR,
        },
        {
            type        => BOOLEAN,
            optional    => 1,
            default     => undef,
            callbacks   => {
                'is boolean' => sub {
                    $_[0] == 0 or $_[0] == 1 or $_[0] == undef;
                },
            },
        },
        {
            type        => BOOLEAN,
            optional    => 1,
            default     => undef,
            callbacks   => {
                'is boolean' => sub {
                    $_[0] == 0 or $_[0] == 1 or $_[0] == undef;
                },
            },
        },
    );

    $log->debug( "entered - @_" );

    my $locations;
    if ($invalid) {
	# delete the invalid instances:
	$locations = $self->find_instances( $key, 'any', 1);
    }
    else {
	$locations = $self->find_instances( $key, 'any' );
    }

    return undef unless $locations;
        
    # a lock is implicitly removed when the last storage object is deleted
    foreach my $uri ( @$locations ) {
        # it is being assumed here that it is better to have files on disk and
        # not in the database then the inverse.
        my $path;
        eval {
            $path = _get_file_path( $uri );
        };
        if ($@) {
            if ($force) {
                $log->warn($@);
                $log->warn("exception ignored because force is in effect");
            } else {
                $log->logdie($@);
            }
        }

        $self->delete_instance($key, $uri) or return undef;

        eval {
            _nuke_file( $path );
        };
        if ($@) {
            if ($force) {
                $log->warn($@);
                $log->warn("exception ignored because force is in effect");
            } else {
                $log->logdie($@);
            }
        }
    }

    $log->debug( "leaving" );

    return 1;
}


sub copy
{
    my $self = shift;

    my ( $key, $new_key, $volume ) = validate_pos( @_,
        {
            type => SCALAR,
        },
        {
            type => SCALAR,
        },
        {
            #volume
            type        => SCALAR,
            optional    => 1,
        },
    );

#    $log->debug( "entered - @_" );

    my $fh      = $self->open( $key, 'read' );
    unless ( $fh ) {
        $log->debug( "can not open object" );
        $log->debug( "leaving" );

        return;
    }

    my $new_fh;
    if (defined $volume) {
        $new_fh  = $self->open_create( $new_key, $volume );
    } else {
        $new_fh  = $self->open_create( $new_key );
    }
    unless ( $new_fh ) {
        $log->debug( "can not open object" );
        $log->debug( "leaving" );

        return;
    }

    File::Copy::copy( $fh, $new_fh ) or $log->logdie( "can not copy object $key" );

    close( $fh ) or $log->logdie->( "can not close $key" );
    close( $new_fh ) or $log->logdie->( "can not close $new_key" );

    $log->debug( "leaving" );

    return 1;
}


sub move
{
    my $self = shift;

    my ( $key, $new_key ) = validate_pos( @_,
        {
            type => SCALAR,
        },
        {
            type => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->rename_object( $key, $new_key );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    $log->debug( "leaving" );

    return 1;
}


sub swap
{
    my $self = shift;

    my ( $key1, $key2 ) = validate_pos( @_,
        {
            type => SCALAR,
        },
        {
            type => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->swap_objects( $key1, $key2 );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    $log->debug( "leaving" );

    return 1;
}


sub delete_instance
{
    my $self = shift;

    my ($key, $uri) = validate_pos(@_,
        {
            type => SCALAR,
        },
        {
            type => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->delete_instance($key, $uri);
    if ( $response->fault ) {
        $self->set_err($response->faultstring);

        if ($response->faultstring =~ /no instance is associated with uri/) {
            $log->debug( "leaving" );
            return;
        }

        if ($response->faultstring =~ /is valid object key/) {
	    $log->logdie("parameter #1 to delete_instance did not pass the 'is valid object key' callback");
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    $log->debug( "server deleted instance" );

    $log->debug( "leaving" );

    return $uri;
}


sub mounts
{
    my $self = shift;

    validate_pos(@_);

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->mounts();
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        $log->logdie("unhandled fault - ", $self->err);
    }

    my $stats = $response->result;

    $log->debug( "leaving" );

    return $stats;
}


sub stat
{
    my $self = shift;

    my ( $key ) = validate_pos( @_,
        {
            type => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->stat_object( $key );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    $log->debug( "server returned a stat" );

    my $stats = $response->result;

    $log->debug( "leaving" );

    return $stats;
}


sub chmod
{
    my $self = shift;

    my ($key, $mode) = validate_pos( @_,
        {
            type => SCALAR,
        },
        {
            type => SCALAR,
            regex       => qr/\d{3,4}/,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->chmod_object($key, $mode);
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
            return;
        }
        if ($response->faultstring =~ /is allowable mode/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    my $response_mode = $response->result;

    $log->debug( "leaving" );

    return $response_mode;
}


sub prune
{
    my $self = shift;

    my ( $key ) = validate_pos( @_,
        {
            type => SCALAR,
        },
    );

    $log->debug( "entered - @_" );

    my $response = $self->{ 'server' }->prune_object( $key );
    if ( $response->fault ) {
        $self->set_err($response->faultstring);
        if ($response->faultstring =~ /is valid object key/) {
            $log->debug( "leaving" );
            return;
        }

        $log->logdie("unhandled fault - ", $self->err);
    }

    $log->debug( "server returned a stat" );

    my $n_removed = $response->result;

    $log->debug( "leaving" );

    return $n_removed;
}


sub err
{
    my $self = shift;

    die "accepts no params" if @_;

    return $self->{ 'err' };
}


sub set_err
{
    my $self = shift;

    my $err_string = shift;

    $self->{ 'err' } = $err_string;

    local $Log::Log4perl::caller_depth += 1;
    $log->error($err_string);

    return $self;
}

sub set_log_level
{
    my $self = shift;

    my $log_level = shift;

    $log->level($log_level);

    return $self;
}


1;

__END__
