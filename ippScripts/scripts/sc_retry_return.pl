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

# Hard coded values
my $DMZ_HOST = 'wtrw';
my $SEC_HOST = 'mu-fe';
my $IPP_PATH = '/turquoise/usr/projects/cosmo/mswarren/ipp/';
my $remote_root  = '/scratch3/watersc1/';

# tools
my $missing_tools;
my $ssh        = can_run('ssh')  or (warn "Can't find ssh" and $missing_tools = 1);
my $scp        = can_run('scp')  or (warn "Can't find scp" and $missing_tools = 1);
my $remotetool = can_run('remotetool') or (warn "Can't find remotetool" and $missing_tools = 1);
my $chiptool   = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Options
my ($remote_id,$path_base,$dbname,$verbose,$no_update,$camera);
$verbose = 0;
GetOptions(
    'remote_id=s'   => \$remote_id,
    'path_base=s'   => \$path_base,
    'camera=s'      => \$camera,
    'dbname=s'      => \$dbname,
    'verbose'       => \$verbose,
    'no_update'     => \$no_update,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id --path_base", -exitval => 3) unless
    defined($path_base) and
    defined($remote_id);

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
    &my_die("Unable to run remotetool to determine remote run status",
            $remote_id,$rt_error_code);
}

my $rrData = $mdcParser->parse(join "", @$rt_stdout_buf) or
    &my_die("Unable to run remotetool to determine remote run status",
            $remote_id,$rt_error_code);
my $rrData2 = parse_md_list($rrData);
my $runData = ${ $rrData2 }[0]; # There should be only one

# my job ID :
my $job_id = $runData->{job_id};
print "remote job ID: $job_id\n";

# Poll the job status? (check status at LANL)
my $poll_response = 0;
my $poll_word  = "nothing";
($poll_response, $poll_word) = poll_job($job_id);

if ($poll_response == -1) { # This job has an error
    print "The job exited incorrectly: $remote_id $job_id $poll_word $poll_response.\n";
    # warn("The job exited incorrectly: $remote_id $job_id $poll_word.");
    exit (2);
}

if ($poll_response != 1) { # This job has NOT completed
    print "The job has not exited at LANL: $remote_id $job_id $poll_word $poll_response.\n";
    exit (2);
}

# Initialize the remote side to sync things
my $uri_return  = $path_base . ".return";
my ($ipp_return, $remote_return) = uri_convert($uri_return);

# Construct list of dbinfo files we'll need to execute.
my @returned_files = ();
my @dbinfo_files   = ();

# get a list of the dbinfo files
open(RETURN, "$ipp_return") || &my_die("Couldn't open file? $ipp_return", $remote_id, $PS_EXIT_SYS_ERROR);
while (<RETURN>) {
    chomp;
    if ($_ =~ /dbinfo/) {
        push @dbinfo_files, $_;
    }
}
close(RETURN);
print STDERR "Expect $#dbinfo_files dbinfo files\n";

# Feed the return list into the transfer tool
# If this fails, we should exit : do not update gpc1 database until all files are back
{
    my $offset = 0;  # Offset to start from.
    my $iter = 0;
    my $transfer_success = 1;
    do {
        $transfer_success = 1;
        $iter++;
        check_ssh_connection();
        my $return_push_output = ssh_exec_command("${remote_root}/sc_transfer_tool.pl --input $remote_return --offset $offset");
        my $return_push_output_print = join "\n", @{ $return_push_output };
        if ($return_push_output_print =~ /exited with value 255/) { $transfer_success = 0; }
        print "$return_push_output_print\n";
    } while (($transfer_success != 1) &&  ($iter < 10));  # Try harder;

    if (!$transfer_success) { &my_die ("failed to get all return files"); }
}
print STDERR "rsync of all files is complete, updating gpc1 database\n";

foreach my $file (@dbinfo_files) {

    # use the dbinfo filename to get stage id, check if we have already updated this entry:
    my ($stage_id, $class_id) = get_stage_id_from_dbinfo ($file);

    # chiptool -dbname -chip_id $stage_id -class_id $class_id

    # we just need to check for the existence of an entry:
    if ($runData->{stage} == "chip") {
        my $cmd1 = "$chiptool -simple -processedimfile -chip_id $stage_id -class_id $class_id";
        $cmd1   .= " -dbname $dbname " if defined($dbname);

        my ( $success1, $error_code1, $full_buf1, $stdout_buf1, $stderr_buf1 ) =
            run(command => $cmd1, verbose => $verbose);

        unless ($success1) {
            $error_code1 = (($error_code1 >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to run chiptool to check on results", $remote_id, $error_code1);
        }

        my $Nrows = @$stdout_buf1;
        if ($Nrows > 0) {
            print STDERR "skip this entry: $stage_id, $class_id\n";
            next;
        }
    } else {
        print STDERR "do not know how to handle stage $runData->{stage}\n";
        exit (3);
    }

    open(DBF,"$file") || warn "Missing file $file\n";
    my $cmd = <DBF>;
    close(DBF);

    chomp($cmd);

    $cmd =~ s/-/ -/g; # This is just a safety check for missing space.
    if ($cmd == "") {
        print STDERR "empty dbinfo file for $stage_id, $class_id, skipping\n";
        next;
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        # This shouldn't fail, but we also can't suddenly abort if something does fail.
        # Now we're going to drop the component that owns this dbinfo, which should force a retry.
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("The command that shouldn't fail has failed: $file $cmd $remote_id $error_code");
        if ($poll_word ne 'Completed') {  # If the job claims it completed, then we're fighting the network.
            # drop_component($remote_id,$cmd,$file);
            print STDERR "we should drop this component: $cmd\n";
        }
    }
}

# We're done, so set the state and exit.
&my_die("Finished",$remote_id,0,"full");

# END PROGRAM

# yes
sub check_ssh_connection {
    my $cmd = "$ssh -O check $DMZ_HOST";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Authorization check failed", $remote_id, 0, 'auth');
    }
}

sub ssh_exec_command {
    my $cmd = shift;
    $cmd = "$ssh  $DMZ_HOST ssh  ${SEC_HOST} $cmd";
    print "EXEC: $cmd\n";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        # If we failed, see if we failed due to authorization.
        check_ssh_connection();
        &my_die("Failed to execute command: >>$cmd<<");
    }
    return ($stdout_buf);
}

sub uri_convert {
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri );
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        my_die();
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk, $remote_disk);
}

sub poll_job {
    my $job_id = shift;
    my $response_array = ssh_exec_command("checkjob -v $job_id");
    my $response = join "\n", @$response_array;

    if ($response =~ /State: Running/) {
        return(0,"Running");
    }
    elsif ($response =~ /State: Completed/) {
        return(1,"Completed");
    }
    elsif ($response =~ /State: Idle/) {
        return(-2,"Idle");
    }
    else {
        my $response_word = $response;
        $response_word =~ s/.*State: //;
        $response_word =~ s/\n.*//;
        return(-1,$response_word);
    }
}


# assumes $runData->{stage} exists
sub get_stage_id_from_dbinfo {
    my $file = shift;

    my $stage_id = 0;
    my $class_id = 0;

    # Steal the stage_id from the filename.  This isn't something I'm happy about.
    my $stage = $runData->{stage};
    if ($stage eq 'chip') {
        ($stage_id, $class_id) = $file =~ /ch\.(\d*).(XY\d\d).dbinfo/;
    }
    elsif ($stage eq 'camera') {
        $stage_id =~ s/^.*cm\.//;
        $class_id = 0;
    }
    elsif ($stage eq 'warp') {
        $stage_id =~ s/^.*wrp\.//;
        $class_id = 0;
    }
    elsif ($stage eq 'stack') {
        $stage_id =~ s/^.*stk\.//;
        $class_id = 0;
    }

    return ($stage_id, $class_id);
}

sub drop_component {
    my $remote_id = shift;
    my $cmd = shift;
    my $file = shift;

    if ($cmd eq '') {
        # Zero byte file.  This dbinfo was never constructed properly.

        # Steal the stage_id from the filename.  This isn't something I'm happy about.
        my $stage = $runData->{stage};
        my $stage_id = $file;
        if ($stage eq 'chip') {
            $stage_id =~ s/^.*ch\.//;
        }
        elsif ($stage eq 'camera') {
            $stage_id =~ s/^.*cm\.//;
        }
        elsif ($stage eq 'warp') {
            $stage_id =~ s/^.*wrp\.//;
        }
        elsif ($stage eq 'stack') {
            $stage_id =~ s/^.*stk\.//;
        }
        $stage_id =~ s/\..*$//;

        # Drop this from the database
        my $drop_cmd = "$remotetool -dropcomponent -remote_id $remote_id -stage_id $stage_id ";
        $drop_cmd   .= " -dbname $dbname " if defined($dbname);
        system($drop_cmd);
    }

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
