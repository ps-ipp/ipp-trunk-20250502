# Copyright (c) 2004  Joshua Hoblitt
#
# $Id: HTTP.pm,v 1.2 2005-06-30 02:35:06 jhoblitt Exp $

package Nebulous::Client::HTTP;

use strict;

our $VERSION = '0.01';

use base qw( SOAP::Lite );

sub call {
    my $result = $_[0]->SUPER::call( @_ );

    if (( UNIVERSAL::isa( $result, 'SOAP::SOM' )) && $result->fault ) {

        return $result->faultstring;
    }

    return $result;
}

1;

__END__
