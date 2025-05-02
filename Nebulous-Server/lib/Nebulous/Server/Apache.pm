# Copyright (C) 2004  Joshua Hoblitt
#
# $Id

package Nebulous::Server::Apache;

use strict;
use warnings FATAL => qw( all );

our $VERSION = 0.01;

use Nebulous::Server::SOAP;
use SOAP::Transport::HTTP;

my $server = SOAP::Transport::HTTP::Apache
    ->dispatch_to( 'Nebulous::Server::SOAP' )
    ->options({ compress_threshold => 10 * 1024 });

no warnings qw( redefine );
sub handler { $server->handler(@_); }
use warnings;

1;

__END__
