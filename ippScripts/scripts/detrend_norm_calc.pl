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

use IPC::Cmd 0.36 qw( can_run );
use IPC::Run 0.36 qw( run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $dettool = can_run('dettool') or (warn "Can't find dettool" and $missing_tools = 1);
my $ppNormCalc = can_run('ppNormCalc') or (warn "Can't find ppNormCalc" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse command-line arguments
my ($det_id, $iter, $det_type, $outroot, $dbname, $verbose, $no_update, $no_op, $redirect );
GetOptions(
    'det_id|d=s'        => \$det_id,    # Detrend id
    'iteration|i=s'     => \$iter,      # Iteration
    'det_type|t=s'      => \$det_type,   # Detrend type
    'outroot|w=s'       => \$outroot,   # output file base name
    'dbname|d=s'        => \$dbname,    # Database name
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op,     # Don't do operations
    'redirect-output'   => \$redirect,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options --det_id --iteration --det_type --outroot",
           -exitval => 3,
           ) unless
    defined $det_id  and
    defined $iter    and
    defined $det_type and
    defined $outroot;

# force det_type to be upper-case in this script
$det_type = uc($det_type);

my $ipprc = PS::IPP::Config->new() or my_die( "Unable to set up", $det_id, $iter, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logfile = $outroot . ".log";
$ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $det_id, $iter, $PS_EXIT_SYS_ERROR ) if $redirect;

use constant STATISTIC => 'bg'; # Background statistic to use from the database

# Define which detrend types we normalise
use constant NORMALIZE => {
    'BIAS'             => 0,
    'DARK'             => 0,
    'DARK_PREMASK'     => 0,
    'SHUTTER'          => 0,
    'FLAT_PREMASK'     => 1,
    'DOMEFLAT_PREMASK' => 1,
    'SKYFLAT_PREMASK'  => 1,
    'FLAT_RAW'         => 1,
    'DOMEFLAT_RAW'     => 1,
    'SKYFLAT_RAW'      => 1,
    'SKYFLATTEST_RAW'  => 1,
    'FLAT'             => 1,
    'FLATTEST'         => 1,
    'DOMEFLAT'         => 1,
    'SKYFLAT'          => 1,
    'FRINGE'           => 0,
    'MASK'             => 0,
    'DARKMASK'         => 0,
    'FLATMASK'         => 0,
    'CTEMASK'          => 0,
    'DARKTEST'         => 0,
    'NOISEMAP'         => 0,
    };

&my_die("Unrecognised detrend type: $det_type", $det_id, $iter, $PS_EXIT_PROG_ERROR) unless exists NORMALIZE()->{$det_type};

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Get the list of inputs
my @files;                      # The input files
{
    my $command = "$dettool -residimfile";
    $command .= " -det_id $det_id";
    $command .= " -iteration $iter";
    $command .= " -included"; # only use the inputs for this detrend run to calculate the norm
    $command .= " -dbname $dbname" if defined $dbname;
    my @command = split /\s+/, $command;
    my ( $stdin, $stdout, $stderr ); # Buffers for running program
    print "Running [$command]...\n" if $verbose;
    if (not run(\@command, \$stdin, \$stdout, \$stderr)) {
        &my_die("Unable to perform dettool -residimfile on detrend $det_id/$iter: $?",
                $det_id, $iter, $PS_EXIT_SYS_ERROR);
    }
    print $stdout . "\n" if $verbose;

    # Because of the length, need to split into individual metadatas --- it parses SO much quicker!
    my @whole = split /\n/, $stdout;
    my @single = ();
    while ( scalar @whole > 0 ) {
        my $value = shift @whole;
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
            push @single, "\n";

            my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) );
            &my_die("Unable to parse output from dettool", $det_id, $iter, $PS_EXIT_PROG_ERROR) unless
                defined $list;
            push @files, @$list;
            @single = ();
        }
    }
}

my $norms;                      # MDC with normalisations
if (NORMALIZE()->{$det_type} and not $no_op) {

    my %matrix; # Matrix of statistics as a function of exposures and classes
    foreach my $file (@files) {
        my $exp_id = $file->{'exp_id'}; # Exposure ID
        my $class_id = $file->{'class_id'}; # Class ID
        my $stat = $file->{STATISTIC()}; # Statistic of interest

        # Create matrix elements
        $matrix{$exp_id} = {} if not defined $matrix{$exp_id};
        $matrix{$exp_id}->{$class_id} = $stat;
    }

    # Generate the input for ppNormCalc
    my $normData;                       # Normalisation data
    foreach my $exp_id (keys %matrix) {
        $normData .= "$exp_id\tMETADATA\n";
        foreach my $class_id (keys %{$matrix{$exp_id}}) {
            $normData .= "\t" . $class_id . "\tF32\t" . $matrix{$exp_id}->{$class_id} . "\n";
        }
        $normData .= "END\n\n";
    }

    # Run ppNormCalc
    {
        my ( $stdout, $stderr ); # Buffers for running program
        my @command = split /\s+/, $ppNormCalc;
        print "Running [$ppNormCalc]...\n" if $verbose;
        if (not run(\@command, \$normData, \$stdout, \$stderr)) {
            &my_die("Unable to perform ppNormCalc: $?", $det_id, $iter, $PS_EXIT_SYS_ERROR);
        }
        print $stdout . "\n" if $verbose;

        # Parse the output
        $norms = $mdcParser->parse($stdout);
        &my_die("Unable to parse metadata config doc", $det_id, $iter, $PS_EXIT_PROG_ERROR) unless $norms;
    }

} else {
    # It's something that doesn't need normalisation --- just push in a normalisation of 1
    my %classes;                # List of unique classes
    foreach my $file (@files) {
        my $class_id = $file->{'class_id'}; # Class Id
        $classes{$class_id} = 1;
    }

    foreach my $class_id (keys %classes) {
        my %mdValue;    # Metadata value for this class id
        $mdValue{name} = $class_id;
        $mdValue{value} = 1.0;
        push @$norms, \%mdValue;
    }
}

my $commandBase = "$dettool -addnormalizedstat";
$commandBase .= " -det_id $det_id";
$commandBase .= " -iteration $iter";
$commandBase .= " -dbname $dbname" if defined $dbname;

# Process output normalisations
unless ($no_update) {
    foreach my $normItem (@$norms) {

        my $className = $normItem->{name}; # Name of component
        my $normalisation = $normItem->{value}; # Normalisation for component

        if ($normalisation == 0.0 or lc($normalisation) eq 'nan') {
            warn("Class $className has bad normalisation: $normalisation");
            # exit($PS_EXIT_SYS_ERROR);
        }

        my $command = $commandBase;
        $command .= " -class_id $className";
        $command .= " -norm $normalisation";

        my @command = split /\s+/, $command;

        my ( $stdin, $stdout, $stderr ); # Buffers for running program
        print "Running [$command]...\n" if $verbose;
        if (not run \@command, \$stdin, \$stdout, \$stderr) {
            warn("Unable to perform dettool -addnormstat for $className: $?");
            # exit($PS_EXIT_SYS_ERROR);
        }
        print $stdout . "\n" if $verbose;
    }
} else {
    print "skipping command: $commandBase\n";
    foreach my $normItem (@$norms) {

        my $className = $normItem->{name}; # Name of component
        my $normalisation = $normItem->{value}; # Normalisation for component

        if ($normalisation == 0.0 or lc($normalisation) eq 'nan') {
            warn("Class $className has bad normalisation: $normalisation");
            # exit($PS_EXIT_SYS_ERROR);
        }
        print "$className : $normalisation\n";
    }
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $det_id = shift;         # Detrend identifier
    my $iter = shift;           # Iteration
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $iter and not $no_update) {
        my $command = "$dettool -addnormalizedstat";
        $command .= " -det_id $det_id";
        $command .= " -iteration $iter";
        # XXX EAM : we should add this to the db : $command .= " -path_base $outroot";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}


END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
