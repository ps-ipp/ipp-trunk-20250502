#!/usr/bin/env perl

use warnings;
use strict;
use Carp;
use IPC::Cmd 0.36 qw( can_run run);
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use DateTime;

#
# Set up
################################################################################

my $missing_tools = 0;
my $regtool  = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
my $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $difftool = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $stacktool = can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
my $magicdstool = can_run('magicdstool') or (warn "Can't find magicdstool" and $missing_tools = 1);
my $laptool  = can_run('laptool') or (warn "Can't find laptool" and $missing_tools = 1);

my ( $help, $verbose, $debug, $do_nothing);
my ( $camera, $dbname);
my ( $lap_id );
my ( $queue_list );
my ( $chip_mode, $monitor_mode, $cleanup_mode);

#
# Global configuration constants that probably should be read from elsewhere.  
my $qstack_threshold     = 0.05; # Only make a quickstack if more than 5% of the exposures require it.
my $minimum_stack_inputs = 2;    # We can avoid magicking stack inputs if we have more than this number.
my $maximum_stack_inputs = 25;   # This is the place-holder guess on how big a stack we should process locally.  25 is the best guess.
my $do_updates_pv2       = 0;    # Do updates using the calls needed for PV2.  

GetOptions(
    'help|h'       => \$help,
    'verbose'      => \$verbose,
    'debug'        => \$debug,
    'do_nothing'   => \$do_nothing,

    'camera=s'     => \$camera,
    'dbname=s'     => \$dbname,
    
    'lap_id=s'     => \$lap_id,
    'queue_list=s' => \$queue_list,

    'chip_mode'    => \$chip_mode,
    'monitor_mode' => \$monitor_mode,
    'cleanup_mode' => \$cleanup_mode,
    ) or pod2usage ( 2 );
pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Choose a mode: --chip_mode --monitor_mode --cleanup_mode",
    -exitval => 3,
    ) unless
    defined $chip_mode or defined $monitor_mode or defined $cleanup_mode;
pod2usage(
    -msg => "--lap_id is required",
    -exitval => 3,
    ) unless
    defined $lap_id;

unless (defined $dbname) {
    $dbname = 'gpc1';
}

my $mdcParser = PS::IPP::Metadata::Config->new;

# Fetch all the information about this run
my %lapRunInfo = get_lapRun_info($lap_id);

# Run the appropriate mode

if (defined($chip_mode)) {
    unless ($lapRunInfo{state} eq 'new') {
	&my_die("Cannot run chip_mode if lapRun.state != new!",$lap_id);
    }
    my $status = chip_mode($lap_id);
    exit $status;
}
if (defined($monitor_mode)) {
    unless ($lapRunInfo{state} eq 'run') {
	&my_die("Cannot run monitor_mode if lapRun != run!", $lap_id);
    }
    my $status = monitor_mode($lap_id);
    exit $status;
}
if (defined($cleanup_mode)) {
    unless (($lapRunInfo{state} eq 'full')||
	    ($lapRunInfo{state} eq 'drop')) {
	&my_die("Cannot run cleanup_mode if lapRun != full!", $lap_id);
    }
    my $status = cleanup_mode($lap_id);
    exit $status;
}

#
# Chip
################################################################################

sub chip_mode {
    my $lap_id = shift;
    my $status = queue_chips($lap_id);

    if (!$status) { # This is the culprit.
	my $command = "$laptool -updaterun -lap_id $lap_id";
	$command .= " -dbname $dbname " if defined $dbname;
	$command .= " -set_state run ";
	my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform laptool -updaterun: $error_code", $lap_id);
	}
    }
    return($status);
}

# retrieve the chip_id for the exposure inserted
sub get_chip_id_from_metadata {
    my $exp_id = shift;
    my $label = shift;
    my $data_group = shift;
    # This is a puzzler... chiptool doesn't actually return a useful metadata.  We'll just scrape it from the database for now.

    my $command = "$chiptool -listrun -pstamp_order -exp_id $exp_id -label $label -data_group $data_group";
    $command .= " -dbname $dbname " if defined $dbname;

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform chiptool -listrun: $error_code", $exp_id, $data_group);
    }
    my $chips = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from chiptool -listrun", $exp_id, $data_group);
    # There should be only one.
    my $chip = ${ $chips }[0];
    my $chip_id = 0;
    if ($chip) {
	$chip_id = $chip->{chip_id};
    }
    
    return($chip_id);
}

sub get_lapRun_info {
    my $lap_id = shift;
    my $command = "$laptool -pendingrun -lap_id $lap_id";
    $command .= " -dbname $dbname " if defined $dbname;
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -pendingrun: $error_code", $lap_id);
    }
    my $Runs = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from laptool -pendingrun", $lap_id);
    # There should be only one.
    my $Run = ${ $Runs }[0];
    my %info = %{ $Run };
    return(%info);
}


# Queue a chipRun for this exposure with the appropriate parameters.
sub remake_this_exposure {
    my $exposure = shift;

    my @utctime = gmtime();
    $utctime[5] += 1900;
    $utctime[4] += 1;

    my $label = $exposure->{label};

    my $date = sprintf("%4d%02d%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir_date = sprintf("%4d/%02d/%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir = "neb://\@HOST\@.0/${dbname}/${label}/${workdir_date}";
    my $data_group = "${label}.${date}";


    my $chip_cmd = "$chiptool";
    $chip_cmd .= " -pretend " if defined $debug;
    $chip_cmd .= " -dbname $dbname " if defined $dbname;
    $chip_cmd .= " -definebyquery -exp_id $exposure->{exp_id} -set_end_stage warp -set_tess_id $exposure->{tess_id} ";
    $chip_cmd .= " -set_label $exposure->{label} -set_data_group $data_group ";
    $chip_cmd .= " -set_workdir $workdir -set_dist_group $exposure->{dist_group} ";
    $chip_cmd .= " -set_reduction LAP_SCIENCE ";
#    $chip_cmd .= " -set_reduction LAP_POLE ";

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $chip_cmd, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform chiptool -definebyquery: $error_code", $exposure->{lap_id}, $exposure->{proj_cell});
    }
    
    $exposure->{chip_id} = get_chip_id_from_metadata($exposure->{exp_id},$exposure->{label},$data_group); 
    $exposure->{data_state} = 'new';
    update_this_exposure($exposure);
    return($exposure);
}

sub remake_this_exposure_by_update {
    my $exposure = shift;

    my @utctime = gmtime();
    $utctime[5] += 1900;
    $utctime[4] += 1;

    my $label = $exposure->{label};

    my $date = sprintf("%4d%02d%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir_date = sprintf("%4d/%02d/%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir = "neb://\@HOST\@.0/${dbname}/${label}/${workdir_date}";
    my $data_group = "${label}.${date}";

    my $chiptool_info_cmd = "chiptool -listrun -exp_id $exposure->{exp_id} -chip_id $exposure->{chip_id} ";
    $chiptool_info_cmd   .= " -dbname $dbname " if defined $dbname;

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $chiptool_info_cmd, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform chiptool -listrun: $error_code", $exposure->{exp_id}, $data_group);
    }
    my $chips = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from chiptool -listrun", $exposure->{exp_id}, $data_group);
    # There should be only one.
    my $chip = ${ $chips }[0];
    my $chip_magicDS_id = 0;
    my $chip_cam_id = 0;
    if ($chip) {
	$chip_magicDS_id = $chip->{magic_ds_id};
	$chip_cam_id     = $chip->{cam_id};
    }
    if ($chip_magicDS_id == 0) {
#	return(&remake_this_exposure($exposure));
    }

    my $do_cam_update = 0;
    if ($chip_cam_id) {      # If there is no cam_id, we can't look for it.
	my $camtool_info_cmd = "camtool -processedexp -cam_id $chip_cam_id ";
	$camtool_info_cmd   .= " -dbname $dbname " if defined $dbname;
	($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $camtool_info_cmd, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform camtool -listrun: $error_code", $exposure->{exp_id}, $data_group);
	}
	my $cams = $mdcParser->parse_list(join "", @$stdout_buf) or
	    &my_die("Unable to parse metadata from camtool -listrun", $exposure->{exp_id}, $data_group);
	my $cam = ${ $cams }[0];
	# There can be only one.
	if ($cam) {
	    if ($cam->{label} ne $label) {
		$do_cam_update = 1;
	    }
	}
    }

    my $warptool_info_cmd = "warptool -listrun -exp_id $exposure->{exp_id} -chip_id $exposure->{chip_id} ";
    $warptool_info_cmd   .= " -dbname $dbname " if defined $dbname;
    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $warptool_info_cmd, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform warptool -listrun: $error_code", $exposure->{exp_id}, $data_group);
    }
    my $warps = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from warptool -listrun", $exposure->{exp_id}, $data_group);
    # There should be only one.
    my $warp = ${ $warps }[0];
    my $warp_id = 0;
    if ($warp) {
	$warp_id = $warp->{warp_id};
    }
    else {
	$exposure->{data_state} = 'drop';
	return($exposure);
    }	
    if ($warp_id == 0) {
	# Handle this correctly. Probably want to drop the exposure here.
	$exposure->{data_state} = 'drop';
	return($exposure);
    }

    if (($chip->{state} eq 'goto_cleaned')||
	($warp->{state} eq 'goto_cleaned')||
	($chip->{dsRun_state} eq 'goto_cleaned')) {
	$exposure->{data_state} = 'pending_update';
	&update_this_exposure($exposure);
	return($exposure);
    }

#
# This is where we launch updates.
#
    
    my $chiptool_update_cmd = "chiptool -setimfiletoupdate -chip_id $exposure->{chip_id} -set_label $label";
    if ($do_updates_pv2) {
	$chiptool_update_cmd   .= " -set_update_mode 1 "; # Use the current config information
    }
    $chiptool_update_cmd   .= " -dbname $dbname " if defined $dbname;
    my $camtool_update_cmd .= "camtool -updaterun -set_state update -cam_id $chip_cam_id -set_label $label";
    $camtool_update_cmd    .= " -dbname $dbname " if defined $dbname;
    my $magicDS_update_cmd  = "magicdstool -setfiletoupdate -magic_ds_id $chip_magicDS_id -set_label $label";
    $magicDS_update_cmd    .= " -dbname $dbname " if defined $dbname;
    my $warptool_preupdate_cmd = "warptool -revertwarped -warp_id $warp_id -fault 26";
    $warptool_preupdate_cmd .= " -dbname $dbname " if defined $dbname;
    my $warptool_update_cmd = "warptool -setskyfiletoupdate -warp_id $warp_id -set_label $label";
    $warptool_update_cmd   .= " -dbname $dbname " if defined $dbname;

    # only need to update the data if the warps are not full
    if ($warp->{state} ne 'full') { 
        if (($chip->{state} eq 'cleaned')||  # Exposure is cleaned
	    (($chip->{state} eq 'update')&&($chip->{label} ne $label))) {  # Exposure is owned by an interloper.
            ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $chiptool_update_cmd, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform chiptool -setimfiletoupdate: $error_code", $exposure->{exp_id}, $data_group);
            }
	    if ($do_updates_pv2) {     # Are we updating camera stages for PV2?
		if ($do_cam_update) {  # Is this camRun still set to the old label?
		    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
			run(command => $camtool_update_cmd, verbose => $verbose);
		    unless ($success) {
			$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
			&my_die("Unable to perform camtool -updaterun: $error_code", $exposure->{exp_id}, $data_group);
		    }
		}
	    }
#             if ($chip_magicDS_id != 0) {    
# 	        ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
# 	            run(command => $magicDS_update_cmd, verbose => $verbose);
# 	        unless ($success) {
# 	            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
# 	            &my_die("Unable to perform magicdstool -setfiletoupdate: $error_code", $exposure->{exp_id}, $data_group);
# 	        }
#             }
        }


        ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $warptool_preupdate_cmd, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform warptool --revertwarped: $error_code", $exposure->{exp_id}, $data_group);
        }

        ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $warptool_update_cmd, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform warptool -setskyfiletoupdate: $error_code", $exposure->{exp_id}, $data_group);
        }
    }
    $exposure->{active} = 1;

    return($exposure);
}
        
sub determine_if_can_update {
    my $exposure = shift;
    
    if (S64_IS_NOT_NULL($exposure->{warp_id})) {
        if ($exposure->{warpRun_state} eq 'full') {
            # this warp is in full state no need to update it or the chips
            return 0;
        }
    }
    if (S64_IS_NOT_NULL($exposure->{chip_id})) {
	if (($exposure->{chip_state} eq 'cleaned')||
	    ($exposure->{chip_state} eq 'update')||
	    ($exposure->{chip_state} eq 'goto_cleaned')||
	    ($exposure->{chip_state} eq 'error_cleaned')) {
	    return(1);
	}
    }

    return(0);
}
	

# This is the "user level" subroutine.
sub queue_chips {
    my $lap_id = shift;
    
    my $command = "$laptool -pendingexp -lap_id $lap_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -pendingexp: $error_code", $lap_id);
    }
    
    if (@$stdout_buf == 0) {
	# Nothing to do.
	return(0);
    }
    
    my $exposures = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from laptool -pendingexp", $lap_id);

    my $counter = 0;
    my %matching = ();
    my %indexing = ();
    # Determine which exposures need a chipRun queued.
    foreach my $exposure (@$exposures) {  
	# $lap_id = $exposure->{lap_id};  # This should be already known.
	my $tess_id = $exposure->{tess_id};
	my $exp_id = $exposure->{exp_id};
	my $chip_id = $exposure->{chip_id};
	my $pair_id = $exposure->{pair_id};
	my $private = $exposure->{private};
	my $pairwise = $exposure->{pairwise};
	my $active = $exposure->{active};
	my $data_state = $exposure->{data_state};
	my $dateobs = $exposure->{dateobs};
	my $object = $exposure->{object};
	my $comment = $exposure->{comment};

	my $updateable = determine_if_can_update($exposure);

	# This is a hack to fix old exposures that have no usable object/comment data.
	unless(defined($comment)) {
	    $comment = '';
	}
	if ((!defined($object))||($object eq 'NULL')||($object eq '')) {
	    if ($comment =~ /3pi_/) {
		$object = $comment;
		$object =~ s/^.*?(3pi_\d\d_\d\d\d\d).*?$/F1 $1/;
	    }
	    elsif ($comment =~ / ps1_/) {
		$object = $comment;
		$object =~ s/^.*?(ps1_\d\d_\d\d\d\d).*$/F2 $1/;
	    }
	    elsif ($comment =~ /ThreePi.*3pi_/) {
		$object = $comment;
		$object =~ s/^ThreePi . (\S+? 3pi_\d\d_\d\d\d\d) .*$/F3 $1/;
	    }           
	    elsif ($comment =~ /ThreePi.*ps1_/) {
		$object = $comment;
		$object =~ s/^ThreePi . (\S+? ps1_\d\d_\d\d\d\d) .*$/F4 $1/;
	    }           
	    elsif ($comment =~ /ThreePi /) {
		$object = $comment;
		$object =~ s/^ThreePi . (\S+? \d\d\d\d) .*$/F5 $1/;
	    }           
	    elsif (($comment =~ /focus/i)||
		   ($comment =~ /test/i)||
		   ($comment =~ /hyster/i)||
		   ($comment =~ /dither/i)||
		   ($comment =~ /camera/i)
		){
		# This is junk that shouldn't exist.
		$object = 'DROP';
		$exposure->{data_state} = 'drop';
		$exposure->{pairwise} = 0;
		$exposure->{pair_id} =  9223372036854775807;
		$exposure->{active} = 0;
		&update_this_exposure($exposure);
		$counter++; # To ensure everyone else is consistent
		next;
	    }
	}

	# Determine the current state of chipRuns for these exposures, and update/remake as needed.
	if (S64_IS_NOT_NULL($chip_id)) { # We already have a defined chip_id
	    if (($pairwise) && !($pair_id)) {
		# This is an error.
		&my_die("Exposure $exp_id for $lap_id is declared pairwise without a defined pair", $lap_id);
	    }
	    $exposure->{data_state} = 'exists';
	    if ($updateable) { # We know about this run, but the data needs to be regenerated.
		$exposure->{data_state} = 'update';
		$exposure = remake_this_exposure_by_update($exposure);
	    }
	    &update_this_exposure($exposure);
	    $matching{$object}{$comment} = $exp_id;
	    $indexing{$exp_id} = $counter;
	    $counter++; 
	}
	else { # We do not already have a chip_id.  
	    # Make a chipRun, and update the exposure.
	    $exposure->{data_state} = 'run';
	    $exposure = remake_this_exposure($exposure); 
	    # Save our information for diff pairing.
	    &update_this_exposure($exposure);
	    $matching{$object}{$comment} = $exp_id;
	    $indexing{$exp_id} = $counter;
	    $counter++;
	}

	if ($verbose) {
	    print "ZZ: $exp_id $object $comment $matching{$object}{$comment}\n";
	}
    }

    # Determine which exposures have a pair for diffing.
    foreach my $object (keys %matching) { 
	my @exp_ids_to_diff = ();
	foreach my $comment (keys %{ $matching{$object} }) {
	    push @exp_ids_to_diff, $matching{$object}{$comment};
	    print "$object $comment $matching{$object}{$comment} $indexing{$matching{$object}{$comment}} $exp_ids_to_diff[-1]\n";
	}
	@exp_ids_to_diff = sort { $indexing{$a} <=> $indexing{$b} } @exp_ids_to_diff; # This is effectively a sort by dateobs.
	
	if (( $#exp_ids_to_diff + 1) % 2 != 0) { # We have an odd number of exposures, even after comment filtering
	    pop(@exp_ids_to_diff); # dump the last entry.
	}

	while ($#exp_ids_to_diff > -1) {  # Save the pair information to the opposite exposure.
	    my $exp_id_A = shift(@exp_ids_to_diff);
	    my $exp_id_B = shift(@exp_ids_to_diff);

	    my $exp_A = ${ $exposures }[$indexing{$exp_id_A}];
	    my $exp_B = ${ $exposures }[$indexing{$exp_id_B}];
	    print "$exp_A $exp_B $exp_id_A $exp_id_B $indexing{$exp_id_A} $indexing{$exp_id_B}\n";
	    if ($exp_A->{diff_id} == $exp_B->{diff_id}) {
		$exp_A->{pairwise} = 1;
		$exp_A->{pair_id} = $exp_B->{chip_id};
		
		$exp_B->{pairwise} = 1;
		$exp_B->{pair_id} = $exp_A->{chip_id};
		if ($verbose) {
		    print "LAP_DIFFS: $object: $exp_A->{exp_id} and $exp_B->{exp_id} are a pair\n";
		}
	    }
	    else {
		$exp_A->{pairwise} = 0;
		$exp_B->{pairwise} = 0;
	    }
	}
    }

    # Scan all exposures, and ensure that pairwise and private are set correctly
    foreach my $exposure (@$exposures) { 
	if ($verbose) {
	    print "YY: $exposure\n";
	}
	if ($exposure->{pairwise} && !($exposure->{pair_id})) {
	    $exposure->{pairwise} = 0; # We marked it for pairwise diffs, but didn't match it. Probably an error.
	}
	
	update_this_exposure($exposure);
    }
    return(0);
}


#
# Decide Things
################################################################################
sub monitor_mode {
    my $lap_id = shift;
    return(check_status($lap_id));
}
    

sub check_stack_status {  # look at lapRun associated stacks, and confirm their states.
    my $lap_id = shift;

    my ($defined_qstack,$have_qstack,$defined_fstack,$have_fstack);
    
    my $command = "$laptool -stacks -lap_id $lap_id";
    $command .= " -dbname $dbname" if defined $dbname;

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -stacks: $error_code", $lap_id, "");
    }

    ($defined_qstack,$have_qstack,$defined_fstack,$have_fstack) = (0,0,0,0);
    if (@$stdout_buf == 0) {
	# Nothing to do.  Possibly an error, but for now, just accept it
	return($defined_qstack,$have_qstack,$defined_fstack,$have_fstack);
    }

    my $stacks = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from laptool -stacks", $lap_id, "");

    my $total_qstacks = 0;
    my $complete_qstacks = 0;
    my $total_fstacks = 0;
    my $complete_fstacks = 0;
    foreach my $stack (@$stacks) {
	if (($stack->{quick_stack_id})&&
	    (S64_IS_NOT_NULL($stack->{quick_stack_id}))) {
	    $defined_qstack = 1;
	    $total_qstacks++;
	}
	if (($stack->{final_stack_id})&&
	    (S64_IS_NOT_NULL($stack->{final_stack_id}))) {
	    $defined_fstack = 1;
	    $total_fstacks++;
	}
	
	if (($stack->{quick_state})&&
	    ($stack->{quick_state} eq 'full')) {
	    $complete_qstacks++;
	}
	elsif (($stack->{quick_state})&&($stack->{quick_state} eq 'new')&&
	       ($stack->{quick_fault} >= 4)&&($stack->{quick_fault} != 32767)) {
	    printf STDERR "Faulted quick stack: $stack->{quick_stack_id} $stack->{quick_fault}\n";
	    $complete_qstacks++;# This is not the best solution, but if they continually fail, there's not much we can do.
	}
	if (($stack->{final_state})&&
	    ($stack->{final_state} eq 'full')) {
	    $complete_fstacks++;
	}
	elsif (($stack->{final_state})&&
	       ($stack->{final_state} eq 'drop')) {
	    $complete_fstacks++;
	}
	elsif (($stack->{final_state})&&
	       ($stack->{final_state} eq 'wait')) {
	    $complete_fstacks++; # Not the best either, but we've noted an issue, and tried to move on.  LAP should respect this.
	}
	elsif (($stack->{final_state})&&($stack->{final_state} eq 'new')&&
	       ($stack->{final_fault} >= 4)&&($stack->{final_fault} != 32767)) {
	    printf STDERR "Faulted final stack: $stack->{final_stack_id} $stack->{final_fault}\n";
	    $complete_fstacks++; # This is not the best solution, but if they continually fail, there's not much we can do.
	}

    }
    if (($complete_qstacks > 0)&&
	($complete_qstacks == $total_qstacks)) {
	$have_qstack = 1;
    }
    if (($complete_fstacks > 0)&&
	($complete_fstacks == $total_fstacks)) {
	$have_fstack = 1;
    }
    
	
    return($defined_qstack,$have_qstack,$defined_fstack,$have_fstack);
}
sub check_status {
    my $lap_id = shift;
    
    my ($defined_qstack,$have_qstack,$defined_fstack,$have_fstack) = check_stack_status($lap_id);

    my $command = "$laptool -exposures -lap_id $lap_id";
    $command .= " -dbname $dbname" if defined $dbname;

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -exposures: $error_code", $lap_id);
    }
    
    if (@$stdout_buf == 0) {
	# Nothing to do. However, this is likely an error.
	return(0);
    }
    
    my $exposures = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from laptool -exposures", $lap_id);

    # Need to prescan to see who matches whom.
    my %match_hash = ();
    my $index = 0;
    foreach my $exposure (@$exposures) {
	if ($exposure->{pair_id}) {
	    $match_hash{$exposure->{pair_id}} = $index; # Tell the companion who we are, so they can find us next time.
	}
	$index++;
    }

    # Things I want to know before I'm through
#     my $needs_qstack = 0;
    my $needs_something_remade = 0;  # I never need something remade, because we're committed to one-exp=one-chip
    my $needs_something_private = 0; # I never need something private, as that is calculated by the difference engine.
#     my $can_qstack = 0;
     my $have_diff = 0;
     my $can_diff = 0;
#     my $can_fstack = 0;
    my $total_exposures = 0;
    my @lonely_exposures = ();

    my $are_warped = 0;
    my $are_magicked = 0;
    foreach my $exposure (@$exposures) {
	$total_exposures++;
	my $companion;

	if ($exposure->{pairwise}) {
            # Load companion exposure information
	    if (($exposure->{pair_id})&&(exists($match_hash{$exposure->{chip_id}}))) {
		$companion = ${ $exposures }[$match_hash{$exposure->{chip_id}}]; # Match!
	    }
	    else { # We claimed to be pairwise, but do not have a valid pair_id.
		$exposure->{pairwise} = 0;
		&update_this_exposure($exposure);
	    }
	}

	if  ($exposure->{data_state} eq 'drop') { # This exposure is impossible, so fudge the counts so we get through.
	    $are_warped++;
	    $can_diff ++;
	    $have_diff ++;
	    $are_magicked ++;
	    next;
	}

	if ($exposure->{data_state} eq 'pending_update') {
	    $exposure->{data_state} = 'update';
	    $exposure = &remake_this_exposure_by_update($exposure);
	    &update_this_exposure($exposure);
	    next;
	}
	
	if ($exposure->{private}) { # I've declared this exposure private to this lapRun.
	    push @lonely_exposures, $exposure;
	}
	
# 	if ($exposure->{needs_remade}) { # This does the check that private = false for other lapRun
# 	    $needs_something_remade = 1;
# 	    $exposure = remake_this_exposure($exposure);
# 	}
	# Do quality checks here
	if ($verbose) {
	    printf("TEST: %d %d %d\n",$exposure->{exp_id},$exposure->{chip_id},$exposure->{chip_component_count});
	}
	# Checks to ensure we do not divide by zero.  Mostly happens with old diffs, it seems.
	if ($exposure->{chip_component_count} == 0) {
	    $exposure->{chip_component_count} = 1;
	}
	if ($exposure->{cam_component_count} == 0) {
	    $exposure->{cam_component_count} = 1;
	}
	if ($exposure->{warp_component_count} == 0) {
	    $exposure->{warp_component_count} = 1;
	}
	if ($exposure->{diff_component_count} == 0) {
	    $exposure->{diff_component_count} = 1;
	}

	my $is_bad_quality = 0;
	if ((defined($exposure->{chipRun_state}))&&($exposure->{chipRun_state} eq 'full')&&
	    ($exposure->{chip_component_count} > 0)&&($exposure->{chip_bad_quality} / $exposure->{chip_component_count} > 0.05)) {
	    printf("QUALITY: $exposure->{exp_id} has bad chip quality: %d / %d\n",
		   $exposure->{chip_bad_quality} , $exposure->{chip_component_count});
	    $is_bad_quality = 1;
	}
	elsif ((defined($exposure->{camRun_state}))&&($exposure->{camRun_state} eq 'full')&&
	       ($exposure->{cam_bad_quality} / $exposure->{cam_component_count} > 0)) {
	    printf("QUALITY: $exposure->{exp_id} has bad cam quality: %d / %d\n",
		   $exposure->{cam_bad_quality} , $exposure->{cam_component_count});
	    $is_bad_quality = 1;
	}
	elsif ((defined($exposure->{warpRun_state}))&&($exposure->{warpRun_state} eq 'full')&&
	       ($exposure->{warp_bad_quality} / $exposure->{warp_component_count} > 0.3)) {
	    printf("QUALITY: $exposure->{exp_id} has bad warp quality: %d / %d\n",
		   $exposure->{warp_bad_quality} , $exposure->{warp_component_count});
	    $is_bad_quality = 1;
	}
	elsif ((defined($exposure->{diffRun_state}))&&($exposure->{diffRun_state} eq 'full')&&
	       ($exposure->{diff_bad_quality} / $exposure->{diff_component_count} > 0.5)) {
	    printf("QUALITY: $exposure->{exp_id} has bad diff quality: %d / %d\n",
		    $exposure->{diff_bad_quality} , $exposure->{diff_component_count});
	    $is_bad_quality = 1;
	}
	# If we've detected a bad quality exposure, drop it, and tell the companion.
	if ($is_bad_quality) {
	    unless ((defined($exposure->{diffRun_state}))&&
		    ($exposure->{diffRun_state} eq 'full')) {
		$are_warped++;
		if ($companion) {
		    $companion->{pairwise} = 0;
		    &update_this_exposure($companion);
#		    push @lonely_exposures, $companion;
		}
		$exposure->{pairwise} = 0;
	    }
	    $exposure->{data_state} = 'drop';
	    &update_this_exposure($exposure);
	    next;
	}
	

	if (($exposure->{warpRun_state})&&  # This exposure has a warp
	    ($exposure->{warpRun_state} eq 'full')) { # This exposure's warp is done.
	    $are_warped++;
#	    $can_qstack ++;
	    $can_diff ++;
	    ## BEGIN These two disable LAP stacks
	    $are_magicked = $total_exposures;
	    $have_diff ++;
	    ## END
#	    $exposure->{data_state} = 'to_diff';
	    $exposure->{data_state} = 'full';
	}
	if (($exposure->{diff_id})&&
	    (&S64_IS_NOT_NULL($exposure->{diff_id}))&&
	    (($exposure->{diffRun_state} eq 'full')||
	     ($exposure->{diffRun_state} eq 'error_cleaned')||
	     ($exposure->{diffRun_state} eq 'cleaned'))
	    ) {
	    $are_magicked++;
	    $have_diff ++;
#	    $exposure->{data_state} = 'to_magic';
	    $exposure->{data_state} = 'full';
	}
	# We no longer need to care about magic
# 	if (($exposure->{magicked})&&
# 	    ($exposure->{warpRun_state})&&
# 	    ($exposure->{warpRun_state} eq 'full')&&
# 	    (&S64_IS_NOT_NULL($exposure->{magicked}))) { # This exposure has been magicked, so it is through with diff.
# 	    $are_magicked++;
# 	    $exposure->{data_state} = 'full';
# #	    $can_fstack ++;
# 	}
	unless ($debug) {
	    &update_this_exposure($exposure);
	}
    }

    print "STATUS: LAP_ID           $lap_id\n";
    print "STATUS: DEFINED_QSTACK:  $defined_qstack\n";
    print "STATUS: HAVE_QSTACK:     $have_qstack\n";
    print "STATUS: DEFINED_FSTACK:  $defined_fstack\n";
    print "STATUS: HAVE_FSTACK:     $have_fstack\n";

    print "STATUS: NEEDS_REMADE:    $needs_something_remade\n";
    print "STATUS: NEEDS PRIVATIZE: $needs_something_private\n";
    print "STATUS: ARE WARPED:      $are_warped\n";
    print "STATUS: ARE MAGICKED:    $are_magicked\n";
    print "STATUS: CAN_DIFF:        $can_diff\n";
    print "STATUS: HAVE_DIFF:       $have_diff\n";

    print "STATUS: TOTAL_EXPOSURES: $total_exposures\n";
    print "\n";
    if ($do_nothing) {
	exit(0);
    }

    # Do the decision making based on what we've learned
    if ($are_warped == $total_exposures) { # Are all the warps done?
	if ($defined_fstack == 0) {        # We have not yet made any stacks.
	    print "STATUS: We can attempt to queue deep final stacks.\n";
	    my $Nstacks_made;
	    unless ($debug) {
		$Nstacks_made = &queue_muggle_finalstack($exposures);
	    }
	    print "STATUS: Made $Nstacks_made final stacks.\n";
	    exit(0);
	    # If this doesn't do anything, we may end up stuck here.  Therefore, we need a more robust test.
	}
	else { # We have made a stack before for this lapRun.
	    if ($have_fstack == $defined_fstack) { # We have made all the stacks we have asked for (so far)
		if ($are_magicked < $total_exposures) { # But we have not yet made diffs
#		if ($have_diff < $total_exposures) { # We have not yet made diffs
		    if (($can_diff == $total_exposures)&& # And we have exposures that should be diffed.
			($have_diff < $can_diff)) {       # and we haven't made them all yet, either.
			print "STATUS: Dropping unpaired exposures and making pairwise diffs.\n";
# 			foreach my $exposure (@lonely_exposures) {
# 			    $exposure->{data_state} = 'drop';
# 			    &update_this_exposure($exposure);
# 			}
			unless ($debug) {
			    &queue_diffs($exposures);
			}
			exit(0);
		    }
		    else {
			# This is apparently the case where we've queued diffs to run, but they have not yet
			# completed though the destreak phase.
			print "STATUS: Waiting for diffs/magic/destreak to complete.\n";
			exit(0);
		    }
		}
		elsif ($are_magicked >= $total_exposures) { # We have made all of the diffs, and propagated through magic
		    print "STATUS: We would create all remaining stacks possible, but no longer require magic.\n";
# 		    my $Nstacks_made = -1;
# 		    unless ($debug) {
# 			$Nstacks_made = &queue_muggle_finalstack($exposures);
# 		    }
# 		    if ($Nstacks_made == 0) { # We have made all the stacks that we will ever make.
			print "STATUS: No more stacks generated. Deactivating exposures for this complete lapRun.\n";
			unless ($debug) {
			    &deactivate_exposures($exposures);
			
			    $command = "$laptool -updaterun -lap_id $lap_id";
			    $command .= " -dbname $dbname " if defined $dbname;
			    $command .= " -set_state full ";
			    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
				run(command => $command, verbose => $verbose);
			    unless ($success) {
				$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
				&my_die("Unable to perform laptool -updaterun: $error_code", $lap_id);
			    }
# 			}
			exit(0);
		    }
# 		    else { 
# 			print "STATUS: $Nstacks_made stacks made.\n";
# 			exit(0);
# 		    }
		}
		else { # What?
		    print "STATUS: More exposures magicked than exist. This is a mistake.\n";
		    exit(0);
		}
	    }
	    else { # We have not yet made all the stacks we've asked for.
		print "STATUS: Defined $defined_fstack, completed $have_fstack. Waiting for stacking to finish.\n";
		exit(0);
	    }
	}
    }
    else { # Not all warps finished.
	# do nothing
	print "STATUS: Not all warps are finished, waiting for that to finish.\n";
	exit(0);
    }

    print "\n";
    return(0);
}

sub queue_quickstack {
    my $exposures = shift; # reference to exposure array;
    my $exposure = ${ $exposures }[0]; # reference to the first exposure to get run level information
    
    my $lap_id    = $exposure->{lap_id};
    my $label     = $exposure->{label};
    my $filter    = $exposure->{filter};
    my $proj_cell = $exposure->{projection_cell};

    unless (defined($label) && defined($filter) && defined($proj_cell)) {
	&my_die("Unable to perform stacktool. Insufficient information.", $lap_id);
    }

    my $warps = '';
    foreach $exposure (@$exposures) {
	if (($exposure->{data_state} ne 'drop')&&
	    (S64_IS_NOT_NULL($exposure->{warp_id}))) {
	    $warps .= " -warp_id $exposure->{warp_id} ";
	}
    }

    my @utctime = gmtime();
    $utctime[5] += 1900;
    $utctime[4] += 1;
    my $date = sprintf("%4d%02d%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir_date = sprintf("%4d/%02d/%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir = "neb://\@HOST\@.0/${dbname}/${label}/${workdir_date}";
    my $data_group = "${label}.${proj_cell}.quick.${lap_id}";

    my $command = "$stacktool ";
    $command .= " -pretend " if defined $debug;
    $command .= " -dbname $dbname " if defined $dbname;
    $command .= " -definebyquery -select_skycell_id ${proj_cell}.% -select_filter $filter ";
#    $command .= " -select_label $label "; # Removed to allow exposure sharing to work
    $command .= " -set_label ${label} -set_data_group $data_group ";
    $command .= "  -set_workdir $workdir  -set_dist_group NODIST ";
    $command .= " -min_num 2 -set_reduction QUICKSTACK ";
    $command .= " $warps ";

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -definebyquery: $error_code", $lap_id);
    }
    my $stacks_made = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("unable to parse metadata from stacktool -definebyquery", $lap_id, "");

    $command = "$stacktool ";
    $command .= " -dbname $dbname " if defined $dbname;
    $command .= " -sassskyfile -data_group $data_group ";
    $command .= " -filter $filter -projection_cell ${proj_cell} ";

    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -sassskyfile: $error_code", $lap_id);
    }
    
    my $stacks = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stacktool -sassskyfile", $lap_id, "");
    
    my $stack = ${ $stacks }[0];
    my $sass_id = $stack->{sass_id};
    unless (defined($sass_id)) {
	&my_die("Unable to parse metadata from stacktool for sass_id", $lap_id, "");
    }

    print "QUICK_SASS_ID: $sass_id\n";
    $command = "$laptool -updaterun -lap_id $lap_id -set_quick_sass_id $sass_id";
    $command .= " -dbname $dbname " if defined $dbname;

    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -updaterun: $error_code", $lap_id);
    }
    
    return($#{ $stacks_made } + 1);
}
sub queue_muggle_finalstack {
    my $exposures = shift; # reference to exposure array;
    my $exposure = ${ $exposures }[0]; # reference to the first exposure to get run level information
    
    my $lap_id    = $exposure->{lap_id};
    my $label     = $exposure->{label};
    my $filter    = $exposure->{filter};
    my $proj_cell = $exposure->{projection_cell};

    unless (defined($label) && defined($filter) && defined($proj_cell)) {
	&my_die("Unable to perform stacktool. Insufficient information.", $lap_id);
    }

    my $warps = '';
    foreach $exposure (@$exposures) {
	if (($exposure->{data_state} ne 'drop')&&
#	    (S64_IS_NOT_NULL($exposure->{magicked}))&&
	    (S64_IS_NOT_NULL($exposure->{warp_id}))) {
	    $warps .= " -warp_id $exposure->{warp_id} ";
	}
    }

    my @utctime = gmtime();
    $utctime[5] += 1900;
    $utctime[4] += 1;
    my $date = sprintf("%4d%02d%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir_date = sprintf("%4d/%02d/%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir = "neb://\@HOST\@.0/${dbname}/${label}/${workdir_date}";
    my $data_group = "${label}.${proj_cell}.final.${lap_id}";


    ##
    ## Queue the "shallow" stacks that will run at los alamos.
    my $command = "$stacktool ";
    $command .= " -pretend " if defined $debug;
    $command .= " -dbname $dbname " if defined $dbname;
    $command .= " -definebyquery -select_skycell_id ${proj_cell}.% -select_filter $filter ";
#    $command .= " -select_label $label "; # Removed to allow exposure sharing to work
    $command .= " -set_label ${label} -set_workdir $workdir -set_data_group $data_group ";
    $command .= " -min_num ${minimum_stack_inputs} -max_num ${maximum_stack_inputs} -set_reduction THREEPI_STACK_1DG -set_dist_group $exposure->{dist_group} ";
    $command .= " $warps ";

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -definebyquery: $error_code :: $command", $lap_id);
    }
    my $stacks_made = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("unable to parse metadata from stacktool -definebyquery", $lap_id, "");    

    ##
    ## Queue the "deep" stacks that will run locally.
    my $deep_minimum = $maximum_stack_inputs + 1;
    if ($deep_minimum < 2) { $deep_minimum = 2; }
    $command = "$stacktool ";
    $command .= " -pretend " if defined $debug;
    $command .= " -dbname $dbname " if defined $dbname;
    $command .= " -definebyquery -select_skycell_id ${proj_cell}.% -select_filter $filter ";
#    $command .= " -select_label $label "; # Removed to allow exposure sharing to work
    $command .= " -set_label ${label}.local -set_workdir $workdir -set_data_group $data_group ";
    $command .= " -min_num ${deep_minimum} -set_reduction THREEPI_STACK_1DG -set_dist_group $exposure->{dist_group} ";
    $command .= " $warps ";

    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -definebyquery: $error_code :: $command", $lap_id);
    }
    my $stacks_made_deep = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("unable to parse metadata from stacktool -definebyquery", $lap_id, "");    

    # Append deep stacks to stacks so we can get a complete count later.
    push @{ $stacks_made }, @{ $stacks_made_deep };
    
    ## Determine the sass_id so we can record it in the database.
    $command = "$stacktool ";
    $command .= " -dbname $dbname " if defined $dbname;
    $command .= " -sassskyfile -data_group $data_group ";
    $command .= " -filter $filter -projection_cell ${proj_cell} ";

    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -sassskyfile: $error_code", $lap_id);
    }

    my $stacks = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stacktool -sassskyfile", $lap_id, "");

    my $stack = ${ $stacks }[0];
    my $sass_id = $stack->{sass_id};
    unless (defined($sass_id)) {
	&my_die("Unable to parse metadata from stacktool for sass_id", $lap_id, "");
    }
    print "FINAL_SASS_ID: $sass_id\n";
    $command = "$laptool -updaterun -lap_id $lap_id -set_final_sass_id $sass_id";
    $command .= " -dbname $dbname " if defined $dbname;

    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -updaterun: $error_code", $lap_id);
    }

    return($#{ $stacks_made } + 1);
}
sub queue_magic_finalstack {
    my $exposures = shift; # reference to exposure array;
    my $exposure = ${ $exposures }[0]; # reference to the first exposure to get run level information
    
    my $lap_id    = $exposure->{lap_id};
    my $label     = $exposure->{label};
    my $filter    = $exposure->{filter};
    my $proj_cell = $exposure->{projection_cell};

    unless (defined($label) && defined($filter) && defined($proj_cell)) {
	&my_die("Unable to perform stacktool. Insufficient information.", $lap_id);
    }

    my $warps = '';
    foreach $exposure (@$exposures) {
	if (($exposure->{data_state} ne 'drop')&&
#	    (S64_IS_NOT_NULL($exposure->{magicked}))&&
	    (S64_IS_NOT_NULL($exposure->{warp_id}))) {
	    $warps .= " -warp_id $exposure->{warp_id} ";
	}
    }

    my @utctime = gmtime();
    $utctime[5] += 1900;
    $utctime[4] += 1;
    my $date = sprintf("%4d%02d%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir_date = sprintf("%4d/%02d/%02d",$utctime[5],$utctime[4],$utctime[3]);
    my $workdir = "neb://\@HOST\@.0/${dbname}/${label}/${workdir_date}";
    my $data_group = "${label}.${proj_cell}.final.${lap_id}";

    my $command = "$stacktool ";
    $command .= " -pretend " if defined $debug;
    $command .= " -dbname $dbname " if defined $dbname;
    $command .= " -definebyquery -select_skycell_id ${proj_cell}.% -select_filter $filter ";
#    $command .= " -select_label $label "; # Removed to allow exposure sharing to work
    $command .= " -set_label ${label} -set_workdir $workdir -set_data_group $data_group ";
    $command .= " -min_num 2 -min_new 2 -set_reduction THREEPI_STACK_1DG -set_dist_group $exposure->{dist_group} ";
    $command .= " $warps ";

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -definebyquery: $error_code", $lap_id);
    }
    my $stacks_made = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("unable to parse metadata from stacktool -definebyquery", $lap_id, "");    
    
    $command = "$stacktool ";
    $command .= " -dbname $dbname " if defined $dbname;
    $command .= " -sassskyfile -data_group $data_group ";
    $command .= " -filter $filter -projection_cell ${proj_cell} ";

    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -sassskyfile: $error_code", $lap_id);
    }

    my $stacks = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stacktool -sassskyfile", $lap_id, "");

    my $stack = ${ $stacks }[0];
    my $sass_id = $stack->{sass_id};
    unless (defined($sass_id)) {
	&my_die("Unable to parse metadata from stacktool for sass_id", $lap_id, "");
    }
    print "FINAL_SASS_ID: $sass_id\n";
    $command = "$laptool -updaterun -lap_id $lap_id -set_final_sass_id $sass_id";
    $command .= " -dbname $dbname " if defined $dbname;

    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -updaterun: $error_code", $lap_id);
    }
    return($#{ $stacks_made } + 1);
}
sub queue_diffs {
    my $exposures = shift;
    
    # Need to prescan to see who matches whom.
    my %match_hash = ();
    my $index = 0;
    foreach my $exposure (@$exposures) {
	if ($exposure->{pair_id}) {
	    $match_hash{$exposure->{pair_id}} = $index; # Tell the companion who we are, so they can find us next time.
	}
	$index++;
    }

    my %already_queued = ();
    foreach my $exposure (@$exposures) {
	if ($exposure->{data_state} eq 'drop') { # Kick out unusable exposures
	    next;
	}
	if ($exposure->{diff_id}&&S64_IS_NOT_NULL($exposure->{diff_id})) { # This happens when we inherit a complete exposure.
	    next;
	}
	if ($already_queued{$exposure->{warp_id}}) {
	    print "STATUS: Have already queued a diff containing $exposure->{exp_id} $exposure->{chip_id} $exposure->{warp_id}\n";
	    next;
	}

	my @utctime = gmtime();
	$utctime[5] += 1900;
	$utctime[4] += 1;

	my $label = $exposure->{label};
	my $dist_group = $exposure->{dist_group};
	my $date = sprintf("%4d%02d%02d",$utctime[5],$utctime[4],$utctime[3]);
	my $workdir_date = sprintf("%4d/%02d/%02d",$utctime[5],$utctime[4],$utctime[3]);
	my $workdir = "neb://\@HOST\@.0/${dbname}/${label}/${workdir_date}";
	my $data_group = "${label}.${date}";


	my $command = "$difftool ";
	$command .= " -pretend " if defined $debug;
	$command .= " -dbname $dbname " if defined $dbname;
	$command .= " -set_label $label -set_workdir $workdir -set_data_group $data_group ";
	if ($exposure->{dist_group}) {
	    $command .= " -set_dist_group $exposure->{dist_group} ";
	}
	my $retry_command;
	if (($exposure->{pairwise})&&(defined(${ $exposures }[$match_hash{$exposure->{chip_id}}]))) { # warpwarp
	    my $companion = ${ $exposures }[$match_hash{$exposure->{chip_id}}];
	    $command .= " -definewarpwarp ";
#	    $command .= "-input_label $label -template_label $label ";
	    $command .= "-warp_id $exposure->{warp_id} -template_warp_id $companion->{warp_id} ";
	    $retry_command = $command;
	    $command .= " -backwards "; # This usually works.
	    $already_queued{$exposure->{warp_id}} = 1;
	    $already_queued{$companion->{warp_id}} = 1;
	    
	}
	else { # warp-qstack
	    # We need to decide if we can make a warp-stack diff:
	    if (&can_warp_stack_diff_be_made($exposure)) {
#	    next; 
#	    $command .= '-pretend';
		$command .= " -definewarpstack -available -good_frac 0.2 ";
		$command .= " -warp_id $exposure->{warp_id} -stack_label ${label} ";
		$already_queued{$exposure->{warp_id}} = 1;
		$exposure->{private} = 0;
		&update_this_exposure($exposure);
	    }
	    else {
		$exposure->{private} = 1;
		$exposure->{data_state} = 'drop';
		$already_queued{$exposure->{warp_id}} = 1;
		&update_this_exposure($exposure);
		next;
	    }
	}
	
	my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to perform difftool -definewarp(warp|stack): $error_code", $exposure->{lap_id}, $exposure->{proj_cell});
	}
	
	my $diffs = $mdcParser->parse_list(join "", @$stdout_buf) or
	    &my_die("Unable to parse metadata from difftool -definewarp(warp|stack)", $lap_id, "");
	
	my $diff = ${ $diffs }[0];
	my $diff_id = $diff->{diff_id};
	unless (defined($diff_id)) {
	    if ($retry_command) {
		($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		    run(command => $retry_command, verbose => $verbose);
		unless ($success) {
		    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		    &my_die("unable to perform difftool -definewarp(warp|stack): $error_code", $exposure->{lap_id}, $exposure->{proj_cell});
		}
		
		$diffs = $mdcParser->parse_list(join "", @$stdout_buf) or
		    &my_die("Unable to parse metadata from difftool -definewarp(warp|stack)", $lap_id, "");
		
		$diff = ${ $diffs }[0];
		$diff_id = $diff->{diff_id};
	    }
	    unless (defined($diff_id)) {
		$exposure->{data_state} = 'drop';
		&update_this_exposure($exposure);
	    }
	}
    }
}

sub can_warp_stack_diff_be_made {
    my $exposure = shift;
    my $lap_id   = $exposure->{lap_id};
    my $warp_id  = $exposure->{warp_id};

    my $command = "$laptool -diffcheck -lap_id $lap_id -warp_id $warp_id";
    $command .= " -dbname $dbname " if defined $dbname;

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -diffcheck: $error_code", $lap_id, $warp_id);
    }
    if (@$stdout_buf == 0) {
	return(0);
    }
    my $skycells = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from laptool -diffcheck", $lap_id, $warp_id);
    foreach my $skycell (@$skycells) {
	unless (defined($skycell->{stack_state})) {
	    return(0);
	}
	if ($skycell->{stack_state} ne 'full') {
	    return(0);
	}
	if ($skycell->{warp_state} ne 'full') {
	    return(0);
	}
	unless (S64_IS_NOT_NULL($skycell->{stack_id})) {
	    return(0);
	}
    }
    return(1);
}


# Deactivate all exposures in this run.
sub deactivate_exposures {
    my $exposures = shift;
    foreach my $exposure (@$exposures) {
	if ($exposure->{private} == 1) {  # This can probably be relaxed since we can update an undestreaked warp.
	    $exposure->{active} = 1;
	}
	else {
	    $exposure->{active} = 0;
	}
	update_this_exposure($exposure);	
    }
}


#
# Cleanup
################################################################################

sub cleanup_mode {
    my $lap_id = shift;

    if (defined($queue_list)) {
	my $successful = 1;
	open(Q,$queue_list) or ($successful = 0);
	if ($successful == 1) {
	    while(<Q>) {
		chomp;
		my $cmd = $_;
		my ($projection_cell,$seq_id,$filter,$label);
		$projection_cell = $cmd;
		$projection_cell =~ s/.*?-projection_cell (skycell.\w+?) .*/$1/;
		$seq_id = $cmd;
		$seq_id =~ s/.*-seq_id (\d+?) .*/$1/;
		$filter = $cmd;
		$filter =~ s/.*-filter (\w\.00000) .*/$1/;
		if ($filter =~ /\s+?/) {
		    $filter =~ s/.*-filter (\w\.00000)$/$1/;
		}
		$label = $cmd;
		$label =~ s/.*-label (.+?) .*/$1/;
		
		my $response;
		chomp($response = 
		      `laptool -dbname gpc1 -listrun -projection_cell $projection_cell -seq_id $seq_id -filter $filter -label $label -simple`);
		my $state = (split /\s+/, $response)[5];
		
		unless (defined($state)) {
		    if ($verbose) {
			print "Queuing: $cmd\n";
		    }
		    system($cmd);
		    
		    my $i = 0;
		    do {
			sleep(5);
			chomp($response = 
			      `laptool -dbname gpc1 -listrun -projection_cell $projection_cell -seq_id $seq_id -filter $filter -label $label -simple`);
			$state = (split /\s+/, $response)[5];
			$i++;
			unless(defined($state)) {
			    $successful = 0;
			}
		    } while (($state ne 'run')&&($i < 20)&&($successful));
		    last;
		}
	    }
	    close(Q);
	}
    }

    my $command = "$laptool -inactiveexp -lap_id $lap_id ";
    $command .= " -dbname $dbname " if defined $dbname;

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -inactiveexp: $error_code", "none", "none");
    }
    if (@$stdout_buf == 0) {
	# Nothing to do.
	$command = "$laptool -updaterun -lap_id $lap_id";
	$command .= " -dbname $dbname " if defined $dbname;
	$command .= " -set_state done ";
	($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform laptool -updaterun: $error_code", $lap_id);
	}

	return(0);
    }
    
    my $exposures = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from laptool -inactiveexp", $lap_id);

    my @clean_modes = (
	'chiptool -dbname @DBNAME@ -updaterun -set_state goto_cleaned -set_label goto_cleaned -label @LABEL@ -chip_id @CHIP_ID@',
#	'warptool -dbname @DBNAME@ -updaterun -set_state goto_cleaned -set_label goto_cleaned -label @LABEL@ -warp_id @WARP_ID@',
	'difftool -dbname @DBNAME@ -updaterun -set_state goto_cleaned -set_label goto_cleaned -label @LABEL@ -diff_id @DIFF_ID@',
	'magictool -dbname @DBNAME@ -updaterun -set_state full -set_label @LABEL@.old -magic_id @MAGIC_ID@',
	'magicdstool -dbname @DBNAME@ -updaterun -set_state goto_cleaned -state full -set_label goto_cleaned -stage chip -stage_id @CHIP_ID@',
	'magicdstool -dbname @DBNAME@ -updaterun -set_state goto_cleaned -state full -set_label goto_cleaned -stage warp -stage_id @WARP_ID@',
	'magicdstool -dbname @DBNAME@ -updaterun -set_state goto_cleaned -state full -set_label goto_cleaned -stage diff -stage_id @DIFF_ID@');
    
    foreach my $exposure (@$exposures) {
	if ($exposure->{is_in_use}) {
	    next;
	}
 	foreach my $clean_mode (@clean_modes) {
 	    my $command = $clean_mode;
	    $command =~ s/\@DBNAME\@/$dbname/g;
	    if ($command =~ /\@CHIP_ID\@/) {
		if (S64_IS_NOT_NULL($exposure->{chip_id})) {
		    $command =~ s/\@CHIP_ID\@/$exposure->{chip_id}/;
		}
		else {
		    next;
		}
	    }
	    if ($command =~ /\@WARP_ID\@/) {
		if (S64_IS_NOT_NULL($exposure->{warp_id})) {
		    $command =~ s/\@WARP_ID\@/$exposure->{warp_id}/;
		}
		else {
		    next;
		}
	    }
	    if ($command =~ /\@DIFF_ID\@/) {
		if (S64_IS_NOT_NULL($exposure->{diff_id})) {
		    $command =~ s/\@DIFF_ID\@/$exposure->{diff_id}/;
		}
		else {
		    next;
		}
	    }
	    if ($command =~ /\@MAGIC_ID\@/) {
		if ((S64_IS_NOT_NULL($exposure->{magicked}))&&
		     ($exposure->{magicked} > 0)) {
		    $command =~ s/\@MAGIC_ID\@/$exposure->{magicked}/;
		}
		else {
		    next;
		}
	    }
	    $command =~ s/\@LABEL\@/$exposure->{label}/;
	    
	    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		run(command => $command, verbose => $verbose);
	    unless ($success) {
		$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		&my_die("Unable to perform cleantool: $command : $error_code", "none", "none");
	    }
 	}
 	$exposure->{data_state} = 'cleaned';
 	update_this_exposure($exposure);
    }
    
    # Cleanup quickstacks.
    $command = "$laptool -stacks -lap_id $lap_id";
    $command .= " -dbname $dbname" if defined $dbname;
    
    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -stacks: $error_code", "none", "none");
    }
    if (@$stdout_buf == 0) {
	# Nothing to do.
	return(0);
    }
    
    my $stacks = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from laptool -inactiveexp", $lap_id);
    

    @clean_modes = (
	'stacktool -dbname @DBNAME@ -updaterun -set_state goto_cleaned -set_label goto_cleaned -label @LABEL@ -stack_id @STACK_ID@');
    foreach my $stack (@$stacks) {
	if (!S64_IS_NOT_NULL($stack->{quick_stack_id})) {
	    next;
	}
	foreach my $clean_mode (@clean_modes) {
	    my $command = $clean_mode;
	    $command =~ s/\@DBNAME\@/$dbname/g;
	    $command =~ s/\@LABEL\@/$stack->{label}/;
	    $command =~ s/\@STACK_ID\@/$stack->{quick_stack_id}/;

	    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		run(command => $command, verbose => $verbose);
	    unless ($success) {
		$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		&my_die("Unable to perform cleantool: $command : $error_code", "none", "none");
	    }
	}
    }    

    $command = "$laptool -updaterun -lap_id $lap_id";
    $command .= " -dbname $dbname " if defined $dbname;
    $command .= " -set_state done ";
    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform laptool -updaterun: $error_code", $lap_id);
    }
    return(0);
}
    
	    


#
# Utilities
################################################################################

sub my_die {
    my $msg = shift; # Warning message on die
    my $lap_id = shift; # identifier
    my $optional = shift;
    my $exit_code = shift;
    unless (defined $exit_code) {
	$exit_code = $PS_EXIT_PROG_ERROR;
    }
    carp($msg);
    exit $exit_code;
}

# Check to see if a 64bit integer is NULL or not.  This is probably fragile, but I don't know how to get a S64 NULL out.
sub S64_IS_NOT_NULL {
    if ($_[0] == 9223372036854775807) {
	return(0);
    }
    else {
	return(1);
    }
}

# Take the current exposure structure, and save it to the database.
sub update_this_exposure {
    my $exposure = shift;
    
    # These are required
    my $lap_id = $exposure->{lap_id};
    my $exp_id = $exposure->{exp_id};
    
    my $command = "$laptool -updateexp -lap_id $lap_id -exp_id $exp_id ";
    $command .= " -dbname $dbname " if defined $dbname;
    if (($exposure->{chip_id})&&(S64_IS_NOT_NULL($exposure->{chip_id}))) {
	$command .= " -set_chip_id  $exposure->{chip_id} ";
    }
    if (($exposure->{pair_id})&&(S64_IS_NOT_NULL($exposure->{pair_id}))) {
	$command .= " -set_pair_id  $exposure->{pair_id} ";
    }

    if ($exposure->{private}) {
	$command .= " -private ";
    }
    else {
	$command .= " -public ";
    }
    if ($exposure->{pairwise}) {
	$command .= " -pairwise ";
    }
    else {
	$command .= " -nopairwise ";
    }
#    if ($exposure->{active}) {
    unless ($exposure->{active} == 0) {
	$command .= " -active ";
    }
    else {
	$command .= " -inactive ";
    }
    if ($exposure->{data_state}) {
	$command .= " -set_data_state $exposure->{data_state} ";
    }
    unless ($do_nothing) {
	my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform laptool -updateexp: $error_code", $exposure->{lap_id}, $exposure->{proj_cell});
	}
    }
}    

