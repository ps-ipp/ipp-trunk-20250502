#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

use File::Basename;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# tools
my $missing_tools;
my $ssh        = can_run('ssh')  or (warn "Can't find ssh" and $missing_tools = 1);
my $scp        = can_run('scp')  or (warn "Can't find scp" and $missing_tools = 1);
my $remotetool = can_run('remotetool') or (warn "Can't find remotetool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Options
my ($remote_id,$job_id,$path_base,$dbname,$verbose,$no_update,$camera,$cmd_recipe);
$verbose = 0;
GetOptions(
    'remote_id=s'   => \$remote_id,
    'path_base=s'   => \$path_base,
    'camera=s'      => \$camera,
    'dbname=s'      => \$dbname,
    'recipe=s'       => \$cmd_recipe,
    'verbose'       => \$verbose,
    'no_update'     => \$no_update,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id --path_base --recipe", -exitval => 3) unless
    defined($path_base) and
    defined($camera) and
    defined($cmd_recipe) and
    defined($dbname) and
    defined($remote_id);

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

# Hard coded values
my $DMZ_HOST = $remote_recipe{DMZ_HOST};
my @SEC_HOSTS = ();
my $SEC_HOST = '';

if (defined($remote_recipe{SEC_HOST})) {
    @SEC_HOSTS = @{ $remote_recipe{SEC_HOST} };
    if ($#SEC_HOSTS != -1) {
	
	$SEC_HOST = $SEC_HOSTS[int(rand(@SEC_HOSTS))]; 
    }
    else {
	$SEC_HOST = '';
    }
}
my $IPP_PATH = $remote_recipe{IPP_PATH};
my $remote_root  = $remote_recipe{REMOTE_ROOT};

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $remote_id);
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Phase 1: See if we can actually do anything.
# If the link is down, there's no benefit in trying to do anything else.
&check_ssh_connection();

print "passed authentication challenge.\n";

# Phase 1.5: Grab the information about this run;
my $rt_cmd = "$remotetool -listrun -remote_id $remote_id ";
$rt_cmd   .= " -dbname $dbname " if defined($dbname);
my ( $rt_success, $rt_error_code, $rt_full_buf, $rt_stdout_buf, $rt_stderr_buf ) =
    run(command => $rt_cmd, verbose => $verbose);
unless ($rt_success) {
    $rt_error_code = (($rt_error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to run remotetool to determine remote run status", $remote_id, $rt_error_code);
}
my $rrData = $mdcParser->parse(join "", @$rt_stdout_buf) or
    &my_die("Unable to run remotetool to determine remote run status", $remote_id, $rt_error_code);
my $rrData2 = parse_md_list($rrData);
my $runData = ${ $rrData2 }[0]; # There should be only one

my ($remote_path) = &uri_local_to_remote($path_base);
print "REMOTE PATH: $remote_path\n";

# Phase 2: Ensure files are in place:
# Copy command files
my @files = ();

my $uri_command = $path_base . ".cmd";
my $uri_transfer= $path_base . ".transfer";
my $uri_check   = $path_base . ".check";
my $uri_config  = $path_base . ".config";
my $uri_generate= $path_base . ".generate";
my $uri_return  = $path_base . ".return";

my $disk_command = $ipprc->file_resolve($uri_command);
my $disk_transfer= $ipprc->file_resolve($uri_transfer);
my $disk_check   = $ipprc->file_resolve($uri_check);
my $disk_config  = $ipprc->file_resolve($uri_config);
my $disk_generate= $ipprc->file_resolve($uri_generate);
my $disk_return  = $ipprc->file_resolve($uri_return);

&scp_put($disk_command,  &uri_local_to_remote($uri_command));
&scp_put($disk_transfer, &uri_local_to_remote($uri_transfer));
&scp_put($disk_check,    &uri_local_to_remote($uri_check));
&scp_put($disk_config,   &uri_local_to_remote($uri_config));
&scp_put($disk_generate, &uri_local_to_remote($uri_generate));
&scp_put($disk_return,   &uri_local_to_remote($uri_return));

my $ssh_check_stdout = &ssh_exec_command("${remote_root}/sc_transfer_tool.pl --input $remote_path --fetch");

# We no longer need to parse this output, as it retrieves files itself.
foreach my $l (split /\n/, (join '', @$ssh_check_stdout)) {
    print "$l\n";
}

# Run real command
my (undef,$remote_command) = &uri_convert($uri_command);
my $ssh_exec_stdout;

if ($remote_recipe{SCHEDULER} eq 'MOAB') {
    $ssh_exec_stdout = &ssh_exec_command("msub -V $remote_command");
}
elsif ($remote_recipe{SCHEDULER} eq 'SLURM') {
    $ssh_exec_stdout = &ssh_exec_command("sbatch $remote_command");
}
else {
    &my_die("No scheduler defined", $remote_id, 0, 'pending');
}
if ($#{ $ssh_exec_stdout } != -1) { # Parse the output
    my $line = ${ $ssh_exec_stdout }[0];
    chomp($line);
    my @line_split = split /\s+/, $line;
    $job_id = $line_split[-1];
    $job_id =~ s/\s+//g;
}

unless(defined($job_id)) { # If we don't have a job_id from this command, it didn't run correctly.
    &my_die("No job_id returned.  Sorry.", $remote_id, $PS_EXIT_PROG_ERROR, "exec_fail");
}

# Notify the database that this entry is currently running.
&my_die("Finished", $remote_id, 0, "run", $job_id);

# END PROGRAM

sub check_ssh_connection {
    my $cmd = "$ssh -O check $DMZ_HOST";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Authorization check failed.", $remote_id, 0, 'pending');
    }
}

sub scp_put {
    my $file = shift;
    my $destination = shift;
    my $cmd;
    if ($SEC_HOST ne '') {
	$cmd = "$scp $file ${DMZ_HOST}:${SEC_HOST}:${destination}";
    }
    else {
        $cmd = "$scp $file $DMZ_HOST:${destination}";
    }

    my $directory = dirname($destination);
    &ssh_exec_command("mkdir -p $directory");

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("failed to send file $file to LANL", $remote_id, $error_code);
    }
}

sub scp_get {
    my $destination = shift;
    my $file = shift;
    my $cmd;
    if ($SEC_HOST ne '') {
	$cmd = "$scp ${DMZ_HOST}:${SEC_HOST}:${destination} $file ";
    }
    else {
        $cmd = "$scp $DMZ_HOST:${destination}  $file";
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("failed to get file $file from LANL", $remote_id, $error_code);
    }
}

sub ssh_exec_command {
    my $cmd = shift;
    if ($SEC_HOST ne '') {
	$cmd = "$ssh -n $DMZ_HOST ssh  ${SEC_HOST} $cmd";
    }
    else {
        $cmd = "$ssh -n $DMZ_HOST $cmd";
    }

    print "EXEC: $cmd\n";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        # If we failed, see if we failed due to authorization.
        check_ssh_connection();
        &my_die("Failed to execute remote command: >>$cmd<<", $remote_id, $error_code);
    }
    return ($stdout_buf);
}

# neb URI to (ipp_disk,remote_disk)
sub uri_convert {
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri );
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die("Failed to generate or find uri $neb_uri", $remote_id, $PS_EXIT_SYS_ERROR);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

# neb URI to remote URI-on-disk
sub uri_local_to_remote {
    # This needs to replace the nebulous tag with the remote root.
    my $local_uri = shift;
    $local_uri =~ s%^.*?/%%; # neb:/
    $local_uri =~ s%^.*?/%%; # /
    $local_uri =~ s%^.*?/%%; # @HOST@.0/
    my $remote_uri = "${remote_root}/" . $local_uri;

    return($remote_uri);
}

sub my_die {
    my $msg = shift;
    my $id  = shift;
    my $exit_code = shift;
    my $exit_state = shift;
    my $jobid = shift;
    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);

    if (defined $id and not $no_update) {
        my $command = "$remotetool -updaterun -remote_id $id";
        $command .= " -fault $exit_code " if defined $exit_code;
        $command .= " -job_id $job_id " if defined $jobid;
        $command .= " -set_state $exit_state " if defined $exit_state;
        $command .= " -dbname $dbname " if defined $dbname;

        system($command);
    }

    exit($exit_code);
}

# Quick review:
# new -> pending -> run -> full
# auth
