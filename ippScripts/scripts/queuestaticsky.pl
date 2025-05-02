#!/usr/bin/env perl

use strict;
use warnings;

use Carp;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use POSIX qw(strftime);
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

my ($seq_id, $tess_id, $projection_cell, $label, $dist_group, $dbname, $pretend, $simple, $verbose, $no_update);

my $good_frac_min = 0.05;

GetOptions(
    'seq_id=s'           =>  \$seq_id,
    'tess_id=s'          =>  \$tess_id,
    'projection_cell=s'  =>  \$projection_cell,
    'label=s'            =>  \$label,
    'dist_group=s'       =>  \$dist_group,
    'dbname=s'           =>  \$dbname,
    'pretend'            =>  \$pretend,
    'simple'             =>  \$simple,
    'no-update'          =>  \$no_update,
    'verbose|v'          =>  \$verbose,
) or pod2usage(2);

unless (defined $seq_id and defined $tess_id and defined $projection_cell and defined $label and defined $dist_group) {
    warn ("label, seq_id, tess_id, and projection_cell are required\n");
    pod2usage(2);
};

# don't update the database if we are pretending
$no_update = 1 if $pretend;

my $missing_tools;
my $laptool = can_run('laptool') or (warn "Can't find laptool" and $missing_tools = 1);
my $staticskytool = can_run('staticskytool') or (warn "Can't find staticskytool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $ipprc = PS::IPP::Config->new() or my_die( "Unable to set up", $PS_EXIT_CONFIG_ERROR );

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# XXX get from site.config
my $workdirBase = "neb://\@HOST\@.0/gpc1";

my @filters;
{
    my $command = "$laptool -filtersforgroup";
    $command .= " -seq_id $seq_id";
    $command .= " -tess_id $tess_id";
    $command .= " -projection_cell $projection_cell";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => 0);
    unless ($success) {
        $error_code = $error_code >> 8;
        my_die("failed to run $command $error_code\n", $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $PS_EXIT_PROG_ERROR);
    my $list = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $PS_EXIT_PROG_ERROR);

    foreach my $entry (@$list) {
        push @filters, $entry->{filter};
    }
}

my $nFilters = scalar @filters;
print STDERR "lapGroup $seq_id $tess_id $projection_cell has $nFilters filters\n";

&my_die("No filters found for $seq_id $tess_id $projection_cell", $PS_EXIT_PROG_ERROR) unless $nFilters;
&my_die("Unexpected number of filters found for $seq_id $tess_id $projection_cell", $PS_EXIT_PROG_ERROR) unless $nFilters <= 5;

my $datestr = strftime "%Y/%m/%d", gmtime;

my $staticsky_command="staticskytool -definebyquery"
    . " -select_label $label"
    . " -select_tess_id $tess_id"
    . " -select_skycell_id $projection_cell%"
    . " -set_workdir $workdirBase/$label/$datestr"
    . " -select_good_frac_min $good_frac_min"
    . " -set_label $label"
    . " -set_data_group $label"
    . " -set_dist_group $dist_group";

$staticsky_command .= " -pretend" if $pretend;
$staticsky_command .= " -simple" if $simple;
$staticsky_command .= " -dbname $dbname" if $dbname;

for (my $num = $nFilters; $num > 0; $num--) {

    # set up the possible combinations of $num filters from @filters

    my $filter_combos = setup_filter_combos($num);

    # printcombos($filter_combos) if $verbose;

    foreach my $filter_combo (@$filter_combos) {
        my $command = $staticsky_command;
        foreach my $f (@$filter_combo) {
            $command .= " -select_filter $f";
        }
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = $error_code >> 8;
            my_die("failed to run $command $error_code\n", $error_code);
        }
    }
    last if $pretend;   # only do the first (full) set if we are in pretend mode
}
{
    my $command = "$laptool -updategroup";
    $command .= " -seq_id $seq_id";
    $command .= " -tess_id $tess_id";
    $command .= " -projection_cell $projection_cell";
    $command .= " -set_state full";
    $command .= " -dbname $dbname" if defined $dbname;
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = $error_code >> 8;
            carp("failed to run $command $error_code\n", $error_code);
            exit $error_code;
        }
    } else {
        print STDERR "Skipping $command\n";
    }
}

exit 0;

sub my_die
{
    my $msg = shift;
    my $exit_code = shift;

    $exit_code = $PS_EXIT_PROG_ERROR unless $exit_code;

    carp($msg);

    my $command = "$laptool -updategroup";
    $command .= " -seq_id $seq_id";
    $command .= " -tess_id $tess_id";
    $command .= " -projection_cell $projection_cell";
    $command .= " -set_fault $exit_code";
    $command .= " -dbname $dbname" if defined $dbname;
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = $error_code >> 8;
            carp("failed to run $command $error_code\n", $error_code);
            exit $error_code;
        }
    } else {
        print STDERR "Skipping $command\n";
    }
    exit $exit_code;
}


# The rest of this file contains some subroutines for setting up the filter combinations


sub setup_filter_combos {
    my $combos = shift;

    my_die( "invalid combos arg: $combos", $PS_EXIT_PROG_ERROR) 
        unless defined $combos and ($combos eq "5" or $combos eq "4" or $combos eq "3" or $combos eq "2" or $combos eq "1");

    if ($combos eq 5) {
        my @ary;
        push @ary, \@filters;
        return \@ary;
    } elsif ($combos eq 4) {
        return makecombo4();
    } elsif ($combos eq 3) {
        return makecombo3();
    } elsif ($combos eq 2) {
        return makecombo2();
    } elsif ($combos eq 1) {
        my @ary;
        foreach my $f (@filters) {
            my @ary2 = ($f);
            push @ary, \@ary2
        }
        return \@ary;
    } else {
        die "how did we get here?";
    }
}



sub printcombos {
    my $combos = shift;
    foreach my $c (@$combos) {
        my $str;
        foreach my $f (@$c) {
            $str .= " -select_filter $f";
        }
        print "$str\n";
    }
}


sub makecombo4 {
    my $numFilters = scalar @filters;
    my @combos;

    for (my $i = 0; $i < $numFilters; $i++) {
        for (my $j = $i + 1; $j < $numFilters; $j++) {
            for (my $k = $j + 1; $k < $numFilters; $k++) {
                for (my $l = $k + 1; $l < $numFilters; $l++) {
                    my @combo = ($filters[$i], $filters[$j], $filters[$k], $filters[$l]);
                    push @combos, \@combo;
                }
            }
        }
    }
    return \@combos;
}

sub makecombo3 {
    my $numFilters = scalar @filters;
    my @combos;

    for (my $i = 0; $i < $numFilters; $i++) {
        for (my $j = $i + 1; $j < $numFilters; $j++) {
            for (my $k = $j + 1; $k < $numFilters; $k++) {
                my @combo = ($filters[$i], $filters[$j], $filters[$k]);
                push @combos, \@combo;
            }
        }
    }
    return \@combos;
}

sub makecombo2 {
    my $numFilters = scalar @filters;
    my @combos;

    for (my $i = 0; $i < $numFilters; $i++) {
        for (my $j = $i + 1; $j < $numFilters; $j++) {
            my @combo = ($filters[$i], $filters[$j]);
            push @combos, \@combo;
        }
    }
    return \@combos;
}


__END__

=pod

=head1 NAME

queuesskylap - queue LAP staticsky runs 

=head1 SYNOPSIS
    
    XXX: pod TODO

    queuesskylap --ra_min <ra_min> --ra_max <ra_max> --dec_min <dec_min> --dec_max <dec_max> [--go]


