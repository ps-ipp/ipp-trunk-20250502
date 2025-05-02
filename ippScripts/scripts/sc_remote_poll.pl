#!/usr/bin/env perl

# we have a remoteRun with an associated job_id in state of run
# 1) poll until MOAB says the processing has completed
# 2) check the status of the jobs by looking for dbinfo files on the remote server
# 3) return a list of the dbinfo files + status (exists, missing)
# 4) generate a subset transfer file with only the items that have dbinfo files?
# 5) rsync back the valid files

# alternative method:
# 1) poll for completion as above
# 2) check for the existence of each file, generate a return table with only existing files
# 3) rsync back the existing files
# 4) run the retrieved dbinfo files

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
my $ssh        = can_run('ssh')        or (warn "Can't find ssh"        and $missing_tools = 1);
my $scp        = can_run('scp')        or (warn "Can't find scp"        and $missing_tools = 1);
my $remotetool = can_run('remotetool') or (warn "Can't find remotetool" and $missing_tools = 1);
my $chiptool   = can_run('chiptool')   or (warn "Can't find chiptool"   and $missing_tools = 1);
my $camtool    = can_run('camtool')    or (warn "Can't find camtool"    and $missing_tools = 1);
my $warptool   = can_run('warptool')   or (warn "Can't find warptool"   and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Options
my ($remote_id,$path_base,$skip_poll,$skip_transfer,$job_id,$dbname,$retry,$verbose,$no_update,$camera,$cmd_recipe);
$verbose = 0;
GetOptions(
    'remote_id=s'   => \$remote_id,
    'job_id=s'      => \$job_id,
    'path_base=s'   => \$path_base,
    'skip_poll'     => \$skip_poll,
    'skip_transfer' => \$skip_transfer,
    'camera=s'      => \$camera,
    'dbname=s'      => \$dbname,
    'recipe=s'       => \$cmd_recipe,
    'retry'         => \$retry,
    'verbose'       => \$verbose,
    'no_update'     => \$no_update,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id --job_id --path_base --recipe", -exitval => 3) unless
    defined($path_base) and
    defined($remote_id) and
    defined($cmd_recipe) and
    defined($job_id);



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


my $ipprc = PS::IPP::Config->new( $camera ) or &my_die( "Unable to set up", $remote_id);
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# STEP 1: See if we can actually do anything.
# If the link is down, there's no benefit in trying to do anything else.
&check_ssh_connection();

print "passed authentication challenge.\n";

# STEP 2: Grab the information about this run;
my $runData;
{
    my $command = "$remotetool -listrun -remote_id $remote_id ";
    $command   .= " -dbname $dbname " if defined($dbname);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run remotetool to determine remote run status", $remote_id, $error_code);
    }

    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
    &my_die("Unable to run remotetool to determine remote run status", $remote_id, $error_code);

    my $metadata = parse_md_list($MDlist);
    $runData = $metadata->[0]; # There should be only one
}
my $stage = $runData->{stage};

# STEP 3: Poll the job status
my $poll_response = 0;
my $poll_word  = "nothing";

if ($skip_poll) {
    $poll_word  = "Completed";
} else {

    my $poll_file = "$remote_root/stask_logs/$runData->{stage}.$remote_id.out";
    ($poll_response,$poll_word) = poll_job_alt($poll_file);

    # ($poll_response,$poll_word) = poll_job($job_id);

    if ($poll_response == -1) { # This job has an error
        print "The job exited incorrectly: $remote_id $job_id $poll_word $poll_response.\n";
        # warn("The job exited incorrectly: $remote_id $job_id $poll_word.");
        &my_die("problem with MOAB processing?", $remote_id, $PS_EXIT_SYS_ERROR);
        exit (2);
    }

    if ($poll_response != 1) { # This job has NOT completed
        print "The job has not exited at LANL: $remote_id $job_id $poll_word $poll_response.\n";
        # NOTE: this should not be a failure condition
        exit (0);
    }
}

my $remote_path = uri_local_to_remote($path_base);

# STEP 4: transfer back the files.  Transfer tool should check the
# existence of files on the remote end and only return those that
# exist
unless ($skip_transfer) {
    my $offset = 0;  # Offset to start from.
    my $iter = 0;
    my $transfer_success = 1;
    do {
        $transfer_success = 1;
        $iter++;
        check_ssh_connection();
        my $return_push_output = ssh_exec_command("${remote_root}/sc_transfer_tool.pl --input $remote_path --offset $offset");
        my $return_push_output_print = join "\n", @{ $return_push_output };
        if ($return_push_output_print =~ /exited with value 255/) { $transfer_success = 0; }
        print "$return_push_output_print\n";
    } while (($transfer_success != 1) &&  ($iter < 10));  # Try harder;

    if (!$transfer_success) { &my_die ("failed to get all return files", $remote_id, 23); }
}
print STDERR "rsync of all files is complete, checking the results\n";

# STEP 5: grab the .stat file from LANL (this file is first constructed at LANL)

my $uri_stats    = $path_base . ".stat";
my $disk_stats   = $ipprc->file_resolve($uri_stats, 1);
my $remote_stats = uri_local_to_remote($uri_stats);

scp_get($remote_stats,$disk_stats);

# STEP 6: check dbinfo files for PASS / PART / FAIL

my %stage_pass = ();
my %stage_fail = ();
my %stage_part = ();

my $Npass = 0;
my $Npart = 0;
my $Nfail = 0;
my @dbinfo_files   = ();
open (STATFILE, "$disk_stats") || &my_die("Couldn't open file? $disk_stats", $remote_id, $PS_EXIT_SYS_ERROR);
while (my $line = <STATFILE>) {
    chomp $line;
#    unless ($line =~ /dbinfo/) { next; }

    my @words = split (" ", $line);

    my ($stage_id, $class_id, $component) = get_stage_id_from_filename ($words[0],$stage);
    unless(defined($stage_id)) {
        print STDERR "Unable to convert file: $words[0]\n";
        next;
    }
#    print STDERR "stage_id : $stage_id, class_id: $class_id, \n";

    if ($words[1] eq "PASS") {
        if ($line =~ /dbinfo/) {
            push @dbinfo_files, $words[0];
        }
        $stage_pass{$stage_id} = 1;
        $Npass ++;
    } elsif ($words[1] eq "FAIL") {
        $stage_fail{$stage_id} = 1;
        $Nfail ++;
    } elsif ($words[1] eq "PART") {
        $stage_part{$stage_id} = 1;
        $Npart ++;
    }
}
close (STATFILE);
print STDERR "dbinfo files: $Npass PASS, $Nfail FAIL, $Npart PART\n";

# STEP 7: if dbinfo is PASS, execute it (fail ones that are not PASS?).
if (1) {
  foreach my $file (@dbinfo_files) {
      my ($stage_id, $class_id) = get_stage_id_from_dbinfo ($file);
      if (defined($stage_part{$stage_id})||
          defined($stage_fail{$stage_id})) {
          print STDERR "Skipping $file as it is not complete: " . $stage_part{$stage_id} . " " . $stage_fail{$stage_id} . "\n";
          next;

      }
      open(DBF,"$file") || warn "Missing file $file\n";
      my $cmd = <DBF>;
      close(DBF);
      chomp($cmd);

      $cmd =~ s/-/ -/g; # This is just a safety check for missing space.

      if ($cmd eq "") {
          print STDERR "problem: empty dbinfo file\n";
          exit (1);
          next;
      }

      print "dbinfo: $file\n";

      if ($retry) {
          # use the dbinfo filename to get stage id, check if we have already updated this entry:
          my ($stage_id, $class_id) = get_stage_id_from_dbinfo ($file);

          # we just need to check for the existence of an entry:
          if ($runData->{stage} eq "chip") {
              my $cmd1 = "$chiptool -simple -processedimfile -chip_id $stage_id -class_id $class_id";
              $cmd1   .= " -dbname $dbname " if defined($dbname);

              my ( $success1, $error_code1, $full_buf1, $stdout_buf1, $stderr_buf1 ) =
                  run(command => $cmd1, verbose => 0);

              unless ($success1) {
                  $error_code1 = (($error_code1 >> 8) or $PS_EXIT_PROG_ERROR);
                  &my_die("Unable to run chiptool to check on results", $remote_id, $error_code1);
              }

              my $Nrows = @$stdout_buf1;
              if ($Nrows > 0) {
                  print STDERR "already updated: $stage_id, $class_id\n";
                  next;
              }
          } elsif ($runData->{stage} eq "camera") {
              my $cmd1 = "$camtool -simple -processedexp -cam_id $stage_id";
              $cmd1   .= " -dbname $dbname " if defined($dbname);

              my ( $success1, $error_code1, $full_buf1, $stdout_buf1, $stderr_buf1 ) =
                  run(command => $cmd1, verbose => 0);

              unless ($success1) {
                  $error_code1 = (($error_code1 >> 8) or $PS_EXIT_PROG_ERROR);
                  &my_die("Unable to run chiptool to check on results", $remote_id, $error_code1);
              }

              my $Nrows = @$stdout_buf1;
              if ($Nrows > 0) {
                  print STDERR "already updated: cam_id: $stage_id\n";
                  next;
              }
          } elsif ($runData->{stage} eq "warp") {
              my $cmd1 = "$warptool -simple -warped -warp_id $stage_id -skycell_id $class_id";
              $cmd1   .= " -dbname $dbname " if defined($dbname);

              my ( $success1, $error_code1, $full_buf1, $stdout_buf1, $stderr_buf1 ) =
                  run(command => $cmd1, verbose => 0);

              unless ($success1) {
                  $error_code1 = (($error_code1 >> 8) or $PS_EXIT_PROG_ERROR);
                  &my_die("Unable to run chiptool to check on results", $remote_id, $error_code1);
              }

              my $Nrows = @$stdout_buf1;
              if ($Nrows > 0) {
                  print STDERR "already updated: warp_id: $stage_id, $class_id\n";
                  next;
              }
          } else {
              print STDERR "do not know how to handle stage $runData->{stage}\n";
              exit (3);
          }
      }

      unless ($no_update) {
          my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
              run(command => $cmd, verbose => $verbose);
          unless ($success) {
              # This shouldn't fail, but we also can't suddenly abort if something does fail.
              # Now we're going to drop the component that owns this dbinfo, which should force a retry.
              $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
              warn("The command that shouldn't fail has failed: $file $cmd $remote_id $error_code");
              my ($stage_id, $class_id) = get_stage_id_from_dbinfo ($file);
              $stage_fail{$stage_id} = 1;
          }
      }
  }
  print STDERR "dbinfo files all run (or skipped)\n";
}

# STEP 8: unify the stage states and update the remoteComponent entries
foreach my $entry (keys (%stage_fail)) {
    $stage_pass{$entry} = 0;
}
foreach my $entry (keys (%stage_part)) {
    $stage_pass{$entry} = 0;
}
foreach my $entry (keys (%stage_pass)) {
    my $cmd = "remotetool -updatecomponent -remote_id $remote_id -stage_id $entry";
    $cmd   .= " -dbname $dbname " if defined($dbname);
    if ($stage_pass{$entry}) {
        $cmd .= " -set_state pass";
    } else {
        $cmd .= " -set_state fail";
    }

    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $cmd, verbose => $verbose);
        unless ($success) {
            &my_die("Unable to run remotetool to determine remote run status", $remote_id);
        }
    } else {
        print STDERR "no_update: $cmd\n";
    }
}

# We're done, so set the state and exit.
&my_die("Finished", $remote_id, 0, "full");

# END PROGRAM

sub check_ssh_connection {
    my $cmd = "$ssh -O check $DMZ_HOST";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Authorization check failed.",$remote_id,0,'auth');
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
        $cmd = "$scp $file ${DMZ_HOST}:${destination}";
    }

    my $directory = dirname($destination);
    ssh_exec_command("mkdir -p $directory");

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Failed to send file $file to LANL", $remote_id, $PS_EXIT_SYS_ERROR);
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
        $cmd = "$scp  ${DMZ_HOST}:${destination} $file";
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Failed to get file $file from LANL", $remote_id, $PS_EXIT_SYS_ERROR);
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
        &my_die("Failed to execute command: >>$cmd<<", $remote_id, $PS_EXIT_SYS_ERROR);
    }
    return ($stdout_buf);
}

sub poll_job_alt {
    my $poll_file = shift;

    my $response_array = ssh_exec_command("$remote_root/sc_checkjob.pl --poll_file $poll_file");
    my $response = join "\n", @$response_array;

    print STDERR "response: $response\n";

    if ($response =~ /STATUS: RUNNING/) {
        return(0,"Running");
    }
    elsif ($response =~ /STATUS: COMPLETED/) {
        return(1,"Completed");
    }
    elsif ($response =~ /STATUS: PENDING/) {
        return(-2,"Idle");
    }
    else {
        my $response_word = $response;
        $response_word =~ s/.*State: //;
        $response_word =~ s/\n.*//;
        return(-1,$response_word);
    }
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

sub uri_to_outputs {
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = uri_convert( $neb_uri );

#    print TRANSFER "$ipp_disk\n";
#    print CHECK    "$remote_disk\n";
    return($ipp_disk,$remote_disk);
}

sub uri_local_to_remote {
    # This needs to replace the nebulous tag with the remote root.
    my $local_uri = shift;
    $local_uri =~ s%^.*?/%%; # neb:/
    $local_uri =~ s%^.*?/%%; # /
    $local_uri =~ s%^.*?/%%; # @HOST@.0/
    my $remote_uri = "${remote_root}/" . $local_uri;

    return($remote_uri);
}

sub uri_remote_to_local {
    # This needs to replace the remote root directory with the nebulous tag.
    my $remote_uri = shift;
    $remote_uri =~ s%${remote_root}%%;
    my $local_uri  = "neb:///" . $remote_uri;

    return($local_uri);
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
        ($stage_id) = $file =~ /cm\.(\d*).dbinfo/;
        $class_id = 0;
    }
    elsif ($stage eq 'warp') {
        ($stage_id, $class_id) = $file =~ /wrp\.(\d*)\.(skycell.\d\d\d\d.\d\d\d).dbinfo/;
    }
    elsif ($stage eq 'stack') {
        print STDERR "dbinfo: $file\n";
        ($class_id, $stage_id) = $file =~ /(skycell.\d\d\d\d.\d\d\d).stk.(\d*).dbinfo/;
    }
    elsif ($stage eq 'staticsky') {
	($class_id, $stage_id) = $file =~ /(skycell.\d\d\d\d.\d\d\d).sky.(\d*).dbinfo/;
    }
    elsif ($stage eq 'diff') {
	($class_id,$stage_id) = $file =~ /(skycell.\d\d\d\d.\d\d\d).WS.dif.(\d*).dbinfo/;
    }
    elsif ($stage eq 'ff') {
	($class_id,$stage_id) = $file =~ /wrp\.(\d*).ff.(\d*).dbinfo/;
    }
    return ($stage_id, $class_id);
}

sub get_stage_id_from_filename {
    my $file = shift;
    my $stage = shift;
    my $stage_id = -1;
    my $class_id = '';
    my $component = '';

    if ($stage eq 'chip') {
        ($stage_id, $class_id, $component) = $file =~ /ch\.(\d*).(XY\d\d).([\w\.]+)$/;
    }
    elsif ($stage eq 'camera') {
        if ($file =~ /XY/) {
            ($stage_id, $class_id, $component) = $file =~ /cm\.(\d*).(XY\d\d)\.([\w\.]+)$/;
        }
        else {
            ($stage_id, $component) = $file =~ /cm\.(\d*)\.([\w\.]+)$/;
            $class_id = 0;
        }
    }
    elsif ($stage eq 'warp') {
        ($stage_id, $class_id, $component) = $file =~ /wrp\.(\d*)\.(skycell.\d\d\d\d.\d\d\d).([\w\.]+)$/;
    }
    elsif ($stage eq 'stack') {
        ($stage_id, $component) = $file =~ /stk\.(\d*)\.([\w\.]+)$/;
        $class_id = 0;
    }
    elsif ($stage eq 'staticsky') {
	($class_id, $stage_id, $component) = $file =~ /(skycell.\d\d\d\d.\d\d\d).sky.(\d*).([\w\.]+)$/;
    }
    elsif ($stage eq 'diff') {
	($class_id,$stage_id, $component) = $file =~ /(skycell.\d\d\d\d.\d\d\d).WS.dif.(\d*).([\w\.]+)$/;
    }
    elsif ($stage eq 'ff') {
	($class_id,$stage_id, $component) = $file =~ /wrp\.(\d*).ff.(\d*).([\w\.]+)$/;
    }

    return($stage_id,$class_id,$component);
}



sub fail_component {
    my $remote_id = shift;
    my $cmd = shift;
    my $file = shift;

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

    print STDERR "failed component: $remote_id, $stage_id\n";

    # Drop this from the database
    ## my $drop_cmd = "$remotetool -dropcomponent -remote_id $remote_id -stage_id $stage_id ";
    ## $drop_cmd   .= " -dbname $dbname " if defined($dbname);
    ## system($drop_cmd);
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

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);

    if (defined $id and not $no_update) {
        my $command = "$remotetool -updaterun -remote_id $id";
        $command .= " -fault $exit_code " if defined $exit_code;
        $command .= " -set_state $exit_state " if defined $exit_state;
        $command .= " -dbname $dbname " if defined $dbname;

        system($command);
    }

    exit($exit_code);
}

# Quick review:
# new -> pending -> run -> full
# auth
