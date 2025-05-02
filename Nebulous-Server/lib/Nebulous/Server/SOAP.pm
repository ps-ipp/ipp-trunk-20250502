# Copyright (c) 2004  Joshua Hoblitt
#
# $Id: SOAP.pm,v 1.4.32.1 2008-12-14 22:52:37 eugene Exp $

package Nebulous::Server::SOAP;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '0.02';

import SOAP::Data 'name'; 
use Apache2::Const -compile => qw(OK);
use Nebulous::Server;
use SOAP::Lite;

our $AUTOLOAD;

our $config;
our $neb;


sub new_on_init
{
    my $self = shift;

    require mod_perl2;
    require Apache2::Module;
    require Apache2::ServerUtil;

    $config = shift;

    my $s = Apache2::ServerUtil->server;
    $s->push_handlers(PerlChildInitHandler => \&init);

    return $self;
}


sub init
{
    my $self = shift;

    $neb = Nebulous::Server->new_from_config($config);        

    return Apache2::Const::OK;
}


sub AUTOLOAD
{
    my $self = shift;

    my ( $package, $method ) = $AUTOLOAD =~ m/(?:(.+)::)([^:]+)$/;
    return undef if $method eq 'DESTROY';

    init() unless defined $neb;

    die "process $$ has not been initialized"
        unless defined $neb;

    die "method $method does not exist in Nebulous::Server"
        unless defined ${Nebulous::Server::}{$method};

    # create a sub and install it in the package so successive calls to the
    # same method don't have to go through AUTOLOAD
    my $method_sub;
    eval q|
    $method_sub = sub {
        my $self = shift;
        return name( result => $neb->| . $method . q|( @_ ) );
    }
    |;
    die $@ if $@;

    no strict 'refs';
    *{"${package}::${method}"} = $method_sub;
    use strict;

    # invoke the method we just created
    return $self->$method(@_);
}


1;
