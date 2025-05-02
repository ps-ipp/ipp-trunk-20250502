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

use PS::IPP::PStamp::RequestFile qw( :standard );
use PS::IPP::PStamp::Job qw( :standard );
use PS::IPP::Config qw( :standard );
use PS::IPP::Metadata::List qw( parse_md_list );

use Astro::FITS::CFITSIO qw( :constants );
Astro::FITS::CFITSIO::PerlyUnpacking(1);

#
# Set up
#
my $host = hostname();

my $EXTVER = 1.0;
my $EXTNAME = 'MOPS_DETECTABILITY_RESPONSE';
my ($req_id,$job_id,$req_name,$product,$need_magic,$missing_tools,$project);
my ($request_file,$output,$workdir,$dbname,$dbserver,$verbose,$save_temps,$ignore_wisdom);
GetOptions(
    'input=s'         =>      \$request_file,
    'output=s'        =>      \$output,
    'workdir=s'       =>      \$workdir,
    'job_id=s'        =>      \$job_id,
    'dbname=s'        =>      \$dbname,
    'dbserver=s'      =>      \$dbserver,
    'verbose'         =>      \$verbose,
    'save-temps'      =>      \$save_temps,
    'ignore-wisdom'   =>      \$ignore_wisdom,
    ) or pod2usage(2);

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --input --output --dbname",
	   -exitval => 3,
    ) unless
    defined $request_file and defined $output and defined $workdir and defined $dbname;

my $detect_query_read = can_run('detect_query_read') or (warn "Can't find detect_query_read" and $missing_tools = 1);
my $psphotForced      = can_run('psphotForced') or (warn "Can't find psphotForced" and $missing_tools = 1);
my $dquery_finish     = can_run('dquery_finish.pl') or (warn "Can't find dquery_finish.pl" and $missing_tools = 1);
my $ppCoord           = can_run('ppCoord') or (warn "Can't find ppCoord" and $missing_tools = 1);
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
# This is hardcoded in for the moment.
$project = resolve_project($ipprc,"gpc1",$dbname,$dbserver);
my $imagedb = $project->{dbname};
if (!$imagedb) {
    carp("failed to find imagedb for project: $project");
}

my %query = ();
my %image_list_hash = ();
my $wisdom_file = "${workdir}/wisdom.dat";
if ((-e $wisdom_file)&&!($ignore_wisdom)) {
    print "Reading wisdom file $wisdom_file instead of parsing...\n";
    open(WISDOM,"$wisdom_file") or my_die("failed to open wisdom file $wisdom_file");
    while(<WISDOM>) {
	chomp;
	my ($fpa_id,@key_values) = split /\s+/;
	my $index = $#{ $query{$fpa_id}{ROWNUM} } + 1;
	while ($#key_values > -1) {
	    my $key = shift(@key_values);
	    my $val = shift(@key_values);
	    # if we have wisdom, then we should have updated already.  If not, we'll bomb out later in the code.
	    if ($key eq 'FAULT') {
		$val = 0;
	    }
	    $query{$fpa_id}{$key}[$index] = $val;
	}
    }
    close(WISDOM);
} # End reading wisdom.
else {
    #
    # Parse input request file using detect_query_read (as it's already written).
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
    # Identify target images.  This should properly collate targets on a single imfile.
    #
    foreach my $fpa_id (keys %query) {
	my %temp_hash;
	my $query_style = 'byexp';
	my $stage;
	my $filter;
	my $mjd;
	# Confirm that we only have one stage/filter/mjd
	if ($fpa_id ne 'Not_Set') {   # We only need to check things that aren't the known odd case.
	    for (my $i = 0; $i <= $#{ $query{$fpa_id}{STAGE} }; $i++) {
		$temp_hash{STAGE}{$query{$fpa_id}{STAGE}[$i]} = 1;
		$temp_hash{FILTER}{$query{$fpa_id}{FILTER}[$i]} = 1;
		$temp_hash{'MJD-OBS'}{$query{$fpa_id}{'MJD-OBS'}[$i]} = 1;
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
	}

	# Set up a rowList with default values
	my @rowList;
	for (my $i = 0; $i <= $#{ $query{$fpa_id}{STAGE} }; $i++) {
	    $rowList[$i]->{CENTER_X} = $query{$fpa_id}{RA1_DEG}[$i];
	    $rowList[$i]->{CENTER_Y} = $query{$fpa_id}{DEC1_DEG}[$i];
	    $rowList[$i]->{ID} = $query{$fpa_id}{ROWNUM}[$i];
	    $rowList[$i]->{COORD_MASK} = 0;
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
	my $option_mask |= 1;
	$option_mask |= $PSTAMP_SELECT_IMAGE;
	$option_mask |= $PSTAMP_SELECT_MASK;
	$option_mask |= $PSTAMP_SELECT_VARIANCE;
	$option_mask |= $PSTAMP_SELECT_PSF;
	my $need_magic = 1;
	if ($stage eq 'stack') {
	    $need_magic = 0;
	}
	my $mjd_min = int $mjd;
	my $mjd_max = $mjd + 1;
	
        my $req_filter;
        if ($filter ne 'Not_Set') {
            $req_filter = $filter . '%';
        }
	
	# Call the PStamp code to find the images that contain the target on the given MJD in the specified filter.
	my $pstamp_images_ref = locate_images($ipprc,$imagedb,
					  \@rowList,
					  $query_style,$stage,
					  $fpa_id,undef,undef,
					  $option_mask,$need_magic,
					  $mjd_min,$mjd_max,$req_filter,undef,$verbose);  
	
	foreach my $this_image_ref (@{ $pstamp_images_ref }) {

            if (0) {
                foreach my $key (sort (keys %{ $this_image_ref } )) {
                    my $value = ${ $this_image_ref }{$key};
                    if ($key eq 'row_index') {
                        $value = join ' ', @{ $this_image_ref->{$key} };
                    }
                    print "$this_image_ref $key $value\n";
                }
            }


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
                    ($query{$fpa_id}{STATE}[$valid_index] eq 'goto_scrubbed') or 
                    ($query{$fpa_id}{DATA_STATE}[$valid_index] eq 'scrubbed')) {
                    
                    # image is gone and it's not coming back
                    $query{$fpa_id}{FAULT}[$valid_index] = $PSTAMP_GONE;
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
} # End calculating wisdom
my %update_request;
my %processing_request;

if ($job_id) {
    $wisdom_file = "$wisdom_file.$job_id";
}
open(WISDOM,">$wisdom_file") or my_die("failed to open wisdom file $wisdom_file");
foreach my $fpa_id (keys %query) {
    for (my $i = 0; $i <= $#{ $query{$fpa_id}{ROWNUM} }; $i++) {
        print WISDOM "$fpa_id\t";
        foreach my $key (keys %{ $query{$fpa_id} }) {
            print WISDOM "$key $query{$fpa_id}{$key}[$i]\t";
        }
        print WISDOM "\n";
        my $data_state = $query{$fpa_id}{DATA_STATE}[$i];
        if ($query{$fpa_id}{BAD_COMPONENT}[$i] == 0) {
            if ($data_state ne 'full') {
                
                @{ $update_request{$query{$fpa_id}{IMAGE}[$i]}{$query{$fpa_id}{FAULT}[$i]} } = 
                    ($query{$fpa_id}{STATE}[$i],$query{$fpa_id}{STAGE}[$i],$query{$fpa_id}{STAGE_ID}[$i],
                     $query{$fpa_id}{COMPONENT_ID}[$i],$query{$fpa_id}{NEED_MAGIC}[$i],$query{$fpa_id}{IMAGE_DB}[$i]);
            }
            push @{ $processing_request{$fpa_id}{$query{$fpa_id}{IMAGE}[$i]} }, $i;
        }
    }
}
close(WISDOM);

if (! defined $job_id) {
    # If there is anything that needs to be updated, create the update request list, and then exit the program.
    my $exit_code = 0;
    my $update_request_file = "${workdir}/update_request.dat";
    open(UPDATE_REQUEST,">$update_request_file") or my_die("failed to open update request_file $update_request_file");
    foreach my $images (keys %update_request) {
        foreach my $fault (keys %{ $update_request{$images} }) {
            if ($fault == 25) {
                $exit_code = 25;
            }
            elsif ($fault != 0) {
                $exit_code = 21;
            }
            my $update_request = join ' ', @{ $update_request{$images}{$fault} };
            print UPDATE_REQUEST "$update_request\n";
        }
    }
    close(UPDATE_REQUEST);
    if ($exit_code != 0) {
        exit($exit_code);
    }
}

# This duplicates stuff returned by PSTAMP, but my thought is to convert that ---^ into a conditional, in which I read
# from the wisdom.dat file.  This means only one pass on all that potentially slow stuff.  If that's the case, then 
# I can recalculate the processing request.

foreach my $fpa_id (keys %processing_request) {
    foreach my $image (keys %{ $processing_request{$fpa_id} }) {
	print "$fpa_id $image\t";
	foreach my $i (@{ $processing_request{$fpa_id}{$image} }) {
	    print "$i ";
	}
	print "\n";
    }
}

# run ppCoord and psphotForced to calculate the required data.
foreach my $fpa_id (keys %processing_request) {
    foreach my $image (keys %{ $processing_request{$fpa_id} }) {
	# Get this image specific data from the first entry. That entry is now the king of this set.
	my $index = $processing_request{$fpa_id}{$image}[0];
	my $fault = $query{$fpa_id}{FAULT}[$index];
	my $catalog = $query{$fpa_id}{CATALOG}[$index];
	my $psf   = $query{$fpa_id}{PSF}[$index];
	my $mask  = $query{$fpa_id}{MASK}[$index];
	my $weight= $query{$fpa_id}{WEIGHT}[$index];
	my $stage = $query{$fpa_id}{STAGE}[$index];
	my $stage_id = $query{$fpa_id}{STAGE_ID}[$index];
	my $component = $query{$fpa_id}{COMPONENT_ID}[$index];
        # print "Input is from $stage $stage_id $component\n";

	# if there's a fault or quality problem, then we can't process this image.
	# if (($fault != 0)||($query{$fpa_id}{BAD_COMPONENT}[$index] == 1)) {
	if ($fault = check_component($stage, $stage_id, $component, $imagedb)) {
            foreach my $i (@{ $processing_request{$fpa_id}{$image} }) {
                $query{$fpa_id}{PROC_ERROR}[$i] = $fault;
                
                $query{$fpa_id}{NPIX}[$i] = 0;
                $query{$fpa_id}{QFACTOR}[$i] = 0.0;
                $query{$fpa_id}{FLUX}[$i] = 0.0;
                $query{$fpa_id}{FLUX_SIG}[$i] = 0.0;
            }

	    next;
	}

	# Create coordinate file to convert to positions.
	my ($coordfile,$coordname) = tempfile("${workdir}/detect.coords.$index.XXXX", 
					      UNLINK => !$save_temps);
	my ($targetfile,$targetname) = tempfile("${workdir}/detect.targets.$index.XXXX", 
						UNLINK => !$save_temps);
	foreach my $i (@{ $processing_request{$fpa_id}{$image} }) {
	    print $coordfile "$query{$fpa_id}{RA1_DEG}[$i] $query{$fpa_id}{DEC1_DEG}[$i]\n";
	}
	
	# Convert the sky coordinates to image coordinates with ppCoord.
	my $command = "ppCoord -astrom $catalog -radec $coordname";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    my_die("Unable to perform $command. Error_code: $error_code",
		   $query{$fpa_id}{QUERY_ID}[$index],$fpa_id,$query{$fpa_id}{'MJD-OBS'}[$index],
		   $query{$fpa_id}{FILTER}[$index],$query{$fpa_id}{OBSCODE}[$index],$query{$fpa_id}{STAGE}[$index],
		   $error_code);
	}
	my @response = split /\n/, (join "", @$stdout_buf);
	my $i = 0;
	foreach my $line (@response) {
	    my ($r_ra,$r_dec,$trash,$r_x,$r_y,$r_chip) = split /\s+/, $line;
	    print $targetfile "$r_x $r_y\n";
	    if ((abs($r_ra - $query{$fpa_id}{RA1_DEG}[$processing_request{$fpa_id}{$image}[$i]]) < 1e-8)&&
		(abs($r_dec - $query{$fpa_id}{DEC1_DEG}[$processing_request{$fpa_id}{$image}[$i]]) < 1e-8)) {
		$query{$fpa_id}{X_PXL}[$processing_request{$fpa_id}{$image}[$i]] = $r_x;
		$query{$fpa_id}{Y_PXL}[$processing_request{$fpa_id}{$image}[$i]] = $r_y;
		$query{$fpa_id}{EXTENSION_BASE}[$processing_request{$fpa_id}{$image}[$i]] = $r_chip;
	    }
	    else {
		$error_code = $PS_EXIT_PROG_ERROR;
		my_die("Unable to match input RA/DEC with output RA/DEC: ($query{$fpa_id}{RA1_DEG}[$i],$query{$fpa_id}{DEC1_DEG}[$i]) -> ($r_ra,$r_dec) $error_code",
		   $query{$fpa_id}{QUERY_ID}[$index],$fpa_id,$query{$fpa_id}{'MJD-OBS'}[$index],
		   $query{$fpa_id}{FILTER}[$index],$query{$fpa_id}{OBSCODE}[$index],$query{$fpa_id}{STAGE}[$index],
		   $error_code);
	    }
	    $i++;
	}

	# Run psphotForced on the target list.
	my $tmpdir  = tempdir("detect.$index.XXXX", DIR => "${workdir}/", CLEANUP => !$save_temps);
	my $outroot = "$tmpdir/detectability.${stage}.${fpa_id}.${index}";
	$query{$fpa_id}{PROC_ERROR}[$index] = 0;
	my $psphot_cmd = "$psphotForced -psf $psf ";
	$psphot_cmd .= "-file $image ";
	$psphot_cmd .= "-mask $mask ";
	$psphot_cmd .= "-variance $weight ";
	$psphot_cmd .= "-srctext $targetname $outroot ";
	$psphot_cmd .= "-D OUTPUT.FORMAT PS1_DV1 ";
	
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $psphot_cmd, verbose => $verbose);
	unless ($success) {
	    $query{$fpa_id}{PROC_ERROR}[$index] = $PSTAMP_SYSTEM_ERROR;
	}
	
	# Why not parse out results here?
# Convert psphot output to response
	if ($query{$fpa_id}{PROC_ERROR}[$index] == 0) {
	    my $class_id = $query{$fpa_id}{CLASS_ID}[$index];
	    my $cmf = "${outroot}.${class_id}.cmf";
	    my ($tmp_Npix,$tmp_Qfact,$tmp_flux,$tmp_flux_error) = read_cmf_file($cmf,$query{$fpa_id}{EXTENSION_BASE}[$index]);
	    foreach ($i = 0; $i <= $#{ $processing_request{$fpa_id}{$image} }; $i++) {
		my $result_index = $processing_request{$fpa_id}{$image}[$i];
		$query{$fpa_id}{PROC_ERROR}[$result_index] = $query{$fpa_id}{PROC_ERROR}[$index];

		$query{$fpa_id}{NPIX}[$result_index] = ${ $tmp_Npix }[$i];
		$query{$fpa_id}{QFACTOR}[$result_index] = ${ $tmp_Qfact }[$i];
		$query{$fpa_id}{FLUX}[$result_index] = ${ $tmp_flux }[$i];
		$query{$fpa_id}{FLUX_SIG}[$result_index] = ${ $tmp_flux_error }[$i];
		
#		print "$fpa_id $image $#{ $processing_request{$fpa_id}{$image} } $result_index $i ${ $tmp_Npix }[$i]\n";
	    }
	}
	else {
	    foreach ($i = 0; $i <= $#{ $processing_request{$fpa_id}{$image} }; $i++) {
		my $result_index = $processing_request{$fpa_id}{$image}[$i];
		$query{$fpa_id}{PROC_ERROR}[$result_index] = $query{$fpa_id}{PROC_ERROR}[$index];

		$query{$fpa_id}{NPIX}[$result_index] = 0;
		$query{$fpa_id}{QFACTOR}[$result_index] = 0.0;
		$query{$fpa_id}{FLUX}[$result_index] = 0.0;
		$query{$fpa_id}{FLUX_SIG}[$result_index] = 0.0;
	    }
	}

	    	
    }
}

write_response_file($output,\%query);

#
# Utilities
#

# Taken largely from detect_query_read
sub read_cmf_file {
    my $cmf_file = shift;
    my $class_id = shift;
    my $extname = $class_id . ".psf";

    my @tmp_Npix = ();
    my @tmp_Qfact = ();
    my @tmp_flux = ();
    my @tmp_flux_err = ();

    my ($detect_F_col,$detect_N_col,$detect_flux_col,$detect_mag_col) = (-1, -1, -1, -1);

    
    my $status = 0;
    my $inFits = Astro::FITS::CFITSIO::open_file( $cmf_file, READONLY, $status ); # Open CMF
    check_fitsio($status);
    $inFits->movnam_hdu(BINARY_TBL, $extname, 0, $status) and check_fitsio($status);

    my $inHeader = $inFits->read_header();

    my $CMFversion = $inHeader->{EXTTYPE};
    $CMFversion =~ s/\'//g;
    $CMFversion =~ s/\s+//g;

    # This is the only data we actually care about
    my $column_defs;
    if ($CMFversion eq 'PS1_V2') {
	$column_defs = [
	    { name => 'PSF_QF',   type => '1E', writetype => TDOUBLE },
	    { name => 'PSF_NPIX', type => '1J', writetype => TLONG },
	    { name => 'PSF_INST_MAG', type => '1E', writetype => TDOUBLE },
	    { name => 'PSF_INST_MAG_SIG', type => '1E', writetype => TDOUBLE },
	    ];
    }
    elsif ($CMFversion eq 'PS1_DV1') {
	$column_defs = [
	    { name => 'PSF_QF',   type => '1E', writetype => TDOUBLE },
	    { name => 'PSF_NPIX', type => '1J', writetype => TLONG },
	    { name => 'PSF_INST_FLUX', type => '1E', writetype => TDOUBLE },
	    { name => 'PSF_INST_FLUX_SIG', type => '1E', writetype => TDOUBLE },
	    ];
    }

    my @colNames; 
    my @colTypes;
    my @colWriteType;
    my %colData;
    foreach my $colSpec (@$column_defs) {
	push @colNames, $colSpec->{name};
	push @colTypes, $colSpec->{type};
	push @colWriteType, $colSpec->{writetype};
    }
    my $numRows;
    $inFits->get_num_rows($numRows, $status) and check_fitsio($status);

    my $correct_error = 0;
#    print STDERR "Ncols:" .  $#{ $column_defs } . "\n";
    foreach my $col (@$column_defs) {
	my ($col_num,$col_type,$col_data);
	$inFits->get_colnum(0, $col->{name}, $col_num, $status) and check_fitsio($status);
	if ($status != 0) {
	    $status = 0;
	    next;
	}
	$inFits->get_coltype($col_num, $col_type, undef, undef, $status) and check_fitsio($status);
	$inFits->read_col($col_type, $col_num, 1, 1, $numRows, 0, $col_data, undef, $status) and check_fitsio($status);
#	print STDERR "$col\t>>" . $col->{name} . "<<\t>>" . @{ $col_data } . "<<\n";
	if ($col->{name} eq 'PSF_QF') {
	    @tmp_Qfact = @{ $col_data };
	}
	elsif ($col->{name} eq 'PSF_NPIX') {
	    @tmp_Npix = @{ $col_data };
	}
	elsif ($col->{name} eq 'PSF_INST_MAG') {
	    @tmp_flux = map { $_ = 10**(-0.4 * $_) } @{ $col_data };
	}
	elsif ($col->{name} eq 'PSF_INST_MAG_SIG') {
	    @tmp_flux_err = map { $_ = $_ / (2.5 * (log(exp(1)) / log(10))) } @{ $col_data };
	    $correct_error = 1;
	}
	elsif ($col->{name} eq 'PSF_INST_FLUX') {
	    @tmp_flux = @{ $col_data };
	}
	elsif ($col->{name} eq 'PSF_INST_FLUX_SIG') {
	    @tmp_flux_err = @{ $col_data };
	}
    }
    if ($correct_error) {
	for (my $c = 0; $c <= $#tmp_flux_err; $c++) {
	    $tmp_flux_err[$c] = $tmp_flux_err[$c] * $tmp_flux[$c];
	}
    }
    $inFits->close_file( $status ) and check_fitsio($status);    
#    print STDERR "$CMFversion\n";
#    print STDERR "Q: $#tmp_Qfact\t@tmp_Qfact\nN: $#tmp_Npix\t@tmp_Npix\n";
    return(\@tmp_Npix, \@tmp_Qfact, \@tmp_flux, \@tmp_flux_err);
}

# A lot of this pulled verbatim from Bill's detect_response_create file
sub write_response_file {
    my $outfile = shift;
    my $query_ref = shift;

    my %query = %{ $query_ref };

    my $columns;
    my $headers;

    my $EXTVER_IS_1 = (scalar(keys(%query)) == 1);
#    print "EXTVER: $EXTVER_IS_1\n";
    my ($query_id,$FPA_ID,$MJD_OBS,$filter,$obscode,$status);
    if ($EXTVER_IS_1 == 1) {
	# Specification of columns to write
	$columns = [
	    # matching rownum from detectability original request
	    { name => 'ROWNUM',   type => '20A', writetype => TSTRING }, 
	    # any errors that occurred during processing
	    { name => 'ERROR_CODE',   type => 'V', writetype => TULONG }, 
	    # number of pixels used in hypothetical PSF for the query detection
	    { name => 'DETECT_N', type => 'V',   writetype => TULONG },
	    # detectibility, indicating the fraction of PSF pixels detetable by IPP
	    { name => 'DETECT_F', type => 'D',   writetype => TDOUBLE },
	    # flux of the target source
	    { name => 'TARGET_FLUX', type => 'D', writetype => TDOUBLE },
	    # error in the flux of the target source
	    { name => 'TARGET_FLUX_SIG', type => 'D', writetype => TDOUBLE },
	    ];
	
	# Header translation table
	$headers = {
	    'QUERY_ID' => { name => 'QUERY_ID', type => TSTRING, 
			    comment => 'MOPS Query ID for this batch query' },
	    'FPA_ID' => { name => 'FPA_ID',   type => TSTRING, 
			  comment => 'original FPA_ID used at ingest' },
# 	    'MJD-OBS' => { name => 'FPA_ID',   type => TSTRING, 
# 			  comment => 'original FPA_ID used at ingest' },
# 	    'FILTER' => { name => 'FPA_ID',   type => TSTRING, 
# 			  comment => 'original FPA_ID used at ingest' },
# 	    'OBSCODE' => { name => 'FPA_ID',   type => TSTRING, 
# 			  comment => 'original FPA_ID used at ingest' },
	};
    }
    else {
	# Specification of columns to write
	$columns = [
	    # matching rownum from detectability original request
	    { name => 'ROWNUM',   type => '20A', writetype => TSTRING}, 
	    # any errors that occurred during processing
	    { name => 'ERROR_CODE',   type => 'V', writetype => TULONG }, 
	    # number of pixels used in hypothetical PSF for the query detection
	    { name => 'DETECT_N', type => 'V',   writetype => TULONG },
	    # detectibility, indicating the fraction of PSF pixels detetable by IPP
	    { name => 'DETECT_F', type => 'D',   writetype => TDOUBLE },
	    # flux of the target source
	    { name => 'TARGET_FLUX', type => 'D', writetype => TDOUBLE },
	    # error in the flux of the target source
	    { name => 'TARGET_FLUX_SIG', type => 'D', writetype => TDOUBLE },
	    # The FPA That would be in the header if it were to be there.
	    { name => 'FPA_ID',   type => '20A', writetype => TSTRING },
	    ];
	
	# Header translation table
	$headers = {
	    'QUERY_ID' => { name => 'QUERY_ID', type => TSTRING, 
			    comment => 'MOPS Query ID for this batch query' },
	};
    }	

    # Parse the list of columns
    my @colNames;                   # Names of columns
    my @colTypes;                   # Types of columns
    my %colData;                    # Data for each column
    my @colWriteType;                 # type to use to write
    foreach my $colSpec ( @$columns) {
	push @colNames, $colSpec->{name};
	push @colTypes, $colSpec->{type};
	push @colWriteType, $colSpec->{writetype};
	$colData{$colSpec->{name}} = [];
    }

    my $inHeader = { };

    # Hack to force the data to match the detect_response_create formats

    $inHeader->{QUERY_ID}->{value} = $query_id;
    if ($EXTVER_IS_1 == 1) {
	my $fpa_id = (keys(%query))[0];
	$inHeader->{FPA_ID}->{value} = $fpa_id;
    }
    
    # Fill the table columns with the data, making sure the flux is defined
    foreach my $fpa_id (keys %query) {
	$inHeader->{QUERY_ID}->{value} = $query{$fpa_id}{QUERY_ID}[0];
	if ($EXTVER_IS_1 == 1) {
	    my $fpa_id = (keys(%query))[0];
	    $inHeader->{FPA_ID}->{value} = $fpa_id;
	}

        my @keysa = keys %{$query{$fpa_id}};

        print "$fpa_id: @keysa\n";
	
	push @{$colData{'ROWNUM'}}, @{ $query{$fpa_id}{ROWNUM} };
	push @{$colData{'ERROR_CODE'}}, @{ $query{$fpa_id}{PROC_ERROR} };
	push @{$colData{'DETECT_N'}}, @{ $query{$fpa_id}{NPIX} };
	push @{$colData{'DETECT_F'}}, @{ $query{$fpa_id}{QFACTOR} };
	push @{$colData{'TARGET_FLUX'}}, @{ $query{$fpa_id}{FLUX} };
	push @{$colData{'TARGET_FLUX_SIG'}}, @{ $query{$fpa_id}{FLUX_SIG} };
	if ($EXTVER_IS_1 != 1) {
	    push @{$colData{'FPA_ID'}}, (map {$fpa_id} @{ $query{$fpa_id}{ROWNUM} });
	}
    }
    my $numRows = 0;
    # Back to detect_response_create:
    $status = 0;
#    print "$output\n";
    if (-e $output) {
        unlink $output or die "failed to unlink existing response file $output\n";
    }
    my $outFits = Astro::FITS::CFITSIO::create_file( $output, $status );
    check_fitsio( $status );
    $outFits->create_img( 16, 0, undef, $status );
    check_fitsio( $status );
    my $EXTNAME = 'MOPS_DETECTABILITY_RESPONSE';
    
    $outFits->create_tbl( BINARY_TBL(), $numRows, scalar @colNames, \@colNames, \@colTypes, undef, $EXTNAME, $status);
    check_fitsio( $status );

    # TODO: get the extension version number from somewhere common
    $outFits->write_key( TINT, 'EXTVER', $EXTVER, 'Version of this Extension', $status );
    check_fitsio( $status );

    # Write the Extension keywords
    foreach my $keyword ( keys %$headers ) {
	my $value = $inHeader->{$keyword}->{value}; # Header keyword value
	unless (defined $value) {
	    print "Can't find header keyword $keyword\n";
	    next;
	}
	$value =~ s/\'//g;
	my $name    = $headers->{$keyword}->{name}; # New name
	my $type    = $headers->{$keyword}->{type}; # Type
	my $comment = $headers->{$keyword}->{comment}; # Comment
	$outFits->write_key( $type, $name, $value, $comment, $status );
	check_fitsio( $status );
    }

    for (my $i = 0; $i < scalar @colNames; $i++) {
	my $colName = $colNames[$i];# Column name
	my $writeType = $colWriteType[$i];
	my $numRows = scalar(@{$colData{$colName}});
	unless(defined($writeType)) {
	    print "write type undefined for $colName\n";
	}
	unless(defined($numRows)) {
	    print "num Rows undefined for $colName\n";
	}
	unless(defined($status)) {
	    print "status undefined for $colName\n";
	}
	unless(defined($colData{$colName})) {
	    print "col data undefined for $colName\n";
	}
	unless(defined($colName)) {
	    print "column name undefined for $i\n";
	}
	print STDERR "$writeType $i $numRows $colName $status @{ $colData{$colName} }\n";
	$outFits->write_col( $writeType, $i + 1, 1, 1, $numRows, $colData{$colName}, $status );
	print STDERR "$writeType $i $numRows $colName $status\n";
	check_fitsio( $status );
    }
    $outFits->close_file( $status );
    return($status);
}

# From Astro::FITS::CFITSIO demo
sub check_fitsio
{
    my $status = shift;         # Status of FITSIO calls

    if ($status != 0) {
        my $msg;                # Message to output
        Astro::FITS::CFITSIO::fits_get_errstatus( $status , $msg );
	carp("CFITSIO error: $status => $msg");
        die "CFITSIO error: $msg\n";
    }
}

sub check_component {
    my ($stage, $stage_id, $component, $imagedb) = @_;

    print "Checking status of component $stage $stage_id $component\n";

    my $command;
    if ($stage eq 'diff') {
        $command = "difftool -diffskyfile -diff_id $stage_id -skycell_id $component";
    } elsif ($stage eq 'stack') {
        $command = "stacktool -sumskyfile -stack_id $stage_id";
    } elsif ($stage eq 'warp') {
        $command = "warptool -warped -warp_id $stage_id -skycell_id $component";
    } elsif ($stage eq 'chip') {
        $command = "chiptool -processedimfile -chip_id $stage_id -class_id $component";
    } else {
        die("check_component not implemented for stage $stage yet");
    }

    $command .= " -dbname $imagedb";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
        my $rc = $error_code >> 8;
        carp "$command failed $error_code $rc";
        exit $PS_EXIT_UNKNOWN_ERROR;
    }
    my $output = join "", @$stdout_buf;
    my $metadata = $mdcParser->parse($output);
    my $results = parse_md_list($metadata);
    if (scalar @$results != 1) {
        carp "$command returned too many components: " . scalar @$results;
        exit $PS_EXIT_UNKNOWN_ERROR;
    }
    my $it = $results->[0];

    if ($stage eq 'stack') {
        $it->{data_state} = $it->{state};
    }

    my $return_status = 0;
    if ($it->{quality}) {
        print "  Bad quality: $it->{quality}\n";
        $return_status = $PSTAMP_GONE;
    } elsif ($it->{fault}) {
        print "  Faulted: $it->{fault}\n";
        $return_status = $PSTAMP_GONE;
    } elsif ($it->{data_state} ne 'full') {
        # XXX does this work for stack?
        carp "  Faulted: $it->{data_state}\n";
        $return_status = $PSTAMP_GONE;
    } else {
        print "  Component ok.\n";
    }

    return $return_status;
}

sub my_die {
    my $message = shift;
    my $QUERY_ID = shift;
    my $FPA_ID = shift;
    my $MJD_OBS = shift;
    my $FILTER = shift;
    my $OBSCODE = shift;
    my $STAGE = shift;

    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp("$message : $QUERY_ID $FPA_ID $MJD_OBS $FILTER $OBSCODE $STAGE");
    exit($exit_code);

}

sub exit_with_failure {
    my $status = shift;
    my $message = shift;
    carp("$message");
    exit($status);
}

