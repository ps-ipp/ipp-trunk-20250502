###
###     PStamp/Job.pm
###     subroutines and constants related to Postage Stamp Jobs
###

package PS::IPP::PStamp::Job;

use strict;
use warnings;

our $VERSION = '1.0';

use base qw( Exporter );

our @EXPORT_OK = qw( 
                    locate_images
                    locate_images_for_row
                    resolve_project
                    getCamRunByCamID
                    runToolAndParse
                    parse_md_fast
                    );
our %EXPORT_TAGS = (standard => [@EXPORT_OK]);


use PS::IPP::PStamp::RequestFile qw( :standard );

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config qw( :standard );
use Carp;
use File::Temp qw(tempfile);
use File::Basename;
use POSIX;
use Time::HiRes qw(gettimeofday);
use IO::Handle;

my $dvo_verbose = 0;
my $save_temps = 0;

# caches of camProcessedExp objects. 
# Key is $exp_id
my %camRunByExpIDCache;
# Key is $cam_id
my %camRunByCamIDCache;

# cache of last project looked up
my $last_project = "";

my ($regtool, $chiptool, $camtool, $warptool, $stacktool, $difftool, $releasetool);
my ($pstamptool, $dvoImagesAtCoords, $whichimage, $ppConfigDump);

my $default_data_groups = "";

# old locate_images() function. Still used by dqueryparse
# Postage stamp server now uses locate_images_for_row()

sub locate_images {
    my $ipprc    = shift;   # required
    my $imagedb = shift;   # required
    my $rowList  = shift;   # required
    my $req_type = shift;   # required
    my $stage = shift;   # required
    my $id       = shift;   # required unless req_type eq bycoord or byskycell
    my $tess_id  = shift;
    my $component= shift;   # class_id or skycell_id
    my $option_mask  = shift;
    my $need_magic = shift;
    my $mjd_min  = shift;
    my $mjd_max  = shift;
    my $filter   = shift;
    my $data_group = shift;
    my $verbose  = shift;

    # we die in response to bad data in request files
    # The caller is responsible for updating the database
    # pstampparse.pl error checks now so this shouldn't happen
    &my_die("Unknown req_type: $req_type", $PS_EXIT_PROG_ERROR) 
        if ($req_type ne "byid") and
           ($req_type ne "byexp") and 
           ($req_type ne "bycoord") and 
           ($req_type ne "bydiff") and 
           ($req_type ne "byskycell");

if (0) {
    if (!$data_group and !$default_data_groups) {
        $default_data_groups = load_data_groups($verbose);
    }
}

    my $dateobs_begin;
    my $dateobs_end;
    if (!iszero($mjd_min)) {
        $dateobs_begin = mjd_to_dateobs($mjd_min);
    }
    if (!iszero($mjd_max)) {
        $dateobs_end = mjd_to_dateobs($mjd_max);
    }
    if (isnull($tess_id)) {
        $tess_id = undef;
    }
    if (isnull($data_group)) {
        $data_group = undef;
    }
    if (isnull($filter)) {
        $filter = undef;
    }

    if ($req_type eq "bycoord") {
        my $num_rows = scalar @$rowList;
        die "Unexpected number of rows found in rowList: $num_rows" if $num_rows != 1;
        my $row = $rowList->[0];
        my $x = $row->{CENTER_X};
        my $y = $row->{CENTER_Y};
        my $results = lookup_bycoord($ipprc, $row, $imagedb, $stage, $tess_id, $component, $need_magic, $x, $y, $dateobs_begin, $dateobs_end, $filter, $data_group, $option_mask, $mjd_min, $mjd_max, $verbose);
        return $results;
    }

    if (($req_type eq "byid") and ($stage eq "diff")) {
        # lookups of all of the information for diff images requires a two level lookup to
        # get the exposure information. Switching the req_type allows us to keep that code
        # in one place
        $req_type = "bydiff";
        my $results = lookup_diff($ipprc, $rowList, $imagedb, $id, $component, 1, $option_mask, $stage, $verbose);
        return $results;
    }

    if ($req_type eq "bydiff") {
        # for bydiff reuqests we go look up the diffRun to obtain exp_id, chip_id, warp_id, stack_id, or 
        # the image from the diffRun
        my $results = lookup_diff($ipprc, $rowList, $imagedb, $id, $component, 0, $option_mask, $stage, $verbose);
        if (!$results) {
            return undef;
        }
        if ($stage eq "diff") {
            # lookup_diff has done all of the work
            return $results;
        } 

        my $image = $results->[0];
        if ($stage eq "raw") {
            $req_type = "byid";
            $id = $image->{exp_id};
            return undef if !$id;
            # fall through and lookup byid
        } elsif ($stage eq "chip") {
            $req_type = "byid";
            $id = $image->{chip_id};
            return undef if !$id;
            # fall through and lookup byid
        } elsif ($stage eq "warp") {
            $req_type = "byid";
            $id = $image->{warp_id};
            $component = $image->{skycell_id};
            return undef if !$id;
            # fall through and lookup by warp_id
        } elsif ($stage eq "stack") {
            $req_type = "byid";
            $id = $image->{stack_id};
            return undef if !$id;
            # fall though and lookup by stack_id
        } else {
            # This is checked this elsewhere?
            print STDERR "Error: $stage is an unknown image type\n";
            return undef;
        }
    } elsif ($req_type eq "byskycell") {
        if (($stage eq "raw") or ($stage eq "chip")) {
            print STDERR "REQ_TYPE byskycell not supported for IMG_TYPE raw or chip\n";
            return undef;
        }
        if (!$tess_id or !$component) {
            print STDERR "component and tess_id are required for REQ_TYPE byskycell\n";
            return undef;
        }
    }
    my $results = lookup($ipprc, $rowList, $imagedb, $req_type, $stage, $id, $tess_id, $component,
        $need_magic, $dateobs_begin, $dateobs_end, $filter, $data_group, $option_mask, $mjd_min, $mjd_max, 
        0, 0,   # fwhm cuts are not applied here
        undef, 0, # no cam run information
        undef, undef, $verbose);

    return $results;
}

sub locate_images_for_row {
    my $ipprc    = shift; 
    my $imagedb  = shift;
    my $camera   = shift;
    my $row      = shift;
    my $verbose  = shift;

    my $req_type  = $row->{REQ_TYPE};

    # we die in response to bad data in request files
    # The caller is responsible for updating the database
    # pstampparse.pl error checks now so this shouldn't happen
    &my_die("Unknown req_type: $req_type", $PS_EXIT_PROG_ERROR) 
        if ($req_type ne "byid") and
           ($req_type ne "byexp") and 
           ($req_type ne "bycoord") and 
           ($req_type ne "bydiff") and 
           ($req_type ne "byskycell");

    my $stage     = $row->{IMG_TYPE};
    my $id        = $row->{ID};
    my $component = $row->{COMPONENT};
    my $tess_id   = $row->{TESS_ID};

    my $filter    = $row->{REQFILT};
    my $mjd_min   = $row->{MJD_MIN};
    my $mjd_max   = $row->{MJD_MAX};
    my $data_group = $row->{DATA_GROUP};

    # default_data_groups is a concept far past it's sell by date
if (0) {
    if (isnull($data_group) and !$default_data_groups) {
        $default_data_groups = load_data_groups($verbose);
    }
}

    my $rownum     = $row->{ROWNUM};
    my $job_type   = $row->{JOB_TYPE};
    my $option_mask= $row->{OPTION_MASK};

    my $dateobs_begin;
    my $dateobs_end;
    if (!iszero($mjd_min)) {
        $dateobs_begin = mjd_to_dateobs($mjd_min);
        if (!$dateobs_begin) {
            $row->{error_code} = $PSTAMP_INVALID_REQUEST;
            print STDERR "Invalid MJD_MIN: $mjd_min\n";
            return undef;
        }
    }
    if (!iszero($mjd_max)) {
        $dateobs_end = mjd_to_dateobs($mjd_max);
        if (!$dateobs_end) {
            $row->{error_code} = $PSTAMP_INVALID_REQUEST;
            print STDERR "Invalid MJD_MAX: $mjd_max\n";
            return undef;
        }
    }
    if (isnull($tess_id)) {
        $tess_id = undef;
    }
    if (isnull($data_group)) {
        $data_group = undef;
    }
    if (isnull($filter)) {
        $filter = undef;
    }

    if ($stage eq 'stack_summary') {
        # stack_summary jobs are so different from others that we lookup in a specialized function
        return lookup_stack_summary($ipprc, $row, $imagedb, $tess_id, $component, $filter, $data_group, $option_mask, $mjd_min, $mjd_max, $verbose);
    }

    if ($req_type eq "bycoord") {
        my $x = $row->{CENTER_X};
        my $y = $row->{CENTER_Y};
        my $results = lookup_bycoord($ipprc, $row, $imagedb, $stage, $tess_id, $component, 0, $x, $y, $dateobs_begin, $dateobs_end, $filter, $data_group, $option_mask, $mjd_min, $mjd_max, $verbose);
        return $results;
    }

    # for compatability with the lower level functions that haven't been converted to take a single row
    my $rowList = [$row];

    if (($req_type eq "byid") and ($stage eq "diff")) {
        # lookups of all of the information for diff images requires a two level lookup to
        # get the exposure information. Switching the req_type allows us to keep that code
        # in one place
        $req_type = "bydiff";
        my $results = lookup_diff($ipprc, $rowList, $imagedb, $id, $component, 1, $option_mask, $stage, $verbose);
        return $results;
    }

    my ($release_name, $survey, $default_tess_id) = get_release_info($row);
    if (!$tess_id and $default_tess_id) {
        # Don't want to set default tess_id in this context
        # byid: no the tess_id is defined by the run
        # byexp warp: This is the one case where it might make sense to use a default to avoid double lookup.
        #   this could be avoided if we did a two level lookup in that case. Find the warpRun first then use the 
        #   run's tess id to find the specific skycell(s).
        # bydiff: no tess_id defined by the diff
        # byskycell requires tess_id so this does not apply
        # note: currently get_release_info only sets default_tess_id for byskycell and bycoord requests
        if (ref($default_tess_id) eq 'ARRAY') {
            $tess_id = $default_tess_id->[0];
        } else {
            $tess_id = $default_tess_id;
        }
    }

    if ($req_type eq "bydiff") {
        # for bydiff reuqests we go look up the diffRun to obtain exp_id, chip_id, warp_id, stack_id, or 
        # the image from the diffRun
        my $results = lookup_diff($ipprc, $rowList, $imagedb, $id, $component, 0, $option_mask, $stage, $verbose);
        if (!$results) {
            return undef;
        }
        if ($stage eq "diff") {
            # lookup_diff has done all of the work
            return $results;
        } 

        my $image = $results->[0];
        if ($stage eq "raw") {
            $req_type = "byid";
            $id = $image->{exp_id};
            return undef if !$id;
            # fall through and lookup byid
        } elsif ($stage eq "chip") {
            $req_type = "byid";
            $id = $image->{chip_id};
            return undef if !$id;
            # fall through and lookup byid
        } elsif ($stage eq "warp") {
            $req_type = "byid";
            $id = $image->{warp_id};
            $component = $image->{skycell_id};
            return undef if !$id;
            # fall through and lookup by warp_id
        } elsif ($stage eq "stack") {
            $req_type = "byid";
            $id = $image->{stack_id};
            return undef if !$id;
            # fall though and lookup by stack_id
        } else {
            # This is checked this elsewhere?
            print STDERR "Error: $stage is an unknown image type\n";
            return undef;
        }
    } elsif ($req_type eq "byskycell") {
        if (($stage eq "raw") or ($stage eq "chip")) {
            print STDERR "REQ_TYPE byskycell not supported for IMG_TYPE raw or chip\n";
            return undef;
        }
        if (!$tess_id or !$component) {
            print STDERR "component and tess_id are required for REQ_TYPE byskycell\n";
            return undef;
        }
    }

    my $results = lookup($ipprc, $rowList, $imagedb, $req_type, $stage, $id, $tess_id, $component,
        0, $dateobs_begin, $dateobs_end, $filter, $data_group, $option_mask, $mjd_min, $mjd_max,
        getValOrZero($row->{FWHM_MIN}), getValOrZero($row->{FWHM_MAX}),
        undef, 0, $release_name, $survey,
        $verbose);

    return $results;
}

# The subroutine lookup handles byexp, byid, and byskycell lookups including lookups that are
# triggered by a bydiff or bycoord request

sub lookup {
    my $ipprc    = shift;
    my $rowList    = shift;
    my $imagedb = shift;
    my $req_type = shift;
    my $stage = shift;
    my $id       = shift;
    my $tess_id  = shift;
    my $component= shift;
    my $need_magic = shift;
    my $dateobs_begin = shift;
    my $dateobs_end   = shift;
    my $filter = shift;
    my $data_group = shift;
    my $option_mask = shift;
    my $mjd_min = shift;
    my $mjd_max = shift;
    my $fwhm_min = shift;
    my $fwhm_max = shift;
    my $selectedAstrom = shift;
    my $selected_cam_id = shift;
    my $release_name = shift;
    my $survey = shift;
    my $verbose  = shift;

    my $inverse = $option_mask & $PSTAMP_SELECT_INVERSE;
    my $unconvolved = $option_mask & $PSTAMP_SELECT_UNCONV;
    my $use_imfile_id = ($req_type eq "byid") && ($option_mask & $PSTAMP_USE_IMFILE_ID);

    my $command;
    my $id_opt;     # option for the lookup
    my $image_name;
    my $mask_name;
    my $weight_name;
    my $cmf_name;
    my $psf_name;
    my $backmdl_name;
    my $base_name;
    my $want_astrom;
    my $set_class_id;
    my $class_id;
    my $skycell_id;
    my $default_error = $PSTAMP_NO_IMAGE_MATCH;

    # note $magic_arg may be cleared below depending on $req_type
    my $magic_arg = $need_magic ? " -destreaked" : "";

    my $row = $rowList->[0];

    # all rows have the same center type
    my $skycenter = ! ($row->{COORD_MASK} & $PSTAMP_CENTER_IN_PIXELS);

    my $use_releasetool = 0;
    my $release_args;
    if (($req_type eq 'byexp' and $stage ne 'raw') or ($req_type eq 'byskycell' and $stage ne 'diff')) {
        # we use releasetool for lookups if we get here and have a survey or release specfied and
        # the survey is not 'bypass' 
        # That is a hook to allow sophisticated users to bypass the release mechanisms. Useful for
        # MOPS which is interested primarily in nightly data and wants to get their data immediately
        # without regard to timeleness of the relexp tables. 
        # This also alows us to defer loading release tables for gpc2
        if ($release_name or ($survey and $survey ne 'bypass')) {  
            $use_releasetool = 1;
            if ($release_name) {
                $release_args = " -release_name $release_name";
            } else {
                $release_args = " -priority_order";
            }
            $release_args .= " -surveyName $survey" if $survey;
        }
    }

    my $component_args;
    my $choose_components = 0;
    if ($stage eq "raw") {
        if ($component or $use_imfile_id or !$skycenter) {
            $command = "$regtool -processedimfile -dbname $imagedb";
            if ($component and $component ne 'all') {
                $class_id = $component;
                $component_args = " -class_id $class_id";
            }
        } else {
            $command = "$regtool -processedexp -dbname $imagedb";
            $choose_components = 1;
        }
        # XXX: for now restrict lookups to type object 
        # are stamps of detrend exposures interesting?
        $command .= " -exp_type object";
        $id_opt = $use_imfile_id ? "-raw_imfile_id" : "-exp_id";

        if ($component and $component ne 'all') {
            $class_id = $component;
            $component_args = " -class_id $class_id";
        }
        $want_astrom = 1;
        $set_class_id = 1;
    } elsif ($stage eq "chip") {
        # if the request is such that it will yield a single image per "run" or the
        # center is specified in pixel coordinates 
        # use chiptool -processsedimfile. Otherwise use chiptool -listrun and then
        # choose the chips containing the center pixel by calling selectComponets() below
        if ($use_releasetool) {
            $command = "$releasetool -dbname $imagedb -listrelexp $release_args";
            $choose_components = 1;
        } else {
            $command = "$chiptool -pstamp_order -dbname $imagedb";
            if ($component or $use_imfile_id or !$skycenter) {
                $command .= " -processedimfile";
                if ($component and $component ne 'all') {
                    $class_id = $component;
                    $component_args = " -class_id $class_id";
                }
            } else {
                $command .= " -listrun";
                $choose_components = 1;
            }
            $id_opt = $use_imfile_id ? "-chip_imfile_id" : "-chip_id";
        }
        # XXX: perhaps we should stop resolving images to this level here and do it at job run time.
        # With the mdc file it has all of the information that it needs.
        $image_name   = "PPIMAGE.CHIP";
        # $mask_name    = "PPIMAGE.CHIP.MASK";
        # XXX: really need to handle this properly!
        $mask_name    = "PSASTRO.OUTPUT.MASK";
        $weight_name  = "PPIMAGE.CHIP.VARIANCE";
        $cmf_name     = "PSPHOT.OUTPUT";
        $psf_name     = "PSPHOT.PSF.SAVE";
        $backmdl_name = "PSPHOT.BACKMDL";
        $base_name    = "path_base"; # name of the field for the chiptool output
        $want_astrom  = 1;
        $set_class_id = 1;
    } elsif ($stage eq 'warp') {
        if ($use_releasetool) {
            $command = "$releasetool -dbname $imagedb -listrelexp $release_args";
            $skycell_id = $component;
            $choose_components = 1;
            if ($req_type eq 'byskycell' and $skycenter) {
                # Add a wide coordinate cut to the releasetool command.
                # This greatly speeds up the query by reducing the number of exposures considered
                # to those near the region of interest.
                # The actual radius value that we use is not critical because the skycell cut does the relevant
                # selection.
                $command .= " -ra $row->{CENTER_X} -decl $row->{CENTER_Y} -radius 5.0";
            }
        } else {
            $command = "$warptool -pstamp_order -dbname $imagedb";
            if ($component or $use_imfile_id or !$skycenter) {
                $command .= " -warped";
                if ($component and $component ne 'all') {
                    $skycell_id = $component;
                    $component_args = " -skycell_id $skycell_id";
                }
            } else {
                $command .= " -listrun";
                $choose_components = 1;
            }
	    ## MEH 20210127 add tess_id to avoid reprocessing in other tessellations
	    ## may want something like component_args since only useful for byexp
	    ## and added there for warp, diff, stack 
	    $command .= " -tess_id $tess_id" if $tess_id; 
	    ##
            $id_opt = $use_imfile_id ? "-warp_skyfile_id" : "-warp_id";
        }
        $image_name   = "PSWARP.OUTPUT";
        $mask_name    = "PSWARP.OUTPUT.MASK";
        $weight_name  = "PSWARP.OUTPUT.VARIANCE";
        $cmf_name     = "PSWARP.OUTPUT.SOURCES";
        $psf_name     = "PSPHOT.PSF.SKY.SAVE";
        $base_name    = "path_base"; # name of the field for the warptool output
    } elsif ($stage eq "diff") {

        if ($component or $use_imfile_id or !$skycenter) {
            $command = "$difftool -diffskyfile -pstamp_order -dbname $imagedb";
            if ($component and $component ne 'all') {
                $skycell_id = $component;
                $component_args = " -skycell_id $skycell_id";
            }
        } else {
            $command = "$difftool -listrun -pstamp_order -dbname $imagedb";
            $choose_components = 1;
        }
        $command .= " -template" if $inverse;
        $id_opt = $use_imfile_id ? "-diff_skyfile_id" : "-diff_id";
        $image_name  = "PPSUB.OUTPUT";
        $mask_name   = "PPSUB.OUTPUT.MASK";
        $weight_name = "PPSUB.OUTPUT.VARIANCE";
        $cmf_name    = "PPSUB.OUTPUT.SOURCES";
        $psf_name    = "PSPHOT.PSF.SKY.SAVE";
        $base_name   = "path_base";
        my $diff_mode = getDiffMode($row->{RUN_TYPE});
        $command .= " -diff_mode $diff_mode" if $diff_mode;
    } elsif ($stage eq "stack") {
        $skycell_id = $component;
        if ($use_releasetool) {
            $command = "$releasetool -listrelstack -dbname $imagedb $release_args";
            $base_name = 'stack_path_base';
            my $stack_type = $row->{RUN_TYPE};
            if (!isnull($stack_type)) {
                if (lc($stack_type) eq 'notnightly') {
                    $command .= " -stack_type deep -stack_type reference";
                } else {
                    $command .= " -stack_type $stack_type";
                }
            }
        } else {
            $command = "$stacktool -sumskyfile -dbname $imagedb";
            # only consider stacks in full state.
            $command .= " -state full";
            $id_opt = "-stack_id";
            $base_name = 'path_base';
        }
        $component_args = " -skycell_id $skycell_id" if $skycell_id and $skycell_id ne  'all';

        if (!$unconvolved) {
            $image_name  = "PPSTACK.OUTPUT";
            $mask_name   = "PPSTACK.OUTPUT.MASK";
            $weight_name = "PPSTACK.OUTPUT.VARIANCE";
        } else {
            $image_name  = "PPSTACK.UNCONV";
            $mask_name   = "PPSTACK.UNCONV.MASK";
            $weight_name = "PPSTACK.UNCONV.VARIANCE";
        }
        # this is wrong but gets the right answer. Need to figure out how to find the
        # rule properly
        $cmf_name     = "PSWARP.OUTPUT.SOURCES";
        # $cmf_name    = "PSPHOT.OUTPUT";    # this puts .fpa. in the name
        $psf_name    = "PPSTACK.TARGET.PSF";
        # XXX TODO should we filter stacks so that only one per skycell/filter combintation is returned for
        # most requests. Currently unless a release is provided all that match are returned.
        # Actually maybe that is the right answer. User's simply need to specify a release value
        # if that is all that they want.
    } else {
        # this should have been caught before we get here.
        die "Unknown IMG_TYPE supplied: $stage";
    }

    my $filter_runs = 0;
    if ($req_type eq "byid") {
        $command .= " $id_opt $id";
        $command .= $component_args if $component_args;
        # don't include -destreaked if lookup is byid. Let pstampparse check so that the
        # error code returned to the client for a given component is 'not destreaked'
        # instead of 'not found'
        $magic_arg = "";
    } elsif ($req_type eq "byexp") {
        $command .= " -exp_name $id";
        $command .= $component_args if $component_args;
        # don't include -destreaked if lookup is byexp. Let pstampparse check so that the error code gives
        # the reason as 'not destreaked'
        $magic_arg = "";
        # remove duplicate runs for the same exposure.
        $filter_runs = 1;
    } elsif ($req_type eq "byskycell") {
        die "tess_id and component are required for byskycell" if !$tess_id or ! $skycell_id;
        $command .= " -tess_id $tess_id -skycell_id $skycell_id";
        if ($stage ne 'stack') {
            $filter_runs = 1;
        }
    } else {
        # this should be caught by caller
        &my_die("Unexpected req_type supplied: $req_type", $PS_EXIT_PROG_ERROR);
    }

    if ($stage ne "stack") {
        $command .= $magic_arg;
        $command .= " -dateobs_begin $dateobs_begin" if $dateobs_begin;
        $command .= " -dateobs_end $dateobs_end" if $dateobs_end;
    } elsif ($req_type ne "byid") {
        # stacks
        $command .= " -mjd_obs_begin $mjd_min" if $mjd_min;
        $command .= " -mjd_obs_end $mjd_max" if $mjd_max;
    }

    $command .= " -filter $filter" if !isnull($filter);

    if (!isnull($data_group)) {
        $command .= " -data_group $data_group";
    } elsif (!$use_releasetool and $req_type eq 'byskycell') {
        # Default data_groups are used by bycoord in the camera run lookup and
        # here for byskycell.
        $command .= $default_data_groups;
    }

    if ($use_releasetool) {
        # fwhm arguments are currently only supported when using releasetool for lookups
        $command .= " -fwhm_min $fwhm_min" if $fwhm_min;
        $command .= " -fwhm_max $fwhm_max" if $fwhm_max;
    }

    my $images = runToolAndParse($command, $verbose);

    if (!$images or scalar @$images == 0) {
        return undef;
    }

    if ($filter_runs) {
        # The image selectors are such that multiple runs my have be returned for the same exposure.
        # Return only the latest one.
        $images = filterRuns($stage, $choose_components, $need_magic, $images, $inverse, $verbose);
    }

    if ($choose_components) {
        # the list of "images" is actually a list of "Runs"
        # match the coords in the rows to the components in the runs
        # returns an actual list of images
        if ($skycenter and $req_type ne 'byskycell') {
            $images = selectComponents($ipprc, $imagedb, $req_type, $stage, $rowList, $images, $verbose);
        } else {
            $images = selectComponentsByName($ipprc, $imagedb, $req_type, $stage, $component, $rowList, $images, $verbose);
        }
    } else {
        # put index for each of the rows into all of the the selected images
        setRowRefs($rowList, $images);
    }

    my $camera;
    my $output = [];
    foreach my $image (@$images) {
        my $base;

        next if $image->{fault};
        if (($stage ne "raw") and $image->{quality}) {
            print STDERR "Selected $stage image has bad quality. Skipping.\n";
            next;
        }

        if ($base_name) {
            $base = $image->{$base_name};
            # if base is undef then the output pruducts for this image are incomplete
            # This may only happen for warps. (Actually the test for ignored that I added
            # just above probably solved the problem I was trying to solve with this test)
            next if !$base;
        } else {
            # raw images don't have path_base yet. Manufacture one
            my $uri = $image->{uri};
            my $path_base = $uri =~ s/\.fits//g;
            $image->{$path_base} = $path_base;
        }
        if (!$camera) {
            # This assumes that all images have the same camera
            $camera = $image->{camera};
            $ipprc->define_camera($camera);
        }
        # ok it's a keeper
        my $out = $image;

        # if uri is nil this will get overridded below
        # (we do this here for raw stage)
        $out->{image}  = $out->{uri};

        if ($set_class_id) {
            $class_id = $out->{class_id};
            $out->{component} = $class_id;
        } else {
            $out->{component} = $out->{skycell_id};
        }
        my $stage_id;
        if ($stage eq "raw") {
            $stage_id = $out->{exp_id};
        } elsif ($stage eq "chip") {
            $stage_id = $out->{chip_id};
        } elsif ($stage eq "warp") {
            $stage_id = $out->{warp_id};
        } elsif ($stage eq "diff") {
            $stage_id = $out->{diff_id};
            if ($inverse && $out->{bothways}) {
                $image_name  = "PPSUB.INVERSE";
                $mask_name   = "PPSUB.INVERSE.MASK";
                $weight_name = "PPSUB.INVERSE.VARIANCE";
                $cmf_name    = "PPSUB.INVERSE.SOURCES";
            }
        } elsif ($stage eq "stack") {
            $stage_id = $out->{stack_id};
	    # dquery wants these set
	    $out->{magicked} = 0;
            if ($use_releasetool) {
                # XXX check quality too
                if (($image->{state} eq 'calibrated') and
                    ($image->{skycal_path_base})) {
                    $out->{astrom} = $image->{skycal_path_base} . ".cmf";
                }
                $out->{data_state} = $out->{state} = $out->{stack_state};
            } else {
                $out->{data_state} = $out->{state};

                # XXX: Consider looking up skycal and staticsky results even if we are
                # not using releasetool
            }
        }
        $out->{stage_id} = $stage_id;
        $out->{stage}    = $stage;
        $out->{imagedb} = $imagedb;

        my $mask_base;
        if ($want_astrom) {
            if ($out->{cam_path_base}) {
                # XXX Cheating here
                my $astromSource = 'PSASTRO.OUTPUT';
                $out->{astrom} = $ipprc->filename($astromSource, $out->{cam_path_base});
            } elsif ($selectedAstrom) {
                $out->{astrom} = $selectedAstrom;
                $out->{cam_path_base} = $selectedAstrom;
                $out->{cam_id} = $selected_cam_id if $selected_cam_id;
                if ($selectedAstrom =~ /\.smf$/) {
                    $out->{cam_path_base} =~ s/\.smf$//;
                } elsif ($selectedAstrom =~ /\.cmf$/) {
                    $out->{cam_path_base} =~ s/\.cmf$//;
                } else {
                    die ("ERROR: don't know how to extract cam_path_base from $selectedAstrom\n");
                }
            } elsif (! defined $out->{astrom}) {
                if (! find_astrometry($ipprc, $imagedb, $out, $verbose)) {
                    print STDERR "failed to find astrometry for $stage $stage_id\n";
                    next;
                }
            }
            # XXX: cheating here
            # XXX: do this right by looking at the recipe
            $mask_base = $out->{cam_path_base};
        } else {
            $mask_base = $base;
        }

        if ($base) {
            $out->{image}  = $ipprc->filename($image_name,  $base, $class_id) if $image_name;
            $out->{mask}   = $ipprc->filename($mask_name,   $mask_base, $class_id) if $mask_name;
            $out->{weight} = $ipprc->filename($weight_name, $base, $class_id) if $weight_name;
            $out->{backmdl}= $ipprc->filename($backmdl_name,$base, $class_id) if $backmdl_name;
            if ($image->{staticsky_path_base}) {
                $out->{psf}    = $ipprc->filename("PSPHOT.STACK.PSF.SAVE", $image->{staticsky_path_base}, $image->{stack_id});
                # if astrom was not set above to the skycal cmf get the sources file from staticsky
                $out->{cmf}    = $ipprc->filename("PSPHOT.STACK.OUTPUT",   $image->{staticsky_path_base}, $image->{stack_id}) if !$out->{astrom};
            } else {
                $out->{psf}    = $ipprc->filename($psf_name,    $base, $class_id) if $psf_name;
                $out->{cmf}    = $ipprc->filename($cmf_name,    $base, $class_id) if $cmf_name;
            }
        }

        push @$output, $out;
    }

    return $output;
}

sub lookup_stack_summary {
    my ($ipprc, $row, $imagedb, $tess_id, $component, $filter, $data_group, $option_mask, $mjd_min, $mjd_max, $verbose) = @_;

    $row->{error_code} = $PSTAMP_NO_IMAGE_MATCH;

    $component = '' if $component eq 'all';

    my $req_type = $row->{REQ_TYPE};
    my $results;
    if ($req_type eq 'byid') {
        my $sass_id = $row->{ID};
        my $command = "$stacktool -dbname $imagedb -summary -sass_id $sass_id";
        $results = runToolAndParse($command, $verbose);
    } else {
        my ($release_name, $survey, $default_tess_id) = get_release_info($row);
        if (!$tess_id and $default_tess_id) {
            if (ref($default_tess_id) eq 'ARRAY') {
                # XXX: handle the case where default_tess_id is an array
                # For now just take the first entry (RINGS.V3)
                $tess_id = $default_tess_id->[0];
            } else {
                $tess_id = $default_tess_id;
            }
        }

        my $skycells;
        if ($req_type eq 'bycoord') {
            my $ra = $row->{CENTER_X};
            my $dec = $row->{CENTER_Y};
            $skycells = lookup_skycell_by_coords($ipprc, $tess_id, $component, $ra, $dec, $verbose);
        } elsif ($req_type eq 'byskycell') {
            if (!defined $tess_id or !defined $component) {
                print STDERR "Error: TESS_ID and  COMPONENT are required for REQ_TYPE byskycell\n";
                $row->{error_code} = $PSTAMP_INVALID_REQUEST;
            }
            my $skycell = {
                tess_id    => $tess_id,
                component => $component
            };
            push @$skycells, $skycell
        } else {
            print STDERR "Error: $req_type is not a valid REQ_TYPE for IMG_TYPE stack_summary\n";
            $row->{error_code} = $PSTAMP_INVALID_REQUEST;
            return undef;
        }

        if ($skycells) {
            # We have a list of skycells. Find matching projection cells.
            # and then matching rows in stackSummary

            my %projection_cells_found;
            foreach my $skycell (@$skycells) {
                my $tess_id = $skycell->{tess_id};
                my $skycell_id = $skycell->{component};

                my $projection_cell = skycell_id_to_projection_cell($skycell_id);

                # only need to include a projection cell once
                next if $projection_cells_found{$projection_cell};

                $projection_cells_found{$projection_cell} = 1;

                # print "$tess_id $skycell_id $projection_cell\n";

                my $command;
                if ($release_name or $survey) {
                    $command = "$releasetool -dbname $imagedb";
                    if ($release_name) {
                        $command .= " -release_name $release_name";
                    } else {
                        $command .= " -priority_order";
                    }
                    $command .= " -surveyName $survey" if $survey;
                } else {
                    $command = "$stacktool -dbname $imagedb";
                }
                $command .= " -summary -tess_id $tess_id";
                $command .= " -projection_cell $projection_cell";

                $command .= " -filter $filter" if $filter;
                $command .= " -data_group $data_group" if $data_group;

                my $these_results = runToolAndParse($command, $verbose);
                push @$results, @$these_results if $these_results;
            }
        }
    }

    if ($results) {
        foreach my $summary (@$results) {
            my $path_base = $summary->{path_base};
            my $projection_cell = $summary->{projection_cell};

            # we need to flesh out the hashes returned with values that the parser expects
            $summary->{stage} = 'stack_summary';
            $summary->{component} = $projection_cell;
            $summary->{stage_id} = $summary->{sass_id};
            $summary->{path_base} = "$path_base.$projection_cell";

            # The stack_summary stage doesn't follow the usual ipp conventions for file rules
            # and path_base. We leave it up to the program that runs the job to handle this.
            # However, the parser uses the image parameter to set the job's outputBase (by removing the .fits)
            # so we need to set it. Just use the new path_base.
            $summary->{image} = $summary->{path_base};
            $summary->{state} = 'full';
            $summary->{data_state} = 'full';
            $summary->{fault} = 0;
            $summary->{imagedb} = $imagedb;
            $summary->{row_index} = [];
            push @{$summary->{row_index}}, 0;

            # XXX: Are there any other required parameters? 
            # TODO: create a function to validate the components returned
            # and make sure that any required elements are there.
        }
    }

    return $results;
}

sub lookup_diff {
    my $ipprc    = shift;
    my $rowList  = shift;
    my $imagedb = shift;
    my $id       = shift;
    my $skycell_id = shift;
    my $byid     = shift;
    my $option_mask  = shift;
    my $stage = shift;
    my $verbose = shift;

    my $inverse = $option_mask & $PSTAMP_SELECT_INVERSE;

    my $command = "$difftool -dbname $imagedb -pstamp_order";

    my $choose_components = 0;
    if ($byid) {
        if ($skycell_id and ($skycell_id ne 'all')) {
            $command .= " -diffskyfile -diff_id $id -skycell_id $skycell_id";
        } else {
            $choose_components = 1;
            $command .= " -listrun -diff_id $id";
            # the following is a work around for the problem reported in ticket #1394
            # 'difftool -listrun returns multiple rows for stack-stack diffs'
            $command .= " -limit 1";
        }
    } else {
        $command .= " -diffskyfile -diff_skyfile_id $id";
    }

    my $diff_mode = getDiffMode($rowList->[0]->{RUN_TYPE});
    $command .= " -diff_mode $diff_mode" if $diff_mode;
    
    my $output = runToolAndParse($command, $verbose);

    my $n = $output ? scalar @$output : 0;
    if ($n > 1) {
        die ("difftool returned an unexpected number of diffskyfiles: $n");
    } elsif ($n == 0) {
        return undef;
    }

    my $images;
    if ($choose_components) {
        $images = selectComponents($ipprc, $imagedb, 'byid', 'diff', $rowList, $output, $verbose);
    } else {
        $images = $output;
    }

    my @results;
    foreach my $image (@$images) {
        my $skycell_id = $image->{skycell_id};

        if ($image->{fault}) {
            print STDERR "selected difference image $id $image->{diff_id} $skycell_id has fault: $image->{fault}\n";
            next;
        }
        if ($image->{quality}) {
            print STDERR "selected difference image $id $image->{diff_id} $skycell_id has poor quality: $image->{quality}\n";
            next;
        }

        # XXX: The logic below is somewhat messed up. See XXX: ... below
        #
        # The standard way to do a diff is warp - stack 
        # so we interpret the requested image in that way

        # XXX difftool currently returns max long long for null
        # these checks are ready if we switch the code to return zero for null
        my $stack1 = $image->{stack1};
        if (($stack1 == 0) or ($stack1 == 9223372036854775807)) {
            $stack1 = undef
        }
        my $stack2 = $image->{stack2};
        if (($stack2 == 0) or ($stack2 == 9223372036854775807)) {
            $stack2 = undef
        }
        my $stack_id;
        if ($image->{diff_mode} == 2) {
            # XXX: .. stack_id wasn't getting set
            $stack_id = $stack2;
            $image->{stack_id} = $stack_id;
        }
        if ($stack1 and $stack2) {
            # we have a stack - stack diff (well it might be stack - warp....)
            # but at any rate we only handle image type diff and stack
            if (($stage ne "diff") and ($stage ne "stack")) {
                print STDERR "lookup_diff: cannot lookup IMG_TYPE $stage bydiff from a stack stack diff run\n";
                setErrorCodes($rowList, $PSTAMP_INVALID_REQUEST);
                # all images will be the same so we can stop
                last;
            }
            # stack-stack diff
            # XXX: define another flag for this
            if (! $inverse) {
                $stack_id = $stack1;
            } else {
                $stack_id = $stack2;
            }
            $image->{stack_id} = $stack_id;
        } else {
            my ($warp_id, $exp_id, $exp_name, $chip_id, $cam_id);
            if ($inverse and !$image->{bothways}) {
                print STDERR "Inverse images requested for diffRun that is not bothways. Ignoring inverse.\n";
                $inverse = 0;
            }
            if ($inverse) {
                $warp_id =  $image->{warp2};
                $exp_id = $image->{exp_id_2};
                $exp_name = $image->{exp_name_2};
                $chip_id = $image->{chip_id_2};
                $cam_id = $image->{cam_id_2};
            } else {
                $warp_id =  $image->{warp1};
                $exp_id = $image->{exp_id_1};
                $exp_name = $image->{exp_name_1};
                $chip_id = $image->{chip_id_1};
                $cam_id = $image->{cam_id_1};
            }
            # XXX difftool currently returns max long long for null
            # this line is ready if we switch the code to return zero for null
            if ($warp_id and ($warp_id != 9223372036854775807)) {
                $image->{warp_id} = $warp_id;
                $image->{exp_id} = $exp_id;
                $image->{exp_name} = $exp_name;
                $image->{chip_id} = $chip_id;
                $image->{cam_id} = $cam_id;
            }
        }

        # XXX: If difftool doesn't return a camera insert it here
        if (!$image->{camera}) {
            $image->{camera} = "GPC1";
            # lie about the magicked status since stack stack diffs don't require destreaking
            # we can remove this once diff_mode gets included
            $image->{magicked} = 42;
        }

        if ($stage eq "diff") {
            my @imageList = ($image);

            setRowRefs($rowList, \@imageList) unless $choose_components;
            # the $image is going to be returned directly in this case so we need to duplicate
            # some of processing that lookup does for other img_types
            if ($image->{camera}) {
                $ipprc->define_camera($image->{camera});
                my $filerule_base = $inverse ? "PPSUB.INVERSE" : "PPSUB.OUTPUT";
                $image->{image}  = $ipprc->filename($filerule_base, $image->{path_base});
                $image->{mask}   = $ipprc->filename($filerule_base . ".MASK", $image->{path_base});
                $image->{weight} = $ipprc->filename($filerule_base . ".VARIANCE", $image->{path_base});
                $image->{cmf}    = $ipprc->filename($filerule_base . ".SOURCES", $image->{path_base});
                $image->{psf}    = $ipprc->filename("PSPHOT.PSF.SKY.SAVE", $image->{path_base});
                $image->{stage_id} = $image->{diff_id};
                $image->{stage}    = "diff";
                $image->{imagedb} = $imagedb;
                $image->{component} = $image->{skycell_id};
            } else {
                # XXX this will only happen if the minuend is not a warp. See hack above
                print STDERR "WARNING: cannot resolve camera so cannot find file rules\n";
                next;
            }
        }
        push @results, $image;
    }
    return \@results;
}

sub lookup_bycoord {
    my $ipprc      = shift;
    my $row        = shift;
    my $imagedb    = shift;
    my $stage      = shift;
    my $tess_id    = shift;
    my $component  = shift;
    my $need_magic = shift;
    my $ra         = shift;
    my $dec        = shift;
    my $dateobs_begin  = shift; # these are used for single frame lookups
    my $dateobs_end    = shift;
    my $filter     = shift;
    my $data_group = shift;
    my $option_mask = shift;
    my $mjd_min = shift;        # these are used for stack lookups
    my $mjd_max = shift;        # these are used for stack lookups
    my $verbose    = shift;

    my $fwhm_min = getValOrZero($row->{FWHM_MIN});
    my $fwhm_max = getValOrZero($row->{FWHM_MAX});
    

    my @tess_ids;
    my ($release_name, $survey, $default_tess_id) = get_release_info($row);
    if ($tess_id) {
        push @tess_ids, $tess_id;
    } elsif ($default_tess_id) {
        if (ref($default_tess_id) eq 'ARRAY') {
            @tess_ids = @$default_tess_id;
        } else {
            push @tess_ids, $default_tess_id;
        }
    }

    my $rowList = [$row];

    my $results = [];
    if (($stage eq "raw") or ($stage eq "chip")) {

        my $chips = lookup_by_cam_id_and_coords($ipprc, $imagedb, $stage,
            $ra, $dec, $need_magic, $dateobs_begin, $dateobs_end, $filter, $data_group, $verbose,
            $release_name, $survey, $fwhm_min, $fwhm_max);

        if (!$chips or scalar @$chips == 0) {
            setErrorCodes($rowList, $PSTAMP_NO_IMAGE_MATCH);
        } else {
            foreach my $chip (@$chips) {
                next if $component and ($chip->{component} ne $component);
                my $these_results = lookup($ipprc, $rowList, $imagedb, "byid", $stage, $chip->{id},
                    # lookup doesn't need tess_id in this context
                    # $tess_id,
                    undef, 
                    $chip->{component}, $need_magic, 
                    $dateobs_begin, $dateobs_end, $filter, $data_group, $option_mask, $mjd_min, $mjd_max,
                    $fwhm_min, $fwhm_max,
                    $chip->{astrom}, $chip->{cam_id}, undef, undef,
                    $verbose);

                next if !$these_results;
                push @$results, @$these_results;
            }
        }
        
    } else {
        # this should have been checked elsewhere
        die "unexpected image type $stage" if ($stage ne "warp") 
                                              and ($stage ne "stack") and ($stage ne "diff");

        foreach my $tess_id (@tess_ids) {
            my $skycells = lookup_skycell_by_coords($ipprc, $tess_id, $component, $ra, $dec, $verbose);

            if ($skycells and scalar @$skycells != 0) {
                # XXX: We are not applying the fwhm cuts unless we use releasetool
                foreach my $skycell (@$skycells) {
                    my $these_results = lookup($ipprc, $rowList, $imagedb, "byskycell", $stage, undef,
                        $skycell->{tess_id}, $skycell->{component}, $need_magic, 
                        $dateobs_begin, $dateobs_end, $filter, $data_group, $option_mask, $mjd_min, $mjd_max, 
                        $fwhm_min, $fwhm_max,
                        undef, 0, $release_name, $survey,
                        $verbose);

                    next if !$these_results;
                    push @$results, @$these_results;
                }
            }
        }
        if (scalar @$results == 0) {
            setErrorCodes($rowList, $PSTAMP_NO_IMAGE_MATCH);
        }
    }

    return $results;
}

# lookup__by_cam_id_and_coords
# given an ra, dec, and optionally other parameters, find camera runs for exposures
# that are within some distance of the coordinates

sub lookup_by_cam_id_and_coords {
    my $ipprc       = shift;
    my $imagedb     = shift;
    my $stage       = shift;
    my $ra          = shift;
    my $dec         = shift;
    my $need_magic  = shift;
    my $dateobs_begin  = shift;
    my $dateobs_end = shift;
    my $filter     = shift;
    my $data_group = shift;
    my $verbose    = shift;
    my $release_name = shift;
    my $survey      = shift;
    my $fwhm_min    = shift;
    my $fwhm_max    = shift;

    my $search_radius = 1.6; # XXX: this should be camera specific

    my $command;
    my $using_camtool = 0;
    if ($release_name or ($survey and $survey ne 'bypass')) {
        $command = "$releasetool -dbname $imagedb -listrelexp";
        if ($survey) {
            $command .= " -surveyName $survey";
        }
        if ($release_name) {
            $command .= " -release_name $release_name";
        } else {
            $command .= " -priority_order";
        }
        if ($fwhm_min != 0) {
            $command .= " -fwhm_min $fwhm_min";
        }
        if ($fwhm_max != 0) {
            $command .= " -fwhm_max $fwhm_max";
        }
    } else {
        $using_camtool = 1;
        $command = "$camtool -dbname $imagedb -processedexp -pstamp_order";
        # NOTE: we are applying the data_group to the camera run.
        # If we're looking for chip stage images there is no guarentee that 
        # chipRun.data_group eq camRun.data_group. In practice this is almost
        # always the case. If this turns out to be a problem we can defer
        # the data_group test to when we look up the chipProcessedImfiles
        if ($data_group) {
            $command .= " -data_group $data_group";
        } else {
            $command .= $default_data_groups;
        }
        $command .= " -destreaked" if $need_magic;
    }
    $command .= " -ra $ra -decl $dec -radius $search_radius";
    $command .= " -dateobs_begin $dateobs_begin" if $dateobs_begin;
    $command .= " -dateobs_end   $dateobs_end"   if $dateobs_end  ;
    $command .= " -filter $filter" if $filter;

    # run the tool and parse the output
    my $results = runToolAndParse($command, $verbose);
    if (!$results) {
        return undef;
    }
    my $components;
    my $last_exp_id = 0;
    foreach my $result (@$results) {
        my $cam_id = $result->{cam_id};
        my $exp_id = $result->{exp_id};

        # go on if we already have a result for this exposure
        next if $exp_id eq $last_exp_id;

        next if $result->{quality};

        if ($using_camtool) {
            next if $result->{fault};
        } else {
            $result->{path_base} = $result->{cam_path_base};
        }
        updateCamRunCache($result);

        $last_exp_id = $exp_id;

        # XXX Use file rule
        my $astrom = $result->{path_base} . ".smf";
        my $astrom_resolved = $ipprc->file_resolve($astrom);
        next if !$astrom_resolved;
        if (! -e $astrom_resolved) {
            print "$astrom_resolved not found skipping\n";
            next;
        }

        my $start_dvo = gettimeofday();
        my $command = "$dvoImagesAtCoords -astrom $astrom_resolved $ra $dec";
        # run the tool and parse the output
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $dvo_verbose);
        my $dtime_dvo = gettimeofday() - $start_dvo;
#        print "Time to run dvoImagesAtCoords: $dtime_dvo\n";
        unless ($success) {

            my $result_code = $error_code >> 8;
            if ($result_code == $PSTAMP_NO_OVERLAP) {
                print "no overlap for $astrom\n" if $verbose;
                next;
            }
            # unexpected result code
            die "unexpected result code: $result_code from $command\n";
        }

        my $output = join "", @$stdout_buf;
        if (!$output) {
            # this shouldn't happen when dvoImagesAtCoord exits with zero status
            die "no output returned from $command\n";
        }

        # dvoImagesAtCoords should return a single line
        # rownum ra dec class_id
        my @lines = split "\n", $output;
        my $n = scalar @lines;
        if ($n != 1) {
            # XXX: There is a bug in dvo where each component is listed twice
            # When that gets fixed remove the conditional and just die
            if ($n ne 2 or $lines[0] ne $lines[1]) {
                print STDERR "unexpected number of lines returned by dvoImagesAtCoords: $n\n";
                print STDERR "OUTPUT:\n$output\n";
                # actually this seems to happen sometimes. Probably due to a problem with
                # astrometry. Just skip this camRun
                print STDERR "skipping: $astrom\n";
                next;
            }
        }

        my (undef, $ra_out, $dec_out, $class_id) = split " ", $lines[0];
        if (!$class_id) {
            die "unexpected output from dvoImagesAtCoords: $lines[0]";
        }

        print "cam_id $cam_id exp_id $result->{exp_id} chip_id $result->{chip_id} $class_id\n";

        # build the hash to return
        # XXX: very few of the entries in this hash are currently used.
        # Used are: id, component
        # I had plans to use it for the camRun lookup optimization, but the cache solves the
        # problem
        my $comp = {
            exp_id    => $result->{exp_id},
            exp_name  => $result->{exp_name},
            chip_id   => $result->{chip_id},
            cam_id    => $result->{cam_id},
            class_id  => $class_id,
            component => $class_id,
            astrom    => $astrom
        };
        if ($stage eq "chip") {
            $comp->{id} = $result->{chip_id};
            $comp->{state} = $result->{chip_state};
            $comp->{magicked} = 0;
        } else {
            $comp->{id} = $result->{exp_id};
            $comp->{state} = 'full';
            $comp->{magicked} = $0;
        }
        push @$components, $comp;
    }

    return $components;
}

sub lookup_skycell_by_coords {
    my $ipprc      = shift;
    my $requested_tess_id    = shift;
    my $requested_skycell  = shift;
    my $ra         = shift;
    my $dec        = shift;
    my $verbose    = shift;

    $requested_tess_id = "" if isnull($requested_tess_id);
    $requested_skycell = "" if isnull($requested_skycell);

    my @lines;
    my $looked_up_fast;
    if ($requested_tess_id) {
        # try using the last method first
        $looked_up_fast = lookup_skycells_fast($ipprc, \@lines, $requested_tess_id, $ra, $dec, $verbose);
    } 
    
    if (!$looked_up_fast) {
        my $command = "$whichimage $ra $dec";
        $command .= " --tess_id $requested_tess_id" if $requested_tess_id;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
        unless ($success) {
            print STDERR @$stderr_buf;
            return undef;
        }

        my $output = join "", @$stdout_buf;
        if (!$output) {
            print STDERR "no output returned from $command\n" if $verbose;
            return undef;
        }
        @lines = split "\n", $output;
        if (!scalar @lines) {
            # can this happen? if $output is not null?
            print STDERR "no output returned from $command\n" if $verbose;
            return undef;
        }
    }
    my $runs;
    foreach my $line (@lines) {
        my ($ra_out, $dec_out, $tess_id, $skycell_id) = split " ", $line;
        die "unexpected output from whichimage" if !$tess_id or !$skycell_id;

        if ($requested_tess_id) {
            next if ($requested_tess_id ne $tess_id);
        } else {
            # skip these obsolete tesselations unless they were explicitly asked for
            next if $tess_id eq "FIXNS";
            next if $tess_id eq "ALLSKY";
            next if $tess_id eq "RINGS.V0";
        }
        next if $requested_skycell and ($skycell_id ne $requested_skycell);

        # build the hash to return
        my $run = {
            tess_id   => $tess_id,
            component => $skycell_id,
        };
        push @$runs, $run;
    }

    return $runs;
}

# cache of results of ppConfigDump
my %astromSources;

# find the astrometry file for a given exposure
# return undef if no completed camRun exists
# XXX Right now this probably only works cameras where the astrometry is done at the camera stage
# What other possibilities are there for ASTROM.SOURCE besides PSASTRO.OUTPUT?

sub find_astrometry {
    my $ipprc = shift;
    my $imagedb = shift;
    my $image = shift;      # hashref to output of the lookup tool
    my $verbose = shift;

    my $mdcParser;

    my $exp_id = $image->{exp_id};

    my $camRun = getCamRunByExpID($exp_id);
    if (!$camRun) {
        my $command = "$camtool -dbname $imagedb -processedexp -exp_id $exp_id -pstamp_order";
        # run the tool and parse the output
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
        unless ($success) {
            print STDERR @$stderr_buf;
            return 0;
        }

        $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

        my $output = join "", @$stdout_buf;
        if (!$output) {
            print STDERR "no output returned from $command\n" if $verbose;
            return 0;
        }
        my $camruns = parse_md_fast($mdcParser, $output);
        if (!$camruns) {
            return 0;
        }

        # If there are multiple cam runs for this exposure, take the last completed one with good quality
        # on the assumption that it has the best astrometry.
        foreach my $cr (@$camruns) {
            if (($cr->{state} eq 'full') and ($cr->{quality} eq 0) and ($cr->{fault} eq 0)) {
                $camRun = $cr;
                last;
            }
        }
        # XXX: this looks like a bug at least if ASTROM.SOURCE eq PSASTRO.OUTPUT
        if (!$camRun) {
            # no cam runs for this exposure id therefore best astrometry is whatever is in the header
            return 0;
        }
        updateCamRunCache($camRun);
    }
    my $camRoot = $camRun->{path_base};
    my $camera = $image->{camera};
    my $astromSource = $astromSources{$camera};
    if (!$astromSource) {
        if ($camera eq "GPC1") {
            # XXX: CHEATER !
            $astromSource = "PSASTRO.OUTPUT";
        } else {
            my $command = "$ppConfigDump -camera $camera -dump-recipe PSWARP -";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                die("Unable to perform ppConfigDump: $error_code");
            }
            $mdcParser = PS::IPP::Metadata::Config->new if !$mdcParser; # Parser for metadata config files
            my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
                die("Unable to parse metadata config doc");
            $astromSource = metadataLookupStr($metadata, 'ASTROM.SOURCE');
        }
        $astromSources{$camera} = $astromSource;
    }

    # XXX: Is this code correct if ASTROM.SOURCE ne "PSASTRO.OUTPUT" ?
    my $astromFile = $ipprc->filename($astromSource, $camRoot);
    if (!$astromFile) {
        print STDERR "failed to find astrometry file from $astromSource $camRoot\n";
        return 0;
    }

    $image->{astrom} = $astromFile;
    $image->{cam_path_base} = $camRoot;
    $image->{cam_id} = $camRun->{cam_id};

    return 1;
}

# splits meta data config input stream into single units to work around the pathalogically
# slow parser. This is similar to and adapted from code in various ippScripts.
sub parse_md_fast {
    my $mdcParser = shift;
    my $input = shift;
    my $output = ();

    my @whole = split /\n/, $input;
    my @single = ();

    my $n;
    while ( ($n = @whole) > 0) {
        my $value = shift @whole;
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
	    push @single, "\n";

            my $list = parse_md_list( $mdcParser->parse( join("\n", @single ) ) ) or
                print STDERR "Unable to parse metdata config doc" and return undef;
#            my $num = @$list;
#            print STDERR "list has $num elments\n";
            push @$output, $list->[0];

            @single = ();
        }
    }
    return $output;
}

sub mjd_to_dateobs {
    my $mjd = shift;

    if ($mjd < 54000) {
        # print STDERR "MJD value too small minimum is 54000\n";
        $mjd = 54000;
    }

    my $ticks = ($mjd - 40587.0) * 86400;

    # CONVERT from TAI to UTC
    # XXX: do this correctly
    if ($mjd >= 54832) {
        $ticks += -34;
    } else {
        $ticks += -33;
    }

    my ($sec, $min, $hr, $day, $mon, $year) = gmtime($ticks);

    return sprintf "'%4d-%02d-%02dT%02d:%02d:%02dZ'", $year+1900, $mon+1, $day, $hr, $min, $sec;
}

sub isnull {
    my $val = shift;

    return (!defined($val) or ($val eq "") or (lc($val) eq "null"));
}

sub iszero {
    my $val = shift;
    return (!defined($val) or ($val == 0));
}

sub getValOrZero {
    my $val = shift;
    return iszero($val) ? 0.0 : $val;
}

# resolve_project()
# get project specific information
sub resolve_project {
    my $ipprc = shift;
    my $project_name = shift;

    findTools();

    if (!$project_name) {
        carp ("project is not defined");
        return undef;
    }

    if ($last_project and ($project_name eq ($last_project->{name}))) {
        return $last_project;
    }

    my $dbname = shift;
    my $dbserver = shift;

    my $verbose = 0;

    my $command = "$pstamptool -project -name $project_name";
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " -dbserver $dbserver" if defined $dbserver;

    my $proj_hash = runToolAndParse($command, $verbose);
    if (!$proj_hash or scalar @$proj_hash == 0) {
        print STDERR "Project $project_name not found\n";
        return undef;
    }

    $last_project = $proj_hash->[0];

    $ipprc->define_camera($last_project->{camera});

    return $last_project;
}

sub updateCamRunCache {
    my $camRun = shift;
    die "updateCamRunCache: camRun is nil" if !$camRun;

    my $exp_id = $camRun->{exp_id};
    die "updateCamRunCache: exp_id is nil" if !$exp_id;

    my $cam_id = $camRun->{cam_id};
    die "updateCamRunCache: cam_id is nil" if !$cam_id;

    $camRunByCamIDCache{$cam_id} = $camRun;

    # if this camRun is newer than previous update the cache.
    # This assumes that the newest has the "best" astrometry
    my $previous = $camRunByExpIDCache{$exp_id};
    if ($previous) {
        my $previous_cam_id = $previous->{cam_id};
        return if $cam_id < $previous_cam_id;
    }

    $camRunByExpIDCache{$exp_id} = $camRun;
}

sub getCamRunByExpID {
    my $exp_id = shift;
    &my_die ("getCamRun: exp_id is nil", $PS_EXIT_PROG_ERROR) if !$exp_id;

    return $camRunByExpIDCache{$exp_id};
}

sub getCamRunByCamID {
    my $cam_id = shift;
    &my_die ("getCamRun: cam_id is nil", $PS_EXIT_PROG_ERROR) if !$cam_id;

    return $camRunByCamIDCache{$cam_id};
}

sub selectComponents {
    my $ipprc = shift;
    my $imagedb = shift;
    my $req_type = shift;
    my $stage  = shift;
    my $rowList = shift;
    my $runList = shift;
    my $verbose = shift;
    my $results = [];
    
    my ($pointsList, $pointsListName) = tempfile ("/tmp/pointsList.XXXX", UNLINK => !$save_temps);
    my $npoints = 0;
    ## MEH hack -- 
    my $pi=3.14159265359;
    my $tra=0.0;
    my $tdec=0.0;
    ##
    foreach my $row (@$rowList) {
        print $pointsList "$npoints $row->{CENTER_X} $row->{CENTER_Y}\n";
	#$npoints++;
        ## MEH hack -- add width, height corners -- chip gap problem and need to add more points to try
        ## -- need to adjust RA for DEC and check 0/360, 90 boundary (dont assume coord to image lookup will do it)
        if ( ($row->{OPTION_MASK} & $PSTAMP_MULTI_OVERLAP_IMAGE) && !($row->{COORD_MASK} & $PSTAMP_RANGE_IN_PIXELS) ){
            foreach my $f (-1.1, 1.1, -1.5, 1.5, -2.0, 2.0, -2.5, 2.5) {
            	## +/-ra +/-dec corner
            	#printf $pointsList "$npoints %f %f\n", $row->{CENTER_X}+$row->{WIDTH}/2.0/3600.0, $row->{CENTER_Y}+$row->{HEIGHT}/2.0/3600.0;
	    	$tra = $row->{CENTER_X}+$row->{WIDTH}/$f/2.0/3600.0/cos($row->{CENTER_Y}/180.0*$pi);
	    	$tdec = $row->{CENTER_Y}+$row->{HEIGHT}/$f/2.0/3600.0;
            	$tra = ($tra>360.0) ? $tra-360.0 : $tra;
            	$tdec = ($tdec>90.0) ? 90.0-($tdec-90.0) : $tdec;
            	printf $pointsList "$npoints %f %f\n",$tra,$tdec;
            	## +/-ra -/+dec corner 
            	$tra = $row->{CENTER_X}+$row->{WIDTH}/$f/2.0/3600.0/cos($row->{CENTER_Y}/180.0*$pi);
           	$tdec = $row->{CENTER_Y}-$row->{HEIGHT}/$f/2.0/3600.0;
            	$tra = ($tra>360.0) ? $tra-360.0 : $tra;
           	$tdec = ($tdec>90.0) ? 90.0-($tdec-90.0) : $tdec;
           	printf $pointsList "$npoints %f %f\n",$tra,$tdec;
	    	## -/+ra side 
            	$tra = $row->{CENTER_X}-$row->{WIDTH}/$f/2.0/3600.0/cos($row->{CENTER_Y}/180.0*$pi);
            	$tdec = $row->{CENTER_Y};
            	$tra = ($tra>360.0) ? $tra-360.0 : $tra;
            	$tdec = ($tdec>90.0) ? 90.0-($tdec-90.0) : $tdec;
            	printf $pointsList "$npoints %f %f\n",$tra,$tdec;
            	## -/+dec side 
            	$tra = $row->{CENTER_X};
            	$tdec = $row->{CENTER_Y}-$row->{HEIGHT}/$f/2.0/3600.0;
            	$tra = ($tra>360.0) ? $tra-360.0 : $tra;
                $tdec = ($tdec>90.0) ? 90.0-($tdec-90.0) : $tdec;
                printf $pointsList "$npoints %f %f\n",$tra,$tdec;
            }
        }
	####
	$npoints++;
        # this gets overwitten if an overlapping image is found
        $row->{error_code} = $PSTAMP_NO_OVERLAP;
    }
    close $pointsList;

    if (($req_type eq "byid") or ($req_type eq "byexp")) {
        my ($last_tess_id, $tess_dir_abs, $astrom_file) = ("", "", "");
        foreach my $run (@$runList) {
            my $command = "$dvoImagesAtCoords -coords $pointsListName";
            if (($stage eq "chip") or ($stage eq "raw")) {
                # XXX: use file rule and handle cameras where the astrometry is solved at
                # the chip stage.
                my $cam_path_base = $run->{cam_path_base};
                if ($cam_path_base) {
                    $astrom_file = $run->{cam_path_base} . ".smf";
                } else {
                    if (! find_astrometry($ipprc, $imagedb, $run, $verbose) ) {
                        setErrorCodes($rowList, $PSTAMP_NOT_AVAILABLE);
                        next;
                    }
                    $cam_path_base = $run->{cam_path_base};
                    $astrom_file = $run->{astrom};
                }
                my $astrom_file_resolved = $ipprc->file_resolve($astrom_file);
                if (!$astrom_file_resolved) {
                    print STDERR "cannot resolve astrometry file: $astrom_file\n";
                    setErrorCodes($rowList, $PSTAMP_NOT_AVAILABLE);
                    next;
                }
                $command .= " -astrom $astrom_file_resolved";
            } else {
                my $tess_id = $run->{tess_id};
                if ($tess_id ne $last_tess_id) {
                    $tess_dir_abs = $ipprc->tessellation_catdir( $tess_id );
                    $tess_dir_abs = $ipprc->convert_filename_absolute( $tess_dir_abs );
                    $last_tess_id = $tess_id;
                }
                $command .= " -D CATDIR $tess_dir_abs"
            }
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                 run(command => $command, verbose => $dvo_verbose);
            unless ($success) {
                # don't fail if the program exited normally and exit status was PSTAMP_NO_OVERLAP
                #                     # That just means that the coordinate didn't match any image/skycell
                if (!WIFEXITED($error_code) || (WEXITSTATUS($error_code) ne $PSTAMP_NO_OVERLAP)) {
                    print STDERR @$stderr_buf;
                    my $rc = WIFEXITED($error_code) ? WEXITSTATUS($error_code) : $PS_EXIT_SYS_ERROR;
                    &my_die( "dvoImagesAtCoords failed: $rc", $rc);
                }
            }
            my %components;
            my @lines = split "\n", join "", @$stdout_buf;
            foreach my $line (@lines) {
                my ($ptnum, undef, undef, $component) = split " ", $line;
                my $ref = $components{$component};
                if (!$ref) {
                    $ref = $components{$component} = [];
                }
		#push @$ref, $ptnum;
                ## MEH hack -- only push ptnum if not already, so check if ptnum in list 
		push(@$ref, $ptnum) unless grep{$_ == $ptnum} @$ref;
		##
                my $row = $rowList->[$ptnum];
                # this row found a match
                $row->{error_code} = 0;
            }
            if ($verbose) {
                foreach my $c (keys %components) {
                    my $ref = $components{$c};
                    print "component $c contains: ";
                    foreach my $i (@$ref) {
                        print "$i ";
                    }
                    print "\n";
                }
            }
            # now find the images for this run
            foreach my $c (keys %components) {
                my $command;
                my $stage_id;
                if ($stage eq 'raw') {
                    $stage_id = $run->{exp_id};
                    $command = "$regtool -processedimfile -exp_id $stage_id -class_id $c";
                } elsif ($stage eq 'chip') {
                    $stage_id = $run->{chip_id};
                    $command = "$chiptool -processedimfile -chip_id $stage_id -class_id $c";
                } elsif ($stage eq 'warp') {
                    $stage_id = $run->{warp_id};
                    $command = "$warptool -warped -warp_id $stage_id -skycell_id $c";
                } elsif ($stage eq 'diff') {
                    $stage_id = $run->{diff_id};
                    $command = "$difftool -diffskyfile -diff_id $stage_id -skycell_id $c";
                }
                $command .= " -dbname $imagedb";
                my $images = runToolAndParse($command, $verbose);
                if (!defined $images) {
                    print "No  $c found for $stage ${stage}_id $stage_id\n";
                    next;
                }
                if (scalar @$images != 1) {
                    my $num_images = scalar @$images;
                    &my_die ("unexpected number of images returned: $num_images\n", $PS_EXIT_PROG_ERROR);
                }
                my $image = $images->[0];
                    
                my $ref = $components{$c};
                $image->{row_index} = $ref;
                if (($stage eq "raw") or ($stage eq 'chip')) {
                    $image->{astrom} = $astrom_file;
                    $image->{cam_id} = $run->{cam_id};
                    $image->{cam_path_base} = $run->{cam_path_base};
                }
                push @$results, $image;
            }
        }
    } else {
        &my_die ("sorry not done with $req_type\n", $PS_EXIT_PROG_ERROR);
    }
    return $results;
}

sub selectComponentsByName {
    my $ipprc = shift;
    my $imagedb = shift;
    my $req_type = shift;
    my $stage  = shift;
    my $component  = shift;
    my $rowList = shift;
    my $runList = shift;
    my $verbose = shift;
    my $results = [];

    $component = "" if $component and lc($component) eq 'all';

    # all runs match all rows so they can share a common row_index
    # XXX: I'm not sure we ever actually get here with more than one
    # row
    my $numRows = scalar @$rowList;
    my $row_index = [];
    for (my $i=0; $i < $numRows; $i++) {
        push @$row_index, $i;
    }
    
    # XXX: Why did I make this restriction? I added byskycell
    if (($req_type eq "byid") or ($req_type eq "byexp") or ($req_type eq 'byskycell')) {
        my ($last_tess_id, $tess_dir_abs, $astrom_file) = ("", "", "");
        foreach my $run (@$runList) {
            # now find the images for this run
            #foreach my $c (keys %components) {
            my $command;
            my $stage_id;
            if ($stage eq 'raw') {
                $stage_id = $run->{exp_id};
                $command = "$regtool -processedimfile -exp_id $stage_id";
                $command .= " -class_id $component" if $component;
            } elsif ($stage eq 'chip') {
                $stage_id = $run->{chip_id};
                $command = "$chiptool -processedimfile -chip_id $stage_id";
                $command .= " -class_id $component" if $component;
            } elsif ($stage eq 'warp') {
                $stage_id = $run->{warp_id};
                $command = "$warptool -warped -warp_id $stage_id";
                $command .= " -skycell_id $component" if $component;
            }
            $command .= " -dbname $imagedb";
            my $images = runToolAndParse($command, $verbose);
            if (!defined $images) {
                print "No  $component found for $stage ${stage}_id $stage_id\n";
                next;
            }
            foreach my $image (@$images) {
                $image->{row_index} = $row_index;
                if (($stage eq "raw") or ($stage eq 'chip')) {
                    $image->{cam_id} = $run->{cam_id};
                    $image->{cam_path_base} = $run->{cam_path_base};
                }
                push @$results, $image;
            }
        }
    } else {
        &my_die ("selectComponentsByName not supported for REQ_TYPE: $req_type\n", $PS_EXIT_PROG_ERROR);
    }
    return $results;
}

sub filterRuns {
    my $stage      = shift;
    my $drop_duplicate_runs = shift;
    my $need_magic = shift;
    my $inputs     = shift;
    my $inverse     = shift;
    my $verbose    = shift;

    if ($inputs and (scalar @$inputs) == 1)  {
        # one run nothing to do
        return $inputs;
    }

    my $output = [];

    return $output if (!$inputs or scalar @$inputs == 0);

    my $id_name = $stage . "_id";
    $id_name = 'exp_id' if $stage eq 'raw';

    # input list is "order by exp_id, run_id DESC"   run_id is one of (chip_id, warp_id, diff_id)
    print "Starting filterRuns\n";
    my $last_exp_id = 0;
    my $last_run_id = 0;
    my $printed = 0;
    foreach my $input (@$inputs) {
        my $exp_id;
        if ($stage ne "diff") {
            $exp_id = $input->{exp_id};
        } elsif (!$inverse) {
            $exp_id = $input->{exp_id_1};
            $input->{exp_id} = $exp_id;
        } else {
            $exp_id = $input->{exp_id_2};
            $input->{exp_id} = $exp_id;
        }
        my $run_id = $input->{$id_name};
        my $magicked = $input->{magicked};  # this will be either stageRun.magicked or stage%file.magicked
        my $state = $input->{state};
        $state = $input->{data_state} if !defined $state;

        # can't process run in these states
        # XXX: also goto_purged or goto_scrubbed or drop
        if (($state eq 'new') or ($state eq 'purged') or ($state eq 'scrubbed')) {
            print "skipping ${stage}Run $run_id for exp_id $exp_id in state $state\n";
            next;
        }

        # $printed = 1 if (($exp_id == $last_exp_id) and ($run_id == $last_run_id));

        # skip if we need magicked run and this one has never been magicked
        if ($need_magic and !$magicked) {
            print "skipping ${stage}Run $run_id for exp_id $exp_id not magicked\n" if !$printed;
            next;
        }

        if (($exp_id == $last_exp_id) and ($drop_duplicate_runs || ($run_id != $last_run_id))) {
            print "Skipping duplicate ${stage}Run $run_id for $exp_id\n" if !$printed;
            next;
        }
        print "Keeping ${stage}Run $run_id for $exp_id\n";
        push @$output, $input;
        $last_exp_id = $exp_id;
        $last_run_id = $run_id;
        $printed = 0;
    }

    my $num_runs = scalar @$output;
    print "filterRuns returning $num_runs ${stage}Run\n";

    return $output;
}

# run a command that produces metadata output and parse the results into an array of objects
sub runToolAndParse {
    my $command = shift;
    my $verbose = shift;

    my ($program) = split " ", $command;
    $program = basename($program);

    print "Running $command\n" if !$verbose;
    my $start_tool = gettimeofday();
    # run the command and parse the output

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
    unless ($success) {
        # not sure if we should die here
        print STDERR @$stderr_buf;
        return undef;
    }

    my $now = gettimeofday();
    my $dtime_tool = $now - $start_tool;
    # XXX: shouldn't this be if $verbose?
    print "Time to run $program: $dtime_tool\n" ; # if $verbose;

    my $buf = join "", @$stdout_buf;
    if (!$buf) {
        return undef;
    }

    my $start_parse = gettimeofday();

    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $results = parse_md_fast($mdcParser, $buf)
        or die ("Unable to parse metadata config doc");

    my $dtime_parse = gettimeofday() - $start_parse;
    # XXX: shouldn't this be if $verbose?
    print "Time to parse results from $program: $dtime_parse\n" ; # if $verbose;

    return $results;
}

sub findTools {
    return if ($regtool);

    my $missing_tools;
    $regtool = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
    $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
    $camtool = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
    $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
    $difftool = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
    $stacktool = can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
    $pstamptool = can_run('pstamptool') or (warn "Can't find pstamptool" and $missing_tools = 1);
    $dvoImagesAtCoords = can_run('dvoImagesAtCoords') or (warn "Can't find dvoImagesAtCoords" 
                                                                and $missing_tools = 1);
    $releasetool = can_run('releasetool') or (warn "Can't find releasetool" 
                                                                and $missing_tools = 1);
    $whichimage = can_run('whichimage') or (warn "Can't find whichimage" and $missing_tools = 1);
    $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
    if ($missing_tools) {
        warn("Can't find required tools.");
        exit ($PS_EXIT_CONFIG_ERROR);
    }
}

# add a row_index array to a set of images with entries for each of the rows in a list
sub setRowRefs {
    my $rowList = shift;
    my $images  = shift;

    my $row_index = [];
    for (my $i = 0; $i < scalar @$rowList; $i++) {
        push @$row_index, $i;
    }
    foreach my $image (@$images) {
        $image->{row_index} = $row_index;
    }
}
# set error_code for an array of rows to a given value
sub setErrorCodes {
    my $rowList = shift;
    my $code = shift;
    foreach my $row (@$rowList) {
        $row->{error_code} = $code;
    }
}

sub load_data_groups {
    my $verbose = shift;
    my $command = "$ppConfigDump -dump-recipe PSTAMP -";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        die("Unable to perform ppConfigDump: $error_code");
    }
    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        die ("Unable to parse metadata config doc");

    my $data_group_string = "";
    foreach my $item (@$metadata) {
        my $tag = $item->{name};
        next if $tag ne "DATA_GROUP";
        my $dg = $item->{value};
#        print "adding $dg to list of default data groups\n";
        $data_group_string .= " -data_group $dg";
    }
    return $data_group_string;
}

# get user supplied release paramters and the asssociated default_tess_id.
# If release parameters are not supplied set defaults.
sub get_release_info {
    my $row = shift;

    my $survey;
    my $release_name = $row->{IPP_RELEASE};
    if (isnull($release_name)) {
        $release_name = "";
        # no release check for survey
        $survey = isnull($row->{SURVEY_NAME}) ? "" : $row->{SURVEY_NAME};
    }

    # XXX: From here down this function is currently very PS1 specific and depends on 
    # conventions being used for release names.
    # The comments following the lines  marked 'XXX to improve' give suggestions on
    # how these hacks can be cleaned up. 

    my $tess_id = $row->{TESS_ID};
    if (!$release_name and !$survey) { 
        # no release or survey. If user does not supply
        # a data group use a default survey
        my $req_type = $row->{REQ_TYPE};
        if ($req_type eq "bycoord" or $req_type eq 'byskycell') {
            # USING 3PI for bycoord and byskycell requests
            # XXX to improve
            # get default survey from project
            my $data_group = $row->{DATA_GROUP};
            if (isnull($data_group) and isnull($tess_id)) {
                print "No release information supplied for this row. Defaulting to 3PI survey\n";
                $survey = '3PI';
            }
        }
    }

    # if survey was not supplied, guess it from the release_name. This is used below
    # to choose the default_tess_id
    if (!$survey) {
        if ($release_name) {
            # XXX: to improve 
            # get survey from ippRelease(release_name) in database. Would need to cache
            # such results
            if ($release_name =~ "^3PI") {
                $survey = '3PI';
            } elsif ($release_name =~ "^MD") {
                $survey = substr($release_name, 0, 4);
            } elsif ($release_name =~ "^SSS") {
                $survey = 'SSS';
            } elsif ($release_name =~ "^SAS") {
                $survey = 'SAS';
            } else {
                # we don't know
                print "Can't guess survey for $release_name\n";
            }
        } elsif (!isnull($tess_id)) {
            # finally if it is a MD tessellation use the MD field as the survey
            # (This will also work for M31 and STS when that data is released.
            if ($tess_id =~ "^MD") {
                $survey = substr($tess_id, 0, 4);
            }
        }
    }

    # choose default tess_id based on the survey

    # Note: Setting a default_tess_id effectively disables lookup bycoord requests
    # for previous tessellations unless the user supplies the tess_id (which is
    # applied by the caller to this function).
    # bills has decided that this is desirable behaviour since most everything has
    # been processed on a V3 tess.
    my $default_tess_id;
    if ($survey) {
        # XXX: to improve
        # Put default tess_id in survey table.
        # note users will need to supply tessellation to get at 3pi skycells
        # in the cnp tessellation. Hmm. Maybe we need to be clever and look
        # at declination or perhaps allow default_tess_id to be an array.
        if ($survey eq '3PI') {
            $default_tess_id = ['RINGS.V3', 'CNP.V3'];
        } elsif ($survey eq 'SSS' or $survey eq 'SAS') {
            $default_tess_id = 'RINGS.V3';
        } elsif ($survey =~ 'MD') {
            $default_tess_id = $survey . '.V3';
        }
    }
            
    # leave the returned survey blank if release_name was supplied
    # XXX: Why? Answer: this is how this function always behaved and I'm not sure if the
    # callers depend on that behavior.
    $survey = "" if $release_name;

    return ($release_name, $survey, $default_tess_id);
}

sub getDiffMode {
    my $run_type = shift;
    my $diff_mode;
    if (!isnull($run_type)) {
        $run_type = lc($run_type);
        # XXX: these diff_mode values should be defined somewhere else.
        if ($run_type eq 'warp_warp') {
            $diff_mode = 1;
        } elsif ($run_type eq 'warp_stack') {
            $diff_mode = 2;
        } elsif ($run_type eq 'stack_stack') {
            $diff_mode = 4;
        } else {
            print STDERR "Ingnoring unrecognized diff RUN_TYPE : $run_type\n";
        }
    }
    return $diff_mode;
}

sub setErrorCodesForRows {
    my $rowList = shift;
    my $error_code = shift;

    foreach my $row (@$rowList) {
        $row->{error_code} = $error_code;
    }

}

# Convert from skycell_id to projection_cell
# This code is not particularly elegant but I think that it matches the SQL used
# to create stackAssocations does. 

sub skycell_id_to_projection_cell {
    my $skycell_id = shift;

    # split skycell_id into '.' separated components
    my @strs = split '\.', $skycell_id;

    my $num = scalar @strs;

    # projection_cell is 'skycell_id' plus all .num values except the last one
    my $projection_cell = $strs[0];
    for (my $i=1; $i < $num - 1; $i++) {
        $projection_cell .= ".$strs[$i]";
    }

    return $projection_cell;
}

# lookup_skycells_fast() 
# Lookup using a "skycell server" process which is implemented by running dvoImagesAtCoords in "server" mode.
# Repeated calls to this function will reuse an existing process. This avoids having to loading the
# tessellation over and over again for each skycell lookup.
# When this parse process exists, the pipes are closed which causes the skycell server to exit.

my $scs_tess_id;    # tess_id for existing scs (skycell server) (if any)
my $scs_out;        # reference filehandle for sending data to the scs
my $scs_in;         # filehandle for response from scs

sub lookup_skycells_fast {
    my ($ipprc, $results, $tess_id, $ra, $dec, $verbose) = @_;

    # see if we need to start the scs (skycell server)

    my $very_verbose = 0;

    if (!$scs_tess_id or ($scs_tess_id ne $tess_id)) {
        if ($scs_tess_id) {
            # tess_id has changed, close down existing scs
            close $scs_out;
            $scs_out = undef;
            close $scs_in;
            $scs_in = undef;
            $scs_tess_id = undef;
        }

        # convert tess_id to a directory name
        my $tess_dir = $ipprc->tessellation_catdir( $tess_id );
        unless ($tess_dir) {
            print STDERR "Unrecognized tess_id: $tess_id\n";
            return 0;
        }

        # convert tess_dir to an absolute directory name
        my $tess_dir_resolved = $ipprc->convert_filename_absolute( $tess_dir );
        unless ($tess_dir_resolved and -d $tess_dir_resolved) {
            print STDERR "failed to resolve tessellation directory for $tess_id\n";
            return 0;
        }

        # start an scs
        print "Starting skycell server for $tess_dir_resolved.\n";

        # create pipes for communicating with the server
        pipe $scs_in, SERVER_WRITER;
        pipe SERVER_READER, $scs_out;

        # set our output to be unbuffered
        $scs_out->autoflush(1);

        # FORK
        my $scs_pid = fork();

        if (!$scs_pid) {
            unless (defined $scs_pid) {
                # This is still in parent process. No joy.
                print STDERR "fork of skycell server failed: $!";
                return 0;
            }

            # This code is running in the child process - the skycell server
            # Close file handles for the parent's ends of the pipes
            close $scs_in;
            close $scs_out;
            # close stdio filehandles. we are about to redirect them
            # (STDERR stays open pointing to the parser log)
            close STDIN;
            close STDOUT;

            # redirect our stdio file handles to the proper end of the pipes
            open (STDIN,  "<&SERVER_READER")   or die "\nSCS: failed to redirect stdin";
            open (STDOUT, ">>&SERVER_WRITER")  or die "\nSCS: failed to redirect stdout";

            # All set. Now exec dvoImagesAtCoords

            my $command = "$dvoImagesAtCoords -D CATDIR $tess_dir_resolved -coords -";

            print STDERR "SCS: execing $command\n" if $very_verbose;

            unless(exec $command) {
                # note parent will notice that we are gone due to the pipes getting broken
                # when this child dies.
                die "SCS: failed to exec $command";
            }
        }

        # This code is running in the parent process (the parser).
        # We have succesfully launched the scs process.
        # Close file handles for the scs' end of the pipes.
        close SERVER_WRITER;
        close SERVER_READER;

        # remember the tess_id 
        $scs_tess_id = $tess_id;
    }

    # send coordinates to the skycell server and wait for the results 
    # (which come nearly instantly which is the point of doing this)

    # If pipes to the skycell server die, don't abort. 
    # Our reads and writes detect errors and act sensibly.
    local $SIG{PIPE} = 'IGNORE';

    # write the coordinates to the pipe
    my $write_ok = print $scs_out "1 $ra $dec\n";
    if (!$write_ok) {
        # server process probably didn't start properly or died. Fail.
        # Caller will attempt to use the conventional method.
        print STDERR "write to skycell server failed. Error is $!\n";
        return 0;
    }

    print STDERR "  sent coordinates to skycell server\n" if $very_verbose;

    my $received_done = 0;

    # Wait for response. If pipe breaks the read returns with no data.
    while (my $line = <$scs_in>) {
        chomp $line;
        if ($line eq 'DONE') {
            # No more results for these coordinates.
            $received_done = 1;
            print STDERR "    received DONE\n" if $very_verbose;
            last;
        }
        print STDERR "    received $line\n" if $very_verbose;

        # The caller expects the output to be in the format used by whichimage
        # which omits the point number and includes the tess_id.
        my ($ptnum, $raout, $decout, $skycell_id) = split " ", $line;

        my $out = "$raout $decout $tess_id $skycell_id";

        # print the result to the log
        print "$out\n" if $verbose;

        push @$results, $out;
    }

    print STDERR "Out of wait for responses loop\n" if $very_verbose;

    if (!$received_done) {
        # something has gone wrong.
        print STDERR "Read loop exited without receiving DONE.\n";

        die "BAILING out" if $very_verbose;

        # forget about the existing server instance
        $scs_tess_id = undef;
        close $scs_out;
        close $scs_in;

        # drop any results received so far
        @$results = ();
        return 0;
    }

    return 1;
}

        

sub my_die
{
    my $msg = shift;
    my $fault = shift;

    carp $msg;

    # we don't fault the request here pstamp_parser_run.pl handles that if necessary

    return $fault;
}
1;
