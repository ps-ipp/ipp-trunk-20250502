# Copyright (c) 2006  Paul Price, Joshua Hoblitt
#
# $Id: Stats.pm,v 1.33 2008-08-06 02:35:40 price Exp $

package PS::IPP::Metadata::Stats;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '0.02';

use Carp qw( carp );
use Statistics::Descriptive;

use PS::IPP::Config qw(
    $PS_EXIT_SUCCESS
    $PS_EXIT_UNKNOWN_ERROR
    $PS_EXIT_SYS_ERROR
    $PS_EXIT_CONFIG_ERROR
    $PS_EXIT_PROG_ERROR
    $PS_EXIT_DATA_ERROR
    $PS_EXIT_TIMEOUT_ERROR
    );

use base qw( Class::Accessor::Fast );
__PACKAGE__->mk_accessors( qw(
                               results
                               ) );

#$::RD_TRACE = 1;
#$::RD_HINT = 1;
use Data::Dumper;

sub new {
    my $class = shift;          # Class name
    my $entries = shift;        # Array of values to extract

    my $self = {
        entries => $entries, # Array of values that should be constant through the FPA
        data => {}              # The data
    };

    # add a hash for storage
    foreach my $entry (@$entries) {
        $entry->{data} = [];
    }

    # Bless and return object
    bless $self, $class;
    return $self;
}

sub cmdflags {

    my $self = shift;                # Where we'll put the information

    # apply the needed calculation to the data based on the type

    my $cmdflags;
    my $entries = $self->{entries};

    foreach my $entry (@$entries) {
        my $value = $entry->{value};
        my $flag = $entry->{flag};
        next unless defined $value and defined $flag;
        if ($value =~ m|\S+\s+\S+|) {
            # protect values with whitespace
            $cmdflags .= " $flag '$value'";
        } else {
            $cmdflags .= " $flag $value";
        }
    }

    return $cmdflags;
}

# Given a parsed metadata from ppStats, assemble summary statistics
sub parse {
    my $self = shift;                # Where we'll put the information
    my $md = shift;                # Parsed metadata, from PS::IPP::Metadata::Config

    $self->_parse_metadata_stats($md);

    # apply the needed calculation to the data based on the type

    my $entries = $self->{entries};

    foreach my $entry (@$entries) {
        my $type = $entry->{type};
        my $data = $entry->{data};

        if ($type eq "constant") {
            if (not defined $entry->{value}) {
                $self->_null_for_type ($entry);
            }
            next;
        }

        if ($type eq "mean") {
            if (scalar @$data > 0) {
                # Get statistics on the value
                my $stats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                $stats->add_data(@$data);
                $entry->{value} = $stats->mean();
            } else {
                $self->_null_for_type ($entry);
            }
            next;
        }

        # calculate a 3-sigma clipped mean (if Nvalues > 8, reject > 3sigma outliers)
        if ($type eq "clipmean") {
            if (scalar @$data > 0) {
                # Get statistics on the value
                my $stats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                $stats->add_data(@$data);
                my $rawMean = $stats->mean();
                my $rawStdev = $stats->standard_deviation();

                # clip, if we can
                if (scalar @$data > 8) {
                    my $rawLimit = 3.0*$rawStdev;
                    my @clipdata = ();
                    foreach my $value (@$data){
                        my $delta = abs($value - $rawMean);
                        if ($delta > $rawLimit) { next; }
                        push @clipdata, $value;
                    }
                    my $clipstats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                    $clipstats->add_data(@clipdata);
                    $entry->{value} = $clipstats->mean();
                } else {
                    $entry->{value} = $stats->mean();
                }
            } else {
                $self->_null_for_type ($entry);
            }
            next;
        }

        if ($type eq "stdev") {
            if (scalar @$data > 1) {
                # Get statistics on the value
                my $stats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                $stats->add_data(@$data);
                $entry->{value} = $stats->standard_deviation();
                next;
            }
            if (scalar @$data == 1) {
                $entry->{value} = 0.0;
                next;
            }
            $self->_null_for_type ($entry);
            next;
        }

        # calculate a 3-sigma clipped stdev (if Nvalues > 8, reject > 3sigma outliers)
        if ($type eq "clipstdev") {
            if (scalar @$data > 0) {
                # Get statistics on the value
                my $stats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                $stats->add_data(@$data);
                my $rawMean = $stats->mean();
                my $rawStdev = $stats->standard_deviation();

                # clip, if we can
                if (scalar @$data > 8) {
                    my $rawLimit = 3.0*$rawStdev;
                    my @clipdata = ();
                    foreach my $value (@$data){
                        my $delta = abs($value - $rawMean);
                        if ($delta > $rawLimit) { next; }
                        push @clipdata, $value;
                    }
                    my $clipstats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                    $clipstats->add_data(@clipdata);
                    $entry->{value} = $clipstats->standard_deviation();
                } else {
                    $entry->{value} = $stats->standard_deviation();
                }
            } else {
                $self->_null_for_type ($entry);
            }
            next;
        }

        if ($type eq "rms") {
            if (scalar @$data > 0) {
                # Get statistics on the value
                # for rmsStats, we pushed the value^2 on the data array
                my $stats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                $stats->add_data(@$data);
                $entry->{value} = sqrt($stats->mean());
            } else {
                $self->_null_for_type ($entry);
            }
            next;
        }

        if ($type eq "sum") {
            if (scalar @$data > 0) {
                # Get statistics on the value
                my $stats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                $stats->add_data(@$data);
                $entry->{value} = $stats->sum();
            } else {
                $self->_null_for_type ($entry);
            }
            next;
        }

        if ($type eq "max") {
            if (scalar @$data > 0) {
                # Get statistics on the value
                my $stats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                $stats->add_data(@$data);
                $entry->{value} = $stats->max();
            } else {
                $self->_null_for_type ($entry);
            }
            next;
        }

        if ($type eq "min") {
            if (scalar @$data > 0) {
                # Get statistics on the value
                my $stats = Statistics::Descriptive::Sparse->new(); # Statistics calculator
                $stats->add_data(@$data);
                $entry->{value} = $stats->min();
            } else {
                $self->_null_for_type ($entry);
            }
            next;
        }

    }

    return $self;
}

# Return 1 if a metadata item value is effectively NULL
sub _is_null_for_type
{
    my ($item) = @_;            # Metadata item

    return 1 unless defined $item->{type} and defined $item->{value};

    if (($item->{type} eq "F32" or $item->{type} eq "F64") and lc($item->{value} eq 'nan')) {
        return 1;
    }
    if ($item->{type} eq "STR" and lc($item->{value}) eq 'null') {
        return 1;
    }
    return 0;
}

sub _null_for_type {

    my ($self, $entry) = @_;

    if ($entry->{dtype} eq "float") {
        $entry->{value} = 'NAN';
        return;
    }
    if ($entry->{dtype} eq "int") {
        $entry->{value} = '0';
        return;
    }
    if ($entry->{dtype} eq "string") {
        $entry->{value} = 'NULL';
        return;
    }
    $entry->{value} = 'NAN';
    return;
}

sub _parse_metadata_stats
{
    my ($self, $md) = @_;

    # descend through the fpa
    foreach my $entry (@$md) {
        # recurse on nested metadata
        if ($entry->{class} eq 'metadata') {
            $self->_parse_metadata_stats($entry->{value});
        }
        #if (defined $entry->{value}) {
        #    print STDERR "found $entry->{name} : $entry->{value}\n";
        #}
        $self->_check_values($entry);
   }

    return $self;
}

# Private function to check a metadata item for things we're interested in
sub _check_values {
    my $self = shift;
    my $mdItem = shift;                # Metadata item to check

    my $name = $mdItem->{name};        # Name of the item
    my $value = $mdItem->{value}; # Value of the item

    if ( _is_null_for_type($mdItem) and $mdItem->{class} eq 'scalar' ) {
        # print "Stats: Ignoring NULL entry $name\n";
        return;
    }

    my $entries = $self->{entries};        # The data

    # we may request the value more than once (different stats on the same value)
    foreach my $entry (@$entries) {
        if ($entry->{name} eq $name) {

            my $type = $entry->{type}; # Type of the item: constant or variable
            my $data = $entry->{data}; # Array containing all the values

            if ($type eq 'constant') {
                if (defined $entry->{value} && ($entry->{value} ne $value)) {
                    carp "Warning: different value for ", $name, " found: ", $entry->{value}, " (old) vs ", $value, " (new).  Ignoring new value.\n";
                    next;
                }
                $entry->{value} = $value;
                next;
            }

            if ($type eq 'mean') {
                push @$data, $value;
                next;
            }

            if ($type eq 'clipmean') {
                push @$data, $value;
                next;
            }

            if ($type eq 'stdev') {
                push @$data, $value;
                next;
            }

            if ($type eq 'clipstdev') {
                push @$data, $value;
                next;
            }

            if ($type eq 'rms') {
                push @$data, ($value**2);
                next;
            }

            if ($type eq 'sum') {
                push @$data, $value;
                next;
            }

            if ($type eq 'max' or $type eq 'min') {
                push @$data, $value;
                next;
            }

            carp "Unrecognised type (", $type, ") for value of ", $name, "\n";
            exit($PS_EXIT_PROG_ERROR);
        }
    }

    return;
}

sub value_for_flag {

    my $self = shift;
    my $flag = shift;

    my $entries = $self->{entries};

    foreach my $entry (@$entries) {
        if ($flag eq $entry->{flag}) {
            return $entry->{value};
        }
    }
    return 'NULL'; #
}


# Return the data structure for a particular value, or the entire hash
sub data {
    my $self = shift;
    my $name = shift;                # Name of data to return

    return $self->{data} if (not defined $name);
    return $self->{data}->{$name};
}


1;

__END__;

