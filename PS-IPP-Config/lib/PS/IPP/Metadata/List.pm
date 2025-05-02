# Copyright (c) 2006  Paul Price, Joshua Hoblitt
#
# $Id: List.pm,v 1.1 2006-08-12 04:26:12 price Exp $

package PS::IPP::Metadata::List;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '0.01';

use Carp qw( carp );

# Allow exporting of function
use base qw( Exporter );
our @EXPORT_OK = qw( parse_md_list );

#$::RD_TRACE = 1;
#$::RD_HINT = 1;
#use Data::Dumper;

# Given a parsed metadata, parse it into an array of hashes
sub parse_md_list {
    my $md = shift;		# Parsed metadata, from PS::IPP::Metadata::Config

    my @array;			# The array of hashes, to be returned
    foreach my $mdItem (@$md) {
	if ($mdItem->{class} ne "metadata") {
	    carp "MD element ", $mdItem->{name}, " isn't of type METADATA --- ignored.\n";
	    next;
	}
	my %hash;		# Hash element
	my $mdComponents = $mdItem->{value}; # Components of the metadata
	foreach my $data (@$mdComponents) {
	    $hash{$data->{name}} = $data->{value};
	}
	push @array, \%hash;
    }

    return \@array;
}

1;

__END__;

