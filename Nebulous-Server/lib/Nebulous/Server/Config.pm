# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: Config.pm,v 1.5 2008-12-14 22:54:25 eugene Exp $

package Nebulous::Server::Config;

use strict;
use warnings FATAL => qw( all );

our $VERSION = 0.03;

use base qw( Class::Accessor::Fast );

use Log::Log4perl qw( :levels );
use Params::Validate qw( validate validate_pos SCALAR ARRAYREF UNDEF );

our %LEVELS = (
    off     => $OFF,
    fatal   => $FATAL,
    error   => $ERROR,
    warn    => $WARN,
    info    => $INFO,
    debug   => $DEBUG,
    all     => $ALL,
);

my $db_validate = {
    dbindex       => {
        type => SCALAR,
        regex => qr/^\d+$/,
    },
    dsn         => { type => SCALAR },
    dbuser      => { type => SCALAR },
    dbpasswd    => { type => SCALAR | UNDEF },
};

my $new_validate = {
    trace       => {
        type        => SCALAR,
        optional    => 1,
        default     => 'fatal',
        callbacks   => {
            'is valid level' => sub {
                defined $LEVELS{ lc $_[0] };
            },
        },
    },
    dsn                 => { type => SCALAR, optional => 1 },
    dbuser              => { type => SCALAR, optional => 1 },
    dbpasswd            => { type => SCALAR | UNDEF, optional => 1 },
    memcached_servers   => { type => ARRAYREF, optional => 1 },
};

__PACKAGE__->mk_ro_accessors( keys %$new_validate );


sub new
{
    my $class = shift;

    my %p = validate( @_, $new_validate );

    # normalize log levels to lower-case
    my $self = {
        trace             => $LEVELS{lc($p{trace})},
        memcached_servers => $p{memcached_servers},
    };

    bless $self, $class || ref $class;

    my @dbs;
    $self->{dbs} = \@dbs;

    if (defined $p{dsn} or defined $p{dbuser} or defined $p{dbpasswd}) {
        $self->add_db(
            dbindex     => 0,
            dsn         => $p{dsn},
            dbuser      => $p{dbuser},
            dbpasswd    => $p{dbpasswd},
        );
    }

    return $self;
}


sub add_db
{
    my $self = shift;
    
    my %p = validate( @_, $db_validate );

    my $config_db = Nebulous::Server::Config::DB->new({
        dsn         => $p{dsn},
        dbuser      => $p{dbuser},
        dbpasswd    => $p{dbpasswd},
    });

    $self->{dbs}->[$p{dbindex}] = $config_db;

    return $self;
}


sub db 
{
    my $self = shift;

    my ($db_index) = validate_pos( @_, { type => SCALAR, optional => 1, });

    # default to 0
    $db_index ||= 0;

    return $self->{dbs}->[$db_index];
}


sub n_db 
{
    my $self = shift;

    return scalar @{ $self->{dbs} };
}


package Nebulous::Server::Config::DB;

use strict;
use warnings FATAL => qw( all );

our $VERSION = 0.01;

use base qw( Class::Accessor::Fast );

__PACKAGE__->mk_ro_accessors(qw( dsn dbuser dbpasswd )); 

1;

__END__
