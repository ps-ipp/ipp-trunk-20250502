#! /usr/bin/env perl
#
#
# Create a response to a MOPS_DETECTABILITY_QUERY
#
#

use strict;
use warnings;

use Sys::Hostname;
use Carp;
use File::Basename;
use File::Copy;
use IPC::Cmd 0.36 qw( can_run run );
use Getopt::Long qw( GetOptions );
use Pod::Usage qw( pod2usage );
use File::Temp qw( tempfile tempdir);
use POSIX;  # for strftime

use PS::IPP::PStamp::RequestFile qw( :standard );
use PS::IPP::PStamp::Job qw( :standard );
use PS::IPP::Config qw( :standard );
use PS::IPP::Metadata::List qw( parse_md_list );

# use Astro::FITS::CFITSIO qw( :constants );
# Astro::FITS::CFITSIO::PerlyUnpacking(1);

my $host = hostname();

my $mode = 'queue_job';

my ($req_id, $product, $need_magic, $missing_tools, $project, $label);
my ($request_file, $outdir, $dbname, $dbserver, $verbose, $save_temps, $no_update);
GetOptions(
    'file=s'          =>      \$request_file,
    'req_id=s'        =>      \$req_id,
    'outdir=s'        =>      \$outdir,
    'label=s'         =>      \$label,
    'product=s'       =>      \$product,
    'mode=s'          =>      \$mode,
    'dbname=s'        =>      \$dbname,
    'dbserver=s'      =>      \$dbserver,
    'verbose'         =>      \$verbose,
    'save-temps'      =>      \$save_temps,
    'no-update'       =>      \$no_update,
    ) or pod2usage(2);

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --file --req_id --outdir --dbname",
	   -exitval => 3,
    ) unless
    defined $request_file and defined $outdir and defined $dbname and defined $req_id;

my $detect_query_read = can_run('detect_query_read') or (warn "Can't find detect_query_read" and $missing_tools = 1);
my $pstamptool = can_run('pstamptool') or (warn "Can't find pstamptool" and $missing_tools = 1);
my $fields = can_run('fields') or (warn "Can't find fields" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

unless (defined $verbose) {
    $verbose = 0;
}

my $ipprc = PS::IPP::Config->new();
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files


if (!$dbserver) {
    $dbserver = metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

$pstamptool .= " -dbname $dbname -dbserver $dbserver";

# Project name is hardcoded to gpc1 for the moment.
# -- set gpc1 by default and read in PROJECT column to update for actual query
$project = resolve_project($ipprc,"gpc1",$dbname,$dbserver);
my $imagedb = $project->{dbname};
if (!$imagedb) {
    my_die("failed to find imagedb for project: $project", $PS_EXIT_CONFIG_ERROR);
}

# get the query id and check the extname and version from the header
my $fields_output; 
{
    my $command = "echo $request_file | $fields -x 0 EXTNAME EXTVER QUERY_ID";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    
    $fields_output = join "", @$stdout_buf;
}
my (undef, $extname, $extver, $req_name) = split " ", $fields_output;

my_die("$request_file is missing one of EXTNAME EXTVER or QUERY_ID", $PS_EXIT_PROG_ERROR)
    if !(defined($extname) and defined($extver) and defined($req_name));
my_die("$request_file has EXTNAME $extname not MOPS_DETECTABILITY_QUERY table", $PS_EXIT_PROG_ERROR)
    if $extname ne "MOPS_DETECTABILITY_QUERY";
my_die("$request_file is version $extver expecting 1 or 2", $PS_EXIT_PROG_ERROR)
    if ($extver ne 1) and ($extver ne 2);

if ($req_id) {
    my $command = "$pstamptool -listreq  -name $req_name -not_req_id $req_id";
    # set verbose to false so that error message about request not found doesn't appear in parse_error.txt
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 0);
    if ($success) {
        # -listreq succeeded duplicate request name
        print STDERR "QUERY_ID $req_name has already been used\n";
        $command = "$pstamptool -addjob -job_type none -rownum 0 -state stop -fault $PSTAMP_DUP_REQUEST -req_id $req_id";
        unless ($no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            if (!$success) {
                my_die("$command failed", $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print STDERR "skipping $command\n";
        }
        
        my $datestr = strftime "%Y%m%d%H%M%S.$req_id", gmtime;
        $req_name = "ERROR.$datestr";
    }
    $command = "$pstamptool -updatereq -req_id $req_id  -set_name $req_name -set_outProduct $product";
    unless ($no_update) {
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            my_die("$command failed", $PS_EXIT_UNKNOWN_ERROR);
        }
    } else {
        print STDERR "skipping $command\n";
    }
}



#
# query is a hash which uses $fpa_id as the key
# The values are hash references which have the various parameter name as the key
#   The values of the parameter hashes are arrays which contain the values for individual rows 
#   in the detectabilty query request
my %query = ();

{
    #
    # Parse input request file using detect_query_read
    # result is a simple text file
    #
    my $dqr_command = "$detect_query_read --dbname $imagedb --input $request_file";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $dqr_command, verbose => $verbose);
    unless ($success) {
	# This is a problem, because I'm not sure how we handle a failure to read something.
	# We need to return a $PSTAMP_INVALID_REQUEST, I think, but if we can't read it, 
	# we can't send that response back.
	die("Unable to perform $dqr_command error code: $error_code");
    }
    my $Nrows = 0;
    {
	my @column_names = ();
        # split output into lines skip until the line which lists the column names is found
        # Parse subsequent lines
	foreach my $entry (split /\n/, (join "", @$stdout_buf)) {
	    if ($entry =~ /^#/) {
		@column_names = split /\s+/, $entry;
		shift(@column_names);  # Dump the hash sign.
	    }
	    elsif (scalar @column_names) {
		# ROWNUM RA1_DEG DEC1_DEG RA2_DEG DEC2_DEG MAG QUERY_ID FPA_ID MJD-OBS FILTER OBSCODE STAGE
		my %row_data;
		@row_data{@column_names} = (split /\s+/, $entry);
		for (my $i = 0; $i <= $#column_names; $i++) {
                    push @{ $query{$row_data{"FPA_ID"}}{$column_names[$i]} }, $row_data{$column_names[$i]};
                    $Nrows = scalar(keys(%query));
                    # print "$row_data{'FPA_ID'} $Nrows $i $column_names[$i] $row_data{$column_names[$i]}\n";
                }
	    }
	}
    }
    #
    # Identify target components.  This should properly collate targets on a single imfile.
    #
    my $query_id;
    foreach my $fpa_id (keys %query) {
	my %temp_hash;
	my $query_style = 'byexp';
	my $stage;
	my $filter;
	my $mjd;
	my $tess_id;
	my $component_id;
	# Confirm that we only have one stage/filter/mjd
	if ($fpa_id ne 'Not_Set') {   # We only need to check things that aren't the known odd case.
	    for (my $i = 0; $i <= $#{ $query{$fpa_id}{STAGE} }; $i++) {
		$temp_hash{STAGE}{$query{$fpa_id}{STAGE}[$i]} = 1;
		$temp_hash{FILTER}{$query{$fpa_id}{FILTER}[$i]} = 1;
		$temp_hash{'MJD-OBS'}{$query{$fpa_id}{'MJD-OBS'}[$i]} = 1;
		$temp_hash{PROJECT}{$query{$fpa_id}{PROJECT}[$i]} = 1;
		$temp_hash{COMPONENT_ID}{$query{$fpa_id}{COMPONENT_ID}[$i]} = 1;
		$temp_hash{TESS_ID}{$query{$fpa_id}{TESS_ID}[$i]} = 1;
	    }
	    if (scalar(keys(%{ $temp_hash{STAGE} })) == 1) {
		$stage = (keys(%{ $temp_hash{STAGE} }))[0];
	    }
	    else {
		exit_with_failure(21,"Too many STAGEs specified");
	    }
	    if (scalar(keys(%{ $temp_hash{FILTER} })) == 1) {
		$filter = (keys(%{ $temp_hash{FILTER} }))[0];
	    }
	    else {
		exit_with_failure(21,"Too many FILTERs specified");
	    }
	    if (scalar(keys(%{ $temp_hash{'MJD-OBS'} })) == 1) {
		$mjd = (keys(%{ $temp_hash{'MJD-OBS'} }))[0];
	    }
	    else {
		exit_with_failure(21,"Too many MJD-OBS specified");
	    }
	    # add non-error cases/options as well 
	    if (scalar(keys(%{ $temp_hash{PROJECT} })) == 1) {
                if ( (keys(%{ $temp_hash{PROJECT} }))[0] ne 'Not_Set') {
                    $imagedb = (keys(%{ $temp_hash{PROJECT} }))[0];
                    $project = resolve_project($ipprc,$imagedb,$dbname,$dbserver);
                    $imagedb = $project->{dbname};
                    if (!$imagedb) {
                        my_die("failed to find imagedb for project: $project", $PS_EXIT_CONFIG_ERROR);
                    }
                }
	    }
            if (scalar(keys(%{ $temp_hash{TESS_ID} })) == 1) {
                if ( (keys(%{ $temp_hash{TESS_ID} }))[0] ne 'Not_Set') {                
                    $tess_id = (keys(%{ $temp_hash{TESS_ID} }))[0];
                }
		else {
		    $tess_id = undef;
                }
            }
            if (scalar(keys(%{ $temp_hash{COMPONENT_ID} })) == 1) {
                if ( (keys(%{ $temp_hash{COMPONENT_ID} }))[0] ne 'Not_Set') { 
                    $component_id = (keys(%{ $temp_hash{COMPONENT_ID} }))[0];
                }
		else {
		    $component_id = undef;
                }
            }
	}

        ## 20190317 MEH hack to add to row
        # Set common request components
        my $option_mask |= 1;
        $option_mask |= $PSTAMP_SELECT_IMAGE;
        $option_mask |= $PSTAMP_SELECT_MASK;
        $option_mask |= $PSTAMP_SELECT_VARIANCE;
        $option_mask |= $PSTAMP_SELECT_PSF;

	# Set up a rowList with default values
	my @rowList;
	for (my $i = 0; $i <= $#{ $query{$fpa_id}{STAGE} }; $i++) {
	    $rowList[$i]->{CENTER_X} = $query{$fpa_id}{RA1_DEG}[$i];
	    $rowList[$i]->{CENTER_Y} = $query{$fpa_id}{DEC1_DEG}[$i];
	    $rowList[$i]->{ID} = $query{$fpa_id}{ROWNUM}[$i];
	    $rowList[$i]->{COORD_MASK} = 0;
            # 20190317 MEH hack add to row  
            $rowList[$i]->{OPTION_MASK} = $option_mask;
	    # Set default values
	    $query{$fpa_id}{BAD_COMPONENT}[$i] = 1;
	    $query{$fpa_id}{IMAGE}[$i] = 'no_image';
	    $query{$fpa_id}{MASK}[$i] = 'no_mask';
	    $query{$fpa_id}{WEIGHT}[$i] = 'no_weight';
	    $query{$fpa_id}{PSF}[$i] = 'no_psf';

	    $query{$fpa_id}{STAGE_ID}[$i] = 'no_id';
	    $query{$fpa_id}{IMAGE_DB}[$i] = 'no_imdb';
	    $query{$fpa_id}{NEED_MAGIC}[$i] = 'no_magic';
	    $query{$fpa_id}{MAGICKED}[$i] = 'no_magic';
	    $query{$fpa_id}{CATALOG}[$i] = 'no_catalog';
	    $query{$fpa_id}{COMPONENT_ID}[$i] = 'no_component';
	    $query{$fpa_id}{CLASS_ID}[$i] = 'no_class';

	    $query{$fpa_id}{STATE}[$i] = 'no_state';
	    $query{$fpa_id}{DATA_STATE}[$i] = 'no_dstate';
	    $query{$fpa_id}{FAULT}[$i] = 'no_fault';
	    $query{$fpa_id}{BURNTOOL_STATE}[$i] = 'no_btstate';
	}

	# Determine the query style for this fpa_id
	# fix remove g  for gpc2
	if ($fpa_id =~ /o.*g.*o/) {
	    $query_style = 'byexp';
	}
	elsif ($fpa_id =~ /\d+/) {
	    $query_style = 'byid';
	}
	elsif ($fpa_id eq 'Not_Set') {
            # no matching file was found skip
	    next;
	}
	else {
	    exit_with_failure(21,"Parse error in request file");
	}
	# Set common request components
#	my $option_mask |= 1;
#	$option_mask |= $PSTAMP_SELECT_IMAGE;
#	$option_mask |= $PSTAMP_SELECT_MASK;
#	$option_mask |= $PSTAMP_SELECT_VARIANCE;
#	$option_mask |= $PSTAMP_SELECT_PSF;

        # magic is dead
	my $need_magic = 0;
	if ($stage eq 'stack') {
	    $need_magic = 0;
	}
	my $mjd_min = int $mjd;
	my $mjd_max = $mjd + 1;
	
        my $req_filter;
        if ($filter ne 'Not_Set') {
            $req_filter = $filter . '%';
        }
	# set defqults if not_set
#        if ($imagedb eq 'Not_Set') {
#            $imagedb = 'gpc1';
#        }
#        if ($component_id eq 'Not_Set') {
#            $component_id = undef;
#        }
#        if ($tess_id eq 'Not_Set') {
#            $tess_id = undef;
#        }

	# Call the PStamp code to find the images that contain the target on the given MJD in the specified filter.
	my $pstamp_images_ref = locate_images($ipprc,$imagedb,
					  \@rowList,
					  $query_style,$stage,
					  $fpa_id,$tess_id,$component_id,
					  $option_mask,$need_magic,
					  $mjd_min,$mjd_max,$req_filter,undef,$verbose);  
	
	foreach my $this_image_ref (@{ $pstamp_images_ref }) {
            # loop over the rows that this component matched
            foreach my $valid_index (@{ $this_image_ref->{row_index} }) {
                $query{$fpa_id}{IMAGE}[$valid_index] = $this_image_ref->{image};
                $query{$fpa_id}{MASK}[$valid_index] = $this_image_ref->{mask};
                $query{$fpa_id}{WEIGHT}[$valid_index] = $this_image_ref->{weight};
                $query{$fpa_id}{PSF}[$valid_index] = $this_image_ref->{psf};
                $query{$fpa_id}{STAGE_ID}[$valid_index] = $this_image_ref->{stage_id};
                $query{$fpa_id}{IMAGE_DB}[$valid_index] = $this_image_ref->{imagedb};
                $query{$fpa_id}{NEED_MAGIC}[$valid_index] = $need_magic;
                $query{$fpa_id}{BAD_COMPONENT}[$valid_index] = 0;
                if (exists($this_image_ref->{astrom})) {
                    $query{$fpa_id}{CATALOG}[$valid_index] = $this_image_ref->{astrom};
                }
                else {
                    $query{$fpa_id}{CATALOG}[$valid_index] = $this_image_ref->{cmf};
                }
                if (exists($this_image_ref->{class_id})) {
                    $query{$fpa_id}{COMPONENT_ID}[$valid_index] = $this_image_ref->{class_id};
                    $query{$fpa_id}{CLASS_ID}[$valid_index] = $this_image_ref->{class_id};
                    
                }
                else {
                    $query{$fpa_id}{COMPONENT_ID}[$valid_index] = $this_image_ref->{skycell_id};
                    $query{$fpa_id}{CLASS_ID}[$valid_index] = 'fpa';
                }
                
                $query{$fpa_id}{STATE}[$valid_index] = $this_image_ref->{state};
                if (exists($this_image_ref->{data_state})) {
                    $query{$fpa_id}{DATA_STATE}[$valid_index] = $this_image_ref->{data_state};
                }
                else {
                    $query{$fpa_id}{DATA_STATE}[$valid_index] = $this_image_ref->{state};
                }
                $query{$fpa_id}{FAULT}[$valid_index] = 0;
		    $query{$fpa_id}{MAGICKED}[$valid_index] = $this_image_ref->{magicked};
                if ($stage eq 'chip') {
                    $query{$fpa_id}{BURNTOOL_STATE}[$valid_index] = $this_image_ref->{burntool_state};
                }
                
                # Determine if the data exists.
                if (($query{$fpa_id}{STATE}[$valid_index] eq 'goto_purged') or 
                    ($query{$fpa_id}{DATA_STATE}[$valid_index] eq 'purged') or
                    ($query{$fpa_id}{STATE}[$valid_index] eq 'drop') or 
                    ($query{$fpa_id}{STATE}[$valid_index] eq 'error_cleaned') or 
		    ($query{$fpa_id}{DATA_STATE}[$valid_index] eq 'error_cleaned') or
                    ($query{$fpa_id}{STATE}[$valid_index] eq 'goto_scrubbed') or 
                    ($query{$fpa_id}{DATA_STATE}[$valid_index] eq 'scrubbed')) {
                    
                    # image is gone and it's not coming back
                    $query{$fpa_id}{FAULT}[$valid_index] = $PSTAMP_GONE;
		    ## MEH why was this missing? should be added to other FAULT cases...
		    ## messes with row order, skipping the faulted rows on output, unclear if problem since happens in other cases too 
		    $query{$fpa_id}{BAD_COMPONENT}[$valid_index] = 1;
                }
                elsif ($need_magic and ($query{$fpa_id}{MAGICKED}[$valid_index] eq 0)) {
                    $query{$fpa_id}{FAULT}[$valid_index] = $PSTAMP_NOT_DESTREAKED;
                }
                elsif (($query{$fpa_id}{DATA_STATE}[$valid_index] ne 'full')) {		      
                    $query{$fpa_id}{FAULT}[$valid_index] = $PSTAMP_NOT_AVAILABLE;
                    
                    # updating stacks isn't implemented
                    if (($stage eq 'stack')) {
                        $query{$fpa_id}{FAULT}[$valid_index] = $PSTAMP_NOT_IMPLEMENTED;
                    }
                    # updating old burntool data isn't implemented
                    elsif ($stage eq 'chip') {
                        if ($query{$fpa_id}{BURNTOOL_STATE}[$valid_index] and 
                            (abs($query{$fpa_id}{BURNTOOL_STATE}[$valid_index]) < 14)) {
                            $query{$fpa_id}{FAULT}[$valid_index] = $PSTAMP_NOT_IMPLEMENTED;
			    }
                    }
                } # End determining error faults.
            }
        }
    }
}

my %update_request;
my %rows_for_component;
my %faulted_rows;

# now build a hash using image name as key which contains the list of rows for that image
foreach my $fpa_id (keys %query) {
    for (my $i = 0; $i <= $#{ $query{$fpa_id}{ROWNUM} }; $i++) {
        if ($query{$fpa_id}{BAD_COMPONENT}[$i] == 0) {
            push @{ $rows_for_component{$fpa_id}{$query{$fpa_id}{IMAGE}[$i]} }, $i;
        } else {
            $faulted_rows{$query{$fpa_id}{ROWNUM}[$i]} = $query{$fpa_id}{FAULT}[$i];
        }
    }
}

#
# Finally build the parameter lists and queue jobs for each component
#

my $job_num = 1;
foreach my $fpa_id (keys %rows_for_component) {
    foreach my $component (keys % {$rows_for_component{$fpa_id} } ) {
        my $i = $rows_for_component{$fpa_id}{$component}[0];
        my $dep_id = 0;
        my $data_state = $query{$fpa_id}{DATA_STATE}[$i];
        if ($data_state ne 'full') {
            # Queue this data to be updated
            my $stage = $query{$fpa_id}{STAGE}[$i];
            my $stage_id = $query{$fpa_id}{STAGE_ID}[$i];
            my $component = $query{$fpa_id}{COMPONENT_ID}[$i];
            my $imagedb = $query{$fpa_id}{IMAGE_DB}[$i];
            print "Need to UPDATE $stage $stage_id $component from $data_state\n";
            $dep_id = queue_update_run($req_id, $outdir, $label, $data_state, $stage, $stage_id, $component, 0, $imagedb);
        }

        my $outputBase = "$outdir/$job_num" . "_";

        # params_file is an mdc description containing the parameters for the job
        my $params_file = $outputBase . "params.mdc";

        open PARAMS, ">$params_file" or my_die("failed to create $params_file", $PS_EXIT_UNKNOWN_ERROR);

        # first the job params
        print PARAMS "dqueryJobParams METADATA\n";
        foreach my $key (keys %{ $query{$fpa_id} } ) {
            my $value = $query{$fpa_id}{$key}[0];
            print PARAMS "\t$key\t\tSTR\t$value\n";
        }
        print PARAMS "END\n\n";

        # then one structure with the coordinates for each row
        print PARAMS "dqueryCoord MULTI\n";

        my @coordkeys = qw(ROWNUM RA1_DEG DEC1_DEG RA2_DEG DEC2_DEG MAG);
        foreach my $row ( @{ $rows_for_component{$fpa_id}{$component} }) {
            print PARAMS "\ndqueryCoord METADATA\n";
            foreach my $key (@coordkeys) {
                my $value = $query{$fpa_id}{$key}[$row];
                print PARAMS "\t$key\t\tSTR\t$value\n";
            }
            print PARAMS "END\n";
        }
        close(PARAMS);

        # use first rownum for this component as the rownum for the job
        my $rownum = $query{$fpa_id}{ROWNUM}[$rows_for_component{$fpa_id}{$component}[0]];

        my $command = "$pstamptool -addjob -req_id $req_id -outputBase $outputBase -job_type detect_query -state run -rownum $rownum";
        $command .= " -dep_id $dep_id" if $dep_id;
        unless ($no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            if ($success) {
                my $job_id = join "", @$stdout_buf;
                print "   Queued job: $job_id\n";
            } else {
                my $exit_status = $error_code >> 8;
                $exit_status = $PS_EXIT_UNKNOWN_ERROR unless $exit_status;
                my_die("Unable to perform $command error code: $error_code", $exit_status);
            }
        } else {
            print STDERR "skipping $command\n";
        }
        $job_num++;
    }
}

foreach my $rownum (keys %faulted_rows ) {
    my $outputBase = "$outdir/$job_num" . "_";
    my $fault = $faulted_rows{$rownum};
    $fault = $PSTAMP_NO_IMAGE_MATCH if ($fault eq 'no_fault');
    print STDERR "insert faulted job for $rownum: $fault\n";
    my $command = "$pstamptool -addjob -req_id $req_id -outputBase $outputBase -job_type detect_query -state stop -rownum $rownum -fault $fault";
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        if ($success) {
            my $job_id = join "", @$stdout_buf;
            print "   Queued job: $job_id\n";
        } else {
            my $exit_status = $error_code >> 8;
            $exit_status = $PS_EXIT_UNKNOWN_ERROR unless $exit_status;
            my_die("Unable to perform $command error code: $error_code", $exit_status);
        }
    } else {
        print STDERR "skipping $command\n";
    }
    $job_num++;
}

{
    my $command = "$pstamptool -updatereq -req_id $req_id -set_name $req_name -set_outProduct $product";
    # $command .= " -set_fault $result" if $result;

    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
        my_die("$command failed",$PS_EXIT_UNKNOWN_ERROR);
        }
    } else {
        print STDERR "Skipping $command\n";
    }
}

exit 0;

sub queue_update_run {
    my ($req_id, $outdir, $label, $state, $stage, $stage_id, $component, $need_magic, $imagedb) = @_;

#    my ($state, $stage, $stage_id, $component, $need_magic, $imagedb) = split /\s+/, $data_to_update;
    
    if (($state ne 'cleaned') and ($state ne 'update') and ($state ne 'goto_cleaned')) {
	# We should have received one of these states, so if not, signal that we have a problem.
	my_die("$stage $stage_id is in unexpected state $state", $PS_EXIT_PROG_ERROR);
    }
    my $dep_id;
    my $command = "$pstamptool -getdependent -stage $stage -stage_id $stage_id -imagedb $imagedb -component $component ";
    $command .= " -outdir $outdir";
    $command .= " -need_magic" if $need_magic;

    my $rlabel = "ps_ud_" . $label if $label;
    $command .= " -rlabel $rlabel" if $rlabel;

    if (!$no_update) {
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    my_die("$command failed", $PS_EXIT_UNKNOWN_ERROR);
	}
	my $output = join "", @$stdout_buf;
	chomp $output;
	$dep_id = $output;

	my_die("pstamptool -getdependent returned invalid dep_id", $PS_EXIT_PROG_ERROR) if !$dep_id;
    } 
    else {
	print STDERR "skipping $command\n";
	$dep_id = 42;
    }

    return($dep_id);
}


sub my_die {
    my $message = shift;
    my $exit_code = shift;

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp("$message");
    exit($exit_code);
}

sub exit_with_failure {
    my $status = shift;
    my $message = shift;
    my_die($message, $status);
}

