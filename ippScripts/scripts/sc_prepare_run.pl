#! /usr/bin/env perl

# this script takes a list of run components ready to be run
# (remoteComponent.state=prep_done) and generates the master run
# scripts for that set.  Components which have
# (remoteComponent.state=prep_fail) are skipped

use Carp;
use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use DateTime;
use Data::Dumper;
use File::Basename;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

#use Data::Dumper;
# Look for programs we need
my $missing_tools;
my $remotetool = can_run('remotetool') or (warn "Can't find remotetool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Options
my ($remote_id,$cmd_recipe,$stage,$camera,$path_base,$dbname,$verbose,$no_update);
GetOptions(
    'remote_id=s'    => \$remote_id,
    'recipe=s'       => \$cmd_recipe,
    'stage=s'        => \$stage,
    'camera|c=s'     => \$camera,
    'path_base=s'    => \$path_base,
    'dbname|d=s'     => \$dbname,
    'verbose'        => \$verbose,
    'no_update'      => \$no_update,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id --stage --camera --dbname --path_base --recipe", -exitval => 3) unless
    defined($remote_id) and
    defined($stage) and
    defined($camera) and
    defined($path_base) and
    defined($cmd_recipe) and
    defined($dbname) and
    defined($dbname);

# Hard coded values
# Now accessible from a recipe
my %remote_recipe = ();
{
    my $verbose = 0;
    my $conf_cmd = "$ppConfigDump -dump-recipe REMOTE -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $conf_cmd, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform ppConfigDump: $error_code", -1, $PS_EXIT_SYS_ERROR);
    }
    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $metadata = $mdcParser->parse(join "", @$stdout_buf);

    my $active_recipe = '';
    my %recipes = ();
    
#    print Dumper($metadata);
    foreach my $entry (@{ $metadata }) {
	if (${ $entry }{name} eq 'ACTIVE') {
	    $active_recipe = ${ $entry }{value}; # Not actually used
	}
	else {
	    if (${ $entry }{class} eq 'metadata') { # A real recipe
		my $name = ${ $entry }{name};
		foreach my $tentry (@{ ${ $entry }{value} }) {
		    if (${ $tentry }{class} eq 'scalar') { # A recipe value
			$recipes{$name}{${ $tentry }{name}} = ${ $tentry }{value};
		    }
		    elsif (${ $tentry }{class} eq 'metadata') { # A recipe array 
			foreach my $arr_entry (@{ ${ $tentry }{value} }) {
			    push @{ $recipes{$name}{${ $tentry }{name}} }, ${ $arr_entry }{value};
			}
		    }
		}
	    }
	}
    }
    
    unless (exists($recipes{$cmd_recipe})) { &my_die("Cannot find recipe $cmd_recipe", -1, $PS_EXIT_CONFIG_ERROR) };
#    print Dumper(%recipes);
    %remote_recipe = %{ $recipes{$cmd_recipe} }; # Select the appropriate recipe.
#    print Dumper(\%remote_recipe);
}

my $remote_root = $remote_recipe{REMOTE_ROOT};
my $remote_raw  = "${remote_root}/tmp/";  # Directory to find raw data in.

my $threads     = 2;                      # How many threads are we going to use?

my $fail_state = "prep_fail";

# Estimate of how long a job runs, in hours (depends on stage)
my %job_cost = ();
$job_cost{"camera"} = 1700 / 60 / 60;
$job_cost{"chip"}   = 150 / 60 / 60;
$job_cost{"warp"}   = 110 / 60 / 60;
$job_cost{"stack"}  = 1500 / 60 / 60;
$job_cost{"staticsky"} = 2; # really?  avg time of 5720s?
$job_cost{"diff"}   = 300 / 60 / 60;
$job_cost{"ff"}     = 300 / 60 / 60; # This is an 83%-ile point, but the tail is long.

# This object holds how "expensive" a given job is in terms of cores on a node.  
# Passed to stask to limit how many jobs run simultaneously
my %job_subscription = ();  
$job_subscription{"camera"} = 1;
$job_subscription{"chip"}   = 1;
$job_subscription{"warp"}   = 1;
$job_subscription{"stack"}  = 3;
$job_subscription{"staticsky"} = 3;
$job_subscription{"diff"} = 1;
$job_subscription{"ff"} = 1;

my $proc_per_node = $remote_recipe{PROC_PER_NODE};  # processors per node
my $min_nodes     = $remote_recipe{MIN_NODES};      # smallest allocation to ask for
my $max_nodes     = $remote_recipe{MAX_NODES};      # largest allocation to ask for
my $min_time      = $remote_recipe{MIN_TIME};       # shortest allocation to ask for
my $max_time      = $remote_recipe{MAX_TIME};       # longest allocation to ask for

# We need to ensure we only ever try to transfer a file once.
my %file_filter = ();


# Finish setup
my $ipprc = PS::IPP::Config->new( $camera ) or &my_die( "Unable to set up", $remote_id, $PS_EXIT_CONFIG_ERROR, $fail_state);

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# STEP 1: Get a list of the components that make up this remoteRun
my $compData;
{
    my $command = "$remotetool -listcomponent -remote_id $remote_id -state prep_done";
    $command   .= " -dbname $dbname " if defined($dbname);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);

        &my_die("Unable to run remotetool to determine stage parameters.", $remote_id, $error_code, $fail_state);
    }
    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine component information.", $remote_id, $PS_EXIT_PROG_ERROR, $fail_state);
    $compData = parse_md_list($MDlist);
}

# STEP 2: Open master output files
my $uri_command = $path_base . ".cmd";
my $uri_transfer= $path_base . ".transfer";
my $uri_check   = $path_base . ".check";
my $uri_config  = $path_base . ".config";
my $uri_generate= $path_base . ".generate";
my $uri_return  = $path_base . ".return";

my $disk_command = $ipprc->file_resolve($uri_command,1);
my $disk_transfer= $ipprc->file_resolve($uri_transfer,1);
my $disk_check   = $ipprc->file_resolve($uri_check,1);
my $disk_config  = $ipprc->file_resolve($uri_config,1);
my $disk_generate= $ipprc->file_resolve($uri_generate,1);
my $disk_return  = $ipprc->file_resolve($uri_return,1);

my %have_transfer = ();
my %have_check    = ();
my %have_config   = ();
my %have_generate = ();
my %have_return   = ();

my (undef, $remote_config) = uri_convert($uri_config); # Needs to be done after we've created it.

open(TRANSFER, ">$disk_transfer")  || &my_die("Couldn't open file? $disk_transfer",$remote_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CHECK,    ">$disk_check")     || &my_die("Couldn't open file? $disk_check",   $remote_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CONFIG,   ">$disk_config")    || &my_die("Couldn't open file? $disk_config",  $remote_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(GENERATE, ">$disk_generate")  || &my_die("Couldn't open file? $disk_generate",$remote_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(RETURN,   ">$disk_return")    || &my_die("Couldn't open file? $disk_return",  $remote_id, $PS_EXIT_SYS_ERROR, $fail_state);

# Step 2: Iterate over all componenets in this remote run.
my $job_index = 0;
foreach my $compEntry (@$compData) {
    my $line;

    my $stage_id = $compEntry->{stage_id};

    my $in_path_base = $compEntry->{path_base};

    my $state = $compEntry->{state};
    if ($state eq 'fail') { next; }

    my $uri_transfer = $in_path_base . ".transfer";
    my $disk_transfer= $ipprc->file_resolve($uri_transfer);
    open(INPUT, "$disk_transfer");
    while ($line = <INPUT>) {
        unless ($have_transfer{$line}) {
            $have_transfer{$line} = 1;
            print TRANSFER $line;
        }
    }
    close (INPUT);

    my $uri_check = $in_path_base . ".check";
    my $disk_check= $ipprc->file_resolve($uri_check);
    open(INPUT, "$disk_check");
    while ($line = <INPUT>) {
        unless ($have_check{$line}) {
            $have_check{$line} = 1;
            print CHECK $line;
        }
    }
    close (INPUT);

    my $uri_config = $in_path_base . ".config";
    my $disk_config= $ipprc->file_resolve($uri_config);
    open(INPUT, "$disk_config");
    while ($line = <INPUT>) {
        unless ($have_config{$line}) {
            $have_config{$line} = 1;
            print CONFIG $line;
        }
    }
    close (INPUT);

    my $uri_generate = $in_path_base . ".generate";
    my $disk_generate= $ipprc->file_resolve($uri_generate);
    open(INPUT, "$disk_generate");
    while ($line = <INPUT>) {
        unless ($have_generate{$line}) {
            $have_generate{$line} = 1;
            print GENERATE $line;
        }
    }
    close (INPUT);

    my $uri_return = $in_path_base . ".return";
    my $disk_return= $ipprc->file_resolve($uri_return);
    open(INPUT, "$disk_return");
    while ($line = <INPUT>) {
        unless ($have_return{$line}) {
            $have_return{$line} = 1;
            print RETURN $line;
        }
    }
    close (INPUT);

    if (($job_index % 100) == 0) {
	print CONFIG "top -b -n 1\n";
    }

    $job_index += $compEntry->{jobs};
}

close(CONFIG);
close(TRANSFER);
close(CHECK);
close(RETURN);
close(GENERATE);

# Construct the moab command last, so we can use the job_index counter to estimate resources.  Somehow.
my $proc_need = $job_index * $threads;       # how many total processors do we need?
my $node_need = $proc_need / $proc_per_node; # this equals how many nodes?
my $time_need = $job_index * $job_cost{$stage};      # How many seconds will this take?

my $fill_factor = 0.8;  # This is the factor of how much of the time allocation we'd like to fill
my ($time_req,$node_req);
if ($node_need * $job_cost{$stage} < $fill_factor * $min_nodes * $min_time) {
    $time_req = $min_time;
    $node_req = $min_nodes;
}
elsif ($node_need * $job_cost{$stage} > $fill_factor * $max_nodes * $max_time) {
    $time_req = $max_time;
    $node_req = $max_nodes;
    print STDERR "You've requested the construction of a bundle that appears to need $node_need nodes and $job_cost{$stage} time per job.  This exceeds the max limits ($max_nodes, $max_time).  Using those max values instead.  Good luck.\n";
}
else {
    $time_req = int(($node_need * $job_cost{$stage}) / ($fill_factor * $max_nodes)) + 1;
    $node_req = int(($node_need * $job_cost{$stage}) / ($fill_factor * $time_req)) + 1;
}

# Most jobs only take 1 core, but ensure that we split them appropriately
$node_req *= $job_subscription{$stage};

if (($stage eq "stack")||($stage eq "staticsky")) {
    $time_req += 2;
} else {
    $time_req += 1; # Safety addition.
}

open(COMMAND,  ">$disk_command") || &my_die("Couldn't open file? $disk_command", $remote_id, $PS_EXIT_SYS_ERROR, $fail_state);
print COMMAND "#!/bin/tcsh\n";
print COMMAND "##### Moab controll lines\n";
if ($remote_recipe{SCHEDULER} eq 'MOAB') {
    print COMMAND "#MSUB -l nodes=${node_req}:ppn=${proc_per_node},walltime=${time_req}:00:00\n"; ## CHECK RESOURCES
    print COMMAND "#MSUB -j oe\n";
    print COMMAND "#MSUB -V\n";
    print COMMAND "#MSUB -o ${remote_root}/stask_logs/${stage}.${remote_id}.out\n";
}
elsif ($remote_recipe{SCHEDULER} eq 'SLURM') {
    print COMMAND "###### sbatch control lines\n";
    print COMMAND "#SBATCH --time ${time_req}:00:00\n";
    print COMMAND "#SBATCH --nodes ${node_req}\n";
    print COMMAND "#SBATCH --open-mode=append\n";
    print COMMAND "#SBATCH --export=ALL\n";
    print COMMAND "#SBATCH --exclusive\n";
    print COMMAND "#SBATCH -o ${remote_root}/stask_logs/${stage}.${remote_id}.out\n";
}
else {
    &my_die("No scheduler defined", $remote_id, 0, 'fail_state');
}

print COMMAND "date\n";
print COMMAND 'srun -n $SLURM_JOB_NUM_NODES -m cyclic -l /bin/hostname | sort -n | awk \'{printf "%s\n", $2}\' > hosts.${SLURM_JOB_ID}' . "\n";
if (($stage eq "stack")||($stage eq "staticsky")) {
    print COMMAND "${remote_root}/stask_stack.py $remote_config " . 'hosts.${SLURM_JOB_ID} '  . $job_subscription{$stage} . "\n";
}
else {
    print COMMAND "${remote_root}/stask_chip.py $remote_config " . 'hosts.${SLURM_JOB_ID} '  . $job_subscription{$stage} . "\n";
}
print COMMAND "date\n";
close(COMMAND);

## We're done here. The execution and handling are done elsewhere.
# Quick review:
# new -> pending -> run -> full
# auth
unless($no_update) {
    my $command = "remotetool -updaterun -remote_id $remote_id ";
    $command .= " -set_state pending ";
    $command .= " -dbname $dbname " if defined $dbname;

    system($command);
}

exit (0);

sub uri_convert {
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri );
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $PS_EXIT_SYS_ERROR, $fail_state);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

sub my_die {
    my $msg = shift;
    my $remote_id  = shift;
    my $exit_code = shift;
    my $exit_state = shift;

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);

    if (defined $remote_id and not $no_update) {
        my $command = "remotetool -updaterun -remote_id $remote_id";
        $command .= " -fault $exit_code " if defined $exit_code;
        $command .= " -set_state $exit_state " if defined $exit_state;
        $command .= " -dbname $dbname " if defined $dbname;

        system($command);
    }

    exit($exit_code);
}

