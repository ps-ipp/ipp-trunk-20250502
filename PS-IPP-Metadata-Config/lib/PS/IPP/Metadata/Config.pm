# Copyright (c) 2005  Joshua Hoblitt
#
# $Id: Config.pm,v 1.32 2007-11-10 00:50:19 jhoblitt Exp $

package PS::IPP::Metadata::Config;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '1.01';

use Carp qw( carp );
use PS::IPP::Metadata::Parser;

use base qw( Class::Accessor::Fast );
__PACKAGE__->mk_accessors( qw( overwrite ) );

#$::RD_TRACE = 1;
#$::RD_HINT = 1;
#use Data::Dumper;

sub new {
    my $class = shift;

    my $self = { _parser => PS::IPP::Metadata::Parser->new };

    bless $self, $class;

    $self->overwrite( undef );

    return $self;
}

sub parse {
    my ( $self, $metadata ) = @_;

    return undef if not defined $metadata;
    return undef if $metadata =~ /^\s*$/;

    # remove any local data from a prevous run
    # this is slightly faster then using Storable::dclone to clone a new parser
    delete $self->{_parser}{local};

    my $tree = $self->{_parser}->startrule( $metadata );

    return undef unless defined $tree;

    # look for duplicate names, there should be none after processing the
    # multi-symbols
    $self->_merge_duplicates( $tree ) or return undef;

    # translate NULL values into perl undefs (Parse::RecDescent can't return
    # undefs, args)
    $self->_fix_nulls( $tree ) or return undef;

    #print Dumper($tree);

    return $tree;
}

# Parse a list of METADATAs
sub parse_list
{
    my $self = shift;           # This parser

    # Split input into separate METADATAs because the parser seems to take forever handling those together
    my @whole = split /\n/, join( '', @_ ); # The whole input
    my @single;                             # A single METADATA element
    my @output;                             # Output
    foreach my $value (@whole) {
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
            push @single, "\n";

            my $md = join("\n", @single); # Metadata to parse
            my $parsed = $self->parse($md) or (carp "Unable to parse input: $md\n" and next);
            (carp "More than one entry in input: $md\n" and next) if scalar @$parsed > 1;
            my $metadata = shift @$parsed; # Metadata of interest
            (carp "MD element is not of type METADATA: $md\n" and next) if $metadata->{class} ne "metadata";
            my $components = $metadata->{value}; # Components of metadata
            my %hash;           # Hash for entry
            foreach my $comp (@$components) {
                $hash{$comp->{name}} = $comp->{value};
            }
            push @output, \%hash;
            @single = ();
        }
    }

    return \@output;
}

sub _merge_duplicates {
    my ( $self, $tree ) = @_;

    # encountered names
    my %names;

    # Iteratate through the parse tree looking for duplicate declarations and
    # resolving them by discarding elements according to the value of
    # 'overwrite'.
    for (my $i = 0; $i < @{$tree}; $i++) {
        my $elem = $tree->[$i];

        # stop if the prevous pass removed the last element and called redo
        last unless defined $elem;

        # recurse through nested metadata
        if ( $elem->{class} eq "metadata" ) {
            $self->_merge_duplicates( $elem->{value} );
        }

        # ignore elements with the "multi" flag
        if ( defined $elem->{multi} ) {
            delete $elem->{multi};
            next;
        }

        if ( defined $names{ $elem->{name} } ) {
            if ( $self->{overwrite} ) {
                # remove the previous occurance
                carp "duplicate variable name: ", $elem->{name}
                    , ", removed previous occurance\n";
                splice @{$tree}, $names{ $elem->{name} }, 1;
            } else {
                # remove the current occurance
                carp "duplicate variable name: ", $elem->{name}
                    , ", removed\n";
                splice @{$tree}, $i, 1;
            }

            # the list just got shorter by one element so we don't want to
            # increment the cursor
            redo;
        }

        # add element name and location to record of previously seen names
        $names{ $elem->{name} } = $i;
    }

    return 1;
}

sub _fix_nulls {
    my ( $self, $tree ) = @_;

    # Iteratate through the parse tree looking for values of NULL and
    # translating them into Perl undefs
    for (my $i = 0; $i < @{$tree}; $i++) {
        my $elem = $tree->[$i];

        # recurse through nested metadata
        if ( $elem->{class} eq "metadata" ) {
            $self->_fix_nulls( $elem->{value} );
        }

        # force stringification of $elem->{value} -- specifically because you
        # can't compare a DateTime object to a string value
        if ("$elem->{value}" eq "NULL") {
            $elem->{value} = undef;
        }
    }

    return 1;
}

1;

__END__
