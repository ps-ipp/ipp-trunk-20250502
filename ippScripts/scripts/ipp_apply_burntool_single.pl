#!/usr/bin/env perl

use warnings;
use strict;
use Carp;

use IPC::Cmd 0.36 qw( can_run run );
#use IPC::Run 0.36 qw( run );
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my $REALRUN = 1;

my ( $class_id, $exp_id, $this_uri, $previous_uri, $imfile_state, $camera, $dbname, $logfile, $verbose, $save_temps );
my ( $rerun, $fixit );
my $continue = 0;
GetOptions(
    'exp_id=s'         => \$exp_id,   # exposure identifier
    'class_id=s'       => \$class_id, # chip identifier
    'this_uri=s'       => \$this_uri, # uri for this image
    'previous_uri=s'   => \$previous_uri, # uri for the previous image
    'imfile_state=s'   => \$imfile_state, # current state of the imfile.
    'camera=s'          => \$camera,     # Camera
    'dbname|d=s'        => \$dbname, # Database name
    'logfile=s'         => \$logfile,
    'rerun'             => \$rerun,
#    'fix|f'             => \$fixit,
    'continue=i'        => \$continue,
    'verbose'           => \$verbose,   # Print to stdout
    'save-temps'        => \$save_temps, # Save temporary files?

    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --class_id --this_uri --exp_id --dbname",
    -exitval => 3,
    ) unless
    defined $class_id and
    defined $this_uri and
    defined $exp_id and
    defined $dbname;

# XXX watch out for TC3 and GPC2 here:
unless (defined $camera) {
    if ($this_uri =~ /ota/) {
        $camera = "GPC1";
    }
 }

my $missing_tools;
my $regtool  = can_run('regtool')  or (warn "Can't find regtool" and $missing_tools = 1);
my $funpack  = can_run('funpack')  or (warn "Can't find funpack" and $missing_tools = 1);
my $burntool = can_run('burntool') or (warn "Can't find burntool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $nebXattr = can_run('neb-xattr') or (warn "Can't find neb-xattr" and $missing_tools = 1);
my $nebreplicate = can_run('neb-replicate') or (warn "Can't find neb-replicate" and $missing_tools = 1);
#my $fpack    = can_run('fpack')    or (warn "Can't find fpack" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# IPP configuration (including nebulous)
my $ipprc = PS::IPP::Config->new( $camera ) or my_die("Unable to set up");

unless (defined $logfile) {
    $logfile = $this_uri;
    $logfile =~ s/fits/burn.log/;
}
$ipprc->redirect_output($logfile) if $logfile;

# Determine the value of a "good" burntool run.
my $config_cmd = "$ppConfigDump -camera $camera -get-key BURNTOOL.STATE.GOOD";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run ( command => $config_cmd, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to perform ppConfigDump: $error_code", $exp_id, $class_id, $PS_EXIT_SYS_ERROR);
}

my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
    &my_die("Unable to parse metadata config doc", $exp_id, $class_id, $PS_EXIT_SYS_ERROR);

my $burntoolStateGood = 999;
foreach my $cfg (@$recipeData) {
    if ($cfg->{name} eq 'BURNTOOL.STATE.GOOD') {
        $burntoolStateGood = $cfg->{value};
    }
}
if ($burntoolStateGood == 999) {
    &my_die("Failed to determine BURNTOOL.STATE.GOOD", $exp_id, $class_id, $PS_EXIT_SYS_ERROR);
}
my $outState = -1 * abs($burntoolStateGood);

print ">>$burntoolStateGood<<\n";

# Determine what to do if we're fixing.
# if ($fixit) {
#     ($exp_id,$this_uri,$previous_uri,$continue) = fix_burntool($exp_id,$this_uri,$class_id);
# }


# Precalculate what we should be able to do if we're continuing
my @continue_uris;
my @continue_exp_ids;
my @continue_states;
if ($continue > 0) {
    my $regtool_get_date   = "$regtool -processedimfile -exp_id $exp_id -class_id $class_id -dbname $dbname";
    my $dateobs;
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run ( command => $regtool_get_date, verbose => $verbose );
    unless ($success) {
	$continue = 0;
	last;
    }
    my $imfile_data = $mdcParser->parse(join "", @$stdout_buf);
    my $imfile_data2 = parse_md_list($imfile_data);
    if ($#{ $imfile_data2 } > 0) {
	&my_die("Too many entries returned for date query", $exp_id, $class_id, $PS_EXIT_SYS_ERROR);
    }
    foreach my $entry (@$imfile_data2) {
	$dateobs = $entry->{dateobs};
    }
    my $date = $dateobs; $date =~ s/T.*//;

    my $regtool_status_cmd = "$regtool -checkstatus -dateobs_begin $dateobs -dateobs_end ${date}T17:30:00 -class_id $class_id -dbname $dbname";
    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run ( command => $regtool_status_cmd, verbose => $verbose );
    unless ($success) {
	print "Unsuccessful at running regtool -checkstatus, aborting continue\n";
	$continue = 0;
    }
    if ($continue != 0) {
	my $night_data = $mdcParser->parse(join "", @$stdout_buf);
	my $night_data2 = parse_md_list($night_data);
	my $addable = 0;
	
	foreach my $entry (@$night_data2) {
	    my ($this_exp_id,$this_uri,$this_state);
	    if ($entry->{exp_id} == $exp_id) {
		$addable = 1;
		next;
	    }
	    if (($addable == 1)&&($#continue_exp_ids < $continue)) {
		$this_exp_id = $entry->{exp_id} ? $entry->{exp_id} : 0;
		$this_uri    = $entry->{uri} ? $entry->{uri} : 'NULL';
		$this_state  = $entry->{imfile_state} ? $entry->{imfile_state} : 'NULL';
		print ">> $this_exp_id $this_uri $this_state\n";
		if ($this_state eq 'pending_burntool') {
		    push @continue_exp_ids, $this_exp_id;
		    push @continue_uris, $this_uri;
		    push @continue_states, $this_state;
		}
		else {
		    last;
		}
	    }
	    if ($#continue_exp_ids >= $continue) {
		last;
	    }
	}
    }
}
$continue = $#continue_exp_ids + 1;

# Look over at least one uri/imfile/exp_id and do burntool on it.
my $next_exp_id;
for (my $iteration = 0; $iteration <= $continue; $iteration ++) {
# Set up files
    my $rawImfile = $this_uri;
    my $rawImfileReal = $ipprc->file_resolve($rawImfile, 0);
    
    my $outTable  = $this_uri;
    if($burntoolStateGood == 15) {
        $outTable =~ s/fits$/burn.v15.tbl/;	    
    } else {
        $outTable =~ s/fits$/burn.tbl/;  	    
    }

    my $outTableReal = $ipprc->file_resolve($outTable, 1);
    if (!$outTableReal) {
	&my_die("failed to resolve output file: $outTable",$exp_id,$class_id);
    }
    
    my $previousTable;
    my $previousTableReal;
    if ($previous_uri) {
	$previousTable = $previous_uri;
        if($burntoolStateGood == 15) {
            $previousTable =~ s/fits$/burn.v15.tbl/;	    
        } else {
            $previousTable =~ s/fits$/burn.tbl/;  	    
        }
	$previousTableReal = $ipprc->file_resolve($previousTable, 0);
        if (!$previousTableReal) {
            &my_die("failed to resolve input file: $previousTable",$exp_id,$class_id);
        }
    }
    
# Set state to processing:
    my $burntool_state = 0;
    
    my $status = vsystem ("$regtool -dbname $dbname -updateprocessedimfile -exp_id $exp_id -class_id $class_id -burntool_state -1", $REALRUN);
    if ($status) {
	&my_die("failed to update imfile",$exp_id,$class_id);
    }
    $burntool_state = 1;
    
    
# funpack
    my $tempfile = new File::Temp ( TEMPLATE => "burntool.${exp_id}.${class_id}.XXXX",
				    DIR => '/tmp/',
				    UNLINK => !$save_temps,
				    SUFFIX => '.fits');
    my $tempPixels = $tempfile->filename;
    
    $status = vsystem ("$funpack -S $rawImfileReal > $tempPixels", $REALRUN);
    if ($status) {
	&my_die("failed on funpack",$exp_id,$class_id);
    }
    
# burntool
    if (($previousTableReal)&&($previousTableReal ne '')) {
        print "Running: $burntool $tempPixels in=$previousTableReal out=$outTableReal tableonly=t persist=t\n";
	$status = vsystem ("$burntool $tempPixels in=$previousTableReal out=$outTableReal tableonly=t persist=t", $REALRUN);
    }
    else {
        print "Running: $burntool $tempPixels out=$outTableReal tableonly=t persist=t\n";
	$status = vsystem ("$burntool $tempPixels out=$outTableReal tableonly=t persist=t", $REALRUN);
    }
    if ($status) {
	&my_die("failed on burntool",$exp_id,$class_id);
    }
    &my_die("Unable to find output file: $outTableReal", $exp_id, $class_id) unless $ipprc->file_exists($outTableReal);
    
# Replicate files
    $status = vsystem ("$nebXattr --write $outTable user.copies:2",$REALRUN);
    $status = vsystem ("$nebreplicate --set_copies 2 $outTable",$REALRUN);

# Prep for next iteration if necessary.
    if ($iteration < $continue) {
	$next_exp_id = shift(@continue_exp_ids);
	my $command = "$regtool -dbname $dbname -updateprocessedimfile -exp_id $next_exp_id -class_id $class_id -set_state check_burntool";
	$status = vsystem ($command, $REALRUN);
	if ($status) {
	    $continue = 0;
	}
    }
# Set state to finished.
    my $command = "$regtool -dbname $dbname -updateprocessedimfile -exp_id $exp_id -class_id $class_id -burntool_state $outState";
    if ($imfile_state ne 'full') {
	$command .= " -set_state full ";
    }
	
    $status = vsystem ($command, $REALRUN);
    if ($status) {
	&my_die("failed to update imfile",$exp_id,$class_id);
    }

# Set values for the next iteration if we are doing another iteration.
    $previous_uri = $this_uri;
    if ($iteration < $continue) {
	$this_uri = shift (@continue_uris);
	$exp_id   = $next_exp_id;
	$imfile_state = shift(@continue_states);
    }

}
exit 0;

sub vsystem {
    my $command = shift;
    my $realrun = shift;

    print "$command\n";

    my $status = 0;
    if ($realrun) {
        $status = system ($command);
    }

    return $status;
}

sub my_die {
    my $message = shift;
    if ($#_ != -1) {
        my $exp_id = shift;
        my $class_id = shift;
	sleep(5); # Take a quick nap to let the database cool down, in case we failed doing an update.
        vsystem("$regtool -dbname $dbname -updateprocessedimfile -exp_id $exp_id -class_id $class_id -burntool_state 0 -set_state pending_burntool",1);
    }
    printf STDERR "$message\n";
    exit 1;
}
	
# sub fix_burntool {
#     my ($exp_id,$this_uri,$class_id,$burntoolGoodState) = @_;
#     my ($dateobs,$exp_name);
#     # Calculate the date for this exposure, and calculate back to find the burntool period.
#     my $regtool_get_date   = "$regtool -processedimfile -exp_id $exp_id -class_id $class_id -dbname $dbname";
#     my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
# 	run ( command => $regtool_get_date, verbose => $verbose );
#     unless ($success) {
# 	$continue = 0;
# 	return($exp_id,$this_uri,undef,0);
#     }
#     my $imfile_data = $mdcParser->parse(join "", @$stdout_buf);
#     foreach my $entry (@$imfile_data) {
# 	if ($entry->{name} eq 'dateobs') {
# 	    $dateobs = $entry->{value};
# 	}
# 	if ($entry->{name} eq 'exp_name') {
# 	    $exp_name = $entry->{value};
# 	}
#     }
#     my $date = $dateobs; $date =~ s/T.*//;
#     my $time = $dateobs; $time =~ s/.*T//;
#     my ($hour,$min,$sec) = split /\:/, $time; #/;
#     if ($min >= 30) {
# 	$min -= 30;
#     }
#     else {
# 	$min += 30;
# 	$hour -= 1;
#     }
#     my $dateobs_begin = sprintf("%sT%02d:%02d:%02d",$date,$hour,$min,$sec);


#     # Get the night data for this date range.
#     my $regtool_status_cmd = "$regtool -checkstatus -dateobs_begin $dateobs_begin -dateobs_end $dateobs -class_id $class_id -dbname $dbname";
#     ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
# 	run ( command => $regtool_get_date, verbose => $verbose );
#     unless ($success) {
# 	return($exp_id,$this_uri,undef,0);
#     }

#     my $night_data = $mdcParser->parse(join "", @$stdout_buf);

#     my $mode;
#     my ($row_exp_id,$row_uri,$row_data_state,$row_bt_state);
#     my $entry = @$night_data[-1];
#     foreach my $row (@$entry) {  # This is a single exposure, so let's get everything we'll need here.
# 	if ($row->{name} eq 'exp_id') {
# 	    $row_exp_id = $row->{value};
# 	}
# 	elsif ($row->{name} eq 'uri') {
# 	    $row_uri = $row->{uri};
# 	}
# 	elsif ($row->{name} eq 'data_state') {
# 	    $row_data_state = $row->{data_state};
# 	}		
# 	elsif ($row->{name} eq 'burntool_state') {
# 	    $row_bt_state = $row->{burntool_state};
# 	}		
#     }
#     # Determine what to do.
#     if ($row_exp_id == $exp_id) {
# 	if (($row_bt_state != $burntoolStateGood)) {
# 	    # Never been run.
# 	    $mode = 1;
# 	}
# 	else {
# 	    $mode = 0;
# 	}
#     }
#     if ($mode == 0) {
# 	# If we're here, then the database thinks we have a table for this uri on disk. Let's see if that's true.
# 	# Zero byte file/good alternate
# 	# Inconsistent md5sums
# 	# Missing this table only.
# 	my $neb = $ipprc->nebulous();
# 	my $table_key = $this_uri; $table_key =~ s/fits/burn.tbl/;

# 	my $neb_instances = $neb->find_instances($table_key, 'any');

# 	my $Ninstances = $#{ $neb_instances } + 1;

# 	if (($Ninstances == 0)||($Ninstances == 1)) {
# 	    #Missing this table only.  This should fall through to do a regular pass on this uri
# 	    $continue = 0;
# 	    $entry = @$night_data[-2];
# 	    foreach my $row (@$entry) {  # This is a single exposure, so let's get everything we'll need here.
# 		if ($row->{name} eq 'exp_id') {
# 		    $row_exp_id = $row->{value};
# 		}
# 		elsif ($row->{name} eq 'uri') {
# 		    $row_uri = $row->{uri};
# 		}
# 		elsif ($row->{name} eq 'data_state') {
# 		    $row_data_state = $row->{data_state};
# 		}		
# 		elsif ($row->{name} eq 'burntool_state') {
# 		    $row_bt_state = $row->{burntool_state};
# 		}		
# 	    }
# 	    $previous_uri = $row_uri;
# 	    # Go through to regular processing of this exposure.
# 	}
# 	elsif ($Ninstances == 2) {
# 	    $md5sum_A = md5sum(${ $neb_instances }[0]);
# 	    $md5sum_B = md5sum(${ $neb_instances }[1]);
# 	    if ($md5sum_A != $md5sum_B) {
# 		# md5sums are different
# 		if ($md5sum_A = 'd41d8cd98f00b204e9800998ecf8427e') {
# 		    # copy B -> A
# 		}
# 		elsif ($md5sum_B = 'd41d8cd98f00b204e9800998ecf8427e') {
# 		    # copy A -> B
# 		}
# 		else {
# 		    # different but unknown why
# 		}
# 	    }
# 	}
#     }

#     if ($mode == 1) {
# 	foreach my $entry (reverse(@$night_data)) {  # reverse it so we can start with the exposure we have and go backwards
# 	    foreach my $row (@$entry) {  # This is a single exposure, so let's get everything we'll need here.
# 		if ($row->{name} eq 'exp_id') {
# 		    $row_exp_id = $row->{value};
# 		}
# 		elsif ($row->{name} eq 'uri') {
# 		    $row_uri = $row->{uri};
# 		}
# 		elsif ($row->{name} eq 'data_state') {
# 		    $row_data_state = $row->{data_state};
# 		}		
# 		elsif ($row->{name} eq 'burntool_state') {
# 		    $row_bt_state = $row->{burntool_state};
# 		}		
# 	    }
# 	}
#     }

#     return($exp_id,$this_uri,$previous_uri,$continue);
# }
