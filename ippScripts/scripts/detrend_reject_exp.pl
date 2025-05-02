#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Config 1.01 qw( :standard );
use PS::IPP::Metadata::List qw( parse_md_list );
use Statistics::Descriptive;

my $ITER_LIMIT = 20;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $dettool = can_run('dettool') or (warn "Can't find dettool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($det_id, $iter, $det_type, $camera, $outroot, $filter, $dbname, $verbose, $no_update, $no_op, $redirect);
GetOptions(
    'det_id|d=s'        => \$det_id,
    'iteration=s'       => \$iter,
    'det_type|t=s'      => \$det_type,
    'camera=s'          => \$camera,
    'outroot|w=s'       => \$outroot,   # output file base name
    'filter=s'          => \$filter,
    'dbname|d=s'        => \$dbname, # Database name
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update,
    'no-op'             => \$no_op,
    'redirect-output'   => \$redirect,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --det_id --iteration --det_type --camera --outroot",
           -exitval => 3) unless
    defined $det_id   and
    defined $iter     and
    defined $det_type and
    defined $camera   and
    defined $outroot;

$det_type = uc($det_type);

my $ipprc = PS::IPP::Config->new() or my_die( "Unable to set up", $det_id, $iter, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
$ipprc->outroot_prepare($outroot) or my_die( "Unable to prepare output root", $det_id, $iter, $PS_EXIT_SYS_ERROR );
my $logName = "$outroot.log"; # Name for log
$ipprc->redirect_output($logName) or my_die( "Unable to redirect", $det_id, $iter, $PS_EXIT_SYS_ERROR ) if $redirect;

# values to extract from output metadata and the stats to calculate
# XXX -bg_mean_stdev should take rms of bg_mean_stdev if bg_mean_stdev != 0 (A)
# XXX -bg_mean_stdev should take stdev of bg_mean if bg_mean_stdev == 0     (B)
# XXX  (A) if imfile.Ncomp > 1, (B) if imfile.Ncomp == 1
my $STATS =
   [
       #          KEYWORD                 STATISTIC          CHIPTOOL FLAG
       { name => "bg",             type => "mean",  flag => "-bg",            dtype => "float" },
       { name => "bg_mean_stdev",  type => "stdev", flag => "-bg_mean_stdev", dtype => "float" },
       { name => "bg_stdev",       type => "rms",   flag => "-bg_stdev",      dtype => "float" },
   ];
my $stats = PS::IPP::Metadata::Stats->new($STATS); # Stats parser

# these stats are used it the rejections but not passed to the database
# there is some duplication with the above, but the calculation time is minimal
my $REJSTATS =
   [
       #          KEYWORD                 STATISTIC          CHIPTOOL FLAG
       { name => "bg",             type => "clipmean",  flag => "ensMeanMean",       dtype => "float" },
       { name => "bg",             type => "clipstdev", flag => "ensMeanStdev",      dtype => "float" },
       { name => "bg_mean_stdev",  type => "clipmean",  flag => "ensMeanStdevMean",  dtype => "float" },
       { name => "bg_mean_stdev",  type => "clipstdev", flag => "ensMeanStdevStdev", dtype => "float" },
       { name => "bg_stdev",       type => "clipmean",  flag => "ensStdevMean",      dtype => "float" },
       { name => "bg_stdev",       type => "clipstdev", flag => "ensStdevStdev",     dtype => "float" },
       { name => "bg_skewness",    type => "clipmean",  flag => "ensSkewness",       dtype => "float" },
       { name => "bg_kurtosis",    type => "clipmean",  flag => "ensKurtosis",       dtype => "float" },

   ];
my $rejstats = PS::IPP::Metadata::Stats->new($REJSTATS); # Stats parser

# Get list of component files
my ($exposures, $command, $success, $error_code, $full_buf, $stdout_buf, $stderr_buf);
{
    # dettool command to select exp data for this det_run
    $command = "$dettool -residexp";
    $command .= " -det_id $det_id";
    $command .= " -iteration $iter";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool -residexp: $error_code", $det_id, $iter, $error_code);
    }

    # Parse the stdout buffer into a metadata
    my $mdcParser = PS::IPP::Metadata::Config->new;     # Parser for metadata config files
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $det_id, $iter, $PS_EXIT_PROG_ERROR);

    # parse the file info in the metadata
    $exposures = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $det_id, $iter, $PS_EXIT_PROG_ERROR);

    # Parse the statistics on the residual image
    $stats->parse($metadata) or &my_die("Unable to find all values in statistics output.", $det_id, $iter, $PS_EXIT_PROG_ERROR);

    # Parse the statistics for rejections
    $rejstats->parse($metadata) or &my_die("Unable to find all values in statistics output.", $det_id, $iter, $PS_EXIT_PROG_ERROR);
}

# each image has a mean mean (average of means over all chips)
# a standard deviation mean (average of standard deviations over all chips)
# and a



# we use the statistics of the ensemble to accept/reject exposurs
my $ensMeanMean       = $rejstats->value_for_flag ("ensMeanMean");       # average of all exposure means (in turn averaged over all chips)
my $ensMeanStdev      = $rejstats->value_for_flag ("ensMeanStdev");      # standard deviation of all exposure means
my $ensMeanStdevMean  = $rejstats->value_for_flag ("ensMeanStdevMean");  # average over all exposures of the stdev of the means for each chip
my $ensMeanStdevStdev = $rejstats->value_for_flag ("ensMeanStdevStdev"); # standard deviation over all exposures of the stdev of the means for each chip
my $ensStdevMean      = $rejstats->value_for_flag ("ensStdevMean");      # average over all exposures of the sum of the squares of the stdevs for each chip
my $ensStdevStdev     = $rejstats->value_for_flag ("ensStdevStdev");     # standard deviation over all exposures of the sum of the squares of the stdevs for each chip

$ipprc->define_camera($camera);
# Rejection thresholds
my $reject_mean      = rejection_limit( 'ENSEMBLE.MEAN',      $det_type, $filter );
my $reject_stdev     = rejection_limit( 'ENSEMBLE.STDEV',     $det_type, $filter );
my $reject_meanstdev = rejection_limit( 'ENSEMBLE.MEANSTDEV', $det_type, $filter );

# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

unless ($no_op) {
    print "Ensemble mean $ensMeanMean +/- $ensMeanStdev\n";
    print "Ensemble stdev $ensStdevMean +/- $ensStdevStdev\n";
    print "Ensemble mean rms (over imfiles) $ensMeanStdevMean +/- $ensMeanStdevStdev\n\n";
}

# Go through again to do rejection, and update the database for each exposure
my $numChanges = 0;             # Number of exposures with changed status
my $numReject = 0;              # Number of exposures rejected
my $firstElement = 1;

foreach my $exposure (@$exposures) {
    my $mean      = $exposure->{bg};    # Mean for this exposure
    my $stdev     = $exposure->{bg_stdev}; # Stdev for this exposure
    my $meanStdev = $exposure->{bg_mean_stdev}; # Stdev of Means for this exposure
    my $expID     = $exposure->{exp_id};
    my $accept    = $exposure->{accept};
    my $include   = $exposure->{include};

    &my_die("Unable to find exposure id.\n", $det_id, $iter, $PS_EXIT_SYS_ERROR) unless defined $expID;
    &my_die("Unable to find accept.\n",      $det_id, $iter, $PS_EXIT_SYS_ERROR) unless defined $accept;
    &my_die("Unable to find include.\n",     $det_id, $iter, $PS_EXIT_SYS_ERROR) unless defined $include;

    my $reject = 0;             # Reject this exposure?

    $command  = "$dettool -updateresidexp";
    $command .= " -det_id $det_id";
    $command .= " -iteration $iter";
    $command .= " -exp_id $expID";
    $command .= " -dbname $dbname" if defined $dbname;

    if (not $accept) {
        # Rejected this at an earlier stage
        unless ($no_op) {
            print "Rejecting $expID based on earlier determination.\n";
        }
        $reject = 1;
        goto UPDATE;
    }

    # Cop-out if we're not doing any operations
    if ($no_op) {
        # Make sure something gets rejected (just once!), just so that
        # we can trace the full range of the workflow
        if ($firstElement and $iter == 0) {
            $reject = 1;
        }
        goto UPDATE;
    }

    if ($reject_mean > 0 and $ensMeanStdev > 0) {
        my $delta = abs($mean - $ensMeanMean);
        if ($delta > ($reject_mean * $ensMeanStdev)) {
            print "Rejecting $expID based on ensemble mean value: ";
            $reject = 1;
            #goto UPDATE;
        } else {
            print "$expID OK against ensemble mean: ";
        }
        print "$mean --> $delta vs " . $reject_mean * $ensMeanStdev . "\n";
    } else {
        print "No rejection of $expID for ensemble mean\n";
    }

    if ($reject_stdev > 0 and $ensStdevStdev > 0) {
        my $delta = abs($stdev - $ensStdevMean);
        if ($delta > ($reject_stdev * $ensStdevStdev)) {
            print "Rejecting $expID based on ensemble stdev: ";
            $reject = 1;
            #goto UPDATE;
        } else {
            print "$expID OK against ensemble stdev: ";
        }
        print "$stdev --> $delta sigma vs " . $reject_stdev * $ensStdevStdev . "\n";
    } else {
        print "No rejection of $expID for ensemble stdev\n";
    }

    if ($reject_meanstdev > 0 and $ensMeanStdevStdev > 0) {
        my $delta = abs($meanStdev - $ensMeanStdevMean);
        if ($delta > ($reject_meanstdev * $ensMeanStdevStdev)) {
            print "Rejecting $expID based on ensemble mean stdev: ";
            $reject = 1;
            #goto UPDATE;
        } else {
            print "$expID OK against ensemble mean stdev: ";
        }
        print "$meanStdev --> $delta sigma vs " . $reject_meanstdev * $ensMeanStdevStdev. "\n";
    } else {
        print "No rejection of $expID for ensemble mean stdev\n";
    }

  UPDATE:
    if ($reject) {
        $command .= ' -reject';
        $numReject++;
    }

    # Check for status changes
    if ((not $include and not $reject) or ($include and $reject)) {
        unless ($no_op) {
            print "Status of $expID has changed.\n";
        }
        $numChanges++;
    }

    unless ($no_update) {
        # Update
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform dettool -updateresidexp: $error_code", $det_id, $iter, $error_code);
        }
    }
}

# Decide if the current is sufficient to use as a master, and if we can stop iterating
my $master = 1;                 # This is good enough for a master
my $stop = 1;                   # Stop iterating

if ($numChanges > 0) {
    $master = 0;
    $stop = 0;
}

# Rejecting everything --- stop before something bad happens!
if ($numReject == scalar @$exposures) {
    $master = 0;
    $stop = 1;
    carp "All inputs rejected!\n";
}

unless ($no_op) {
    print "Master: $master\n";
    print "Stop: $stop\n";
}

# Allow iteration to be turned off
my $allow_iter = metadataLookupBool($ipprc->{rejection}, "ITERATION"); # Allow iteration?
my $force_master = metadataLookupBool($ipprc->{rejection}, "MASTER"); # Force the stack to be accepted
$stop = 1 unless $allow_iter;
$master = 1 if $force_master and $stop;

# attempt to prevent endless, pathological iterations
if ($iter >= $ITER_LIMIT) {
    warn("iteration limit reached -- bailing out");
    exit($PS_EXIT_PROG_ERROR);
}

## add the summary statistics, and request a new iteration if needed
$command = "$dettool -adddetrunsummary";
$command .= " -det_id $det_id";
$command .= " -iteration $iter";
$command .= " -accept" if $master;
$command .= " -dbname $dbname" if defined $dbname;
$command .= " -again" unless $stop;
$command .= $stats->cmdflags();

# Put results into the database
unless ($no_update) {
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -adddetrunsummary: $error_code");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}


sub my_die
{
    my $msg = shift; # Warning message on die
    my $det_id = shift;         # Detrend identifier
    my $iter = shift;           # Iteration
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $iter and not $no_update) {
        my $command = "$dettool -adddetrunsummary";
        $command .= " -det_id $det_id";
        $command .= " -iteration $iter";
        # XXX EAM : we should add this to the db : $command .= " -path_base $outroot";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}

# Retrieve the requested rejection limit, dying unless extant
sub rejection_limit
{
    my $name = shift;           # Rejection limit to
    my $type = shift;           # Type of exposure
    my $filter = shift;         # Filter

    my $value = $ipprc->rejection( $name, $det_type, $filter );
    if (not defined $value) {
        $filter = "(no filter)" unless defined $filter;
        die "Unable to determine $name rejection limit for $det_type with $filter.\n";
    }

    return $value;
}

__END__
