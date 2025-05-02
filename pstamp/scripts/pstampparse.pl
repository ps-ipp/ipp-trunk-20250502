#!/bin/env perl
###
### pstampparse.pl
###
###     Run the postage stamp request parser for a given request id
###

use warnings;
use strict;

use Sys::Hostname;
use Getopt::Long qw( GetOptions );
use PS::IPP::PStamp::RequestFile qw( :standard );
use PS::IPP::PStamp::Job qw( :standard );
use File::Temp qw(tempfile);
use File::Basename qw(basename);
use Carp;
use POSIX;
use Time::HiRes qw(gettimeofday);
use Time::Local;

my $verbose;
my $dbname;
my $dbserver;
my $req_id;
my $request_file_name;
my $mode = "list_uri";
my $outdir;
my $product;
my $label = "";
my $save_temps;
my $no_update;
my $dest_requires_magic;
my $dump_params = 0;
my $new_email;

my %accessLevelCache;

# set this to true to disable update processing
my $no_updates_allowed = 0;

GetOptions(
    'file=s'    =>  \$request_file_name,
    'req_id=s'  =>  \$req_id,
    'outdir=s'  =>  \$outdir,
    'product=s' =>  \$product,
    'label=s'   =>  \$label,
    'new_email=s' => \$new_email,
    'mode=s'    =>  \$mode,
    'need_magic'=>  \$dest_requires_magic,
    'dbname=s'  =>  \$dbname,
    'dbserver=s'=>  \$dbserver,
    'verbose'   =>  \$verbose,
    'save-temps'=>  \$save_temps,
    'no-update' =>  \$no_update,
    'dump-params' => \$dump_params,
);

die "invalid mode '$mode'" unless ($mode eq "list_uri") or ($mode eq "queue_job");
die "--file is required"   unless defined($request_file_name);
die "req_id is required"  unless defined $req_id;
if ($mode ne 'list_uri') {
    die "outdir is required"  if !$outdir;
} else {
    $outdir = "somewhere" if !$outdir
}

use IPC::Cmd 0.36 qw( can_run run );

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw( :standard );
my $ipprc = PS::IPP::Config->new(); # IPP Configuration

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $pstampdump  = can_run('pstampdump') or (warn "Can't find pstampdump" and $missing_tools = 1);
my $fields      = can_run('fields') or (warn "Can't find fields" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

# just deal with these arguments once and for all
$pstamptool .= " -dbname $dbname" if $dbname;
$pstamptool .= " -dbserver $dbserver" if $dbserver;

# If $mode is not queue_job we are using a debugging mode
# do not update the database
$no_update = 1 if $mode ne "queue_job";

$product = 'NULL' unless $product;


# look up some defaults
my $defaultAccessLevel = metadataLookupS32($ipprc->{_siteConfig}, 'PSTAMP_DEFAULT_ACCESS_LEVEL');
if (!defined $defaultAccessLevel) {
    warn("cannot find 'PSTAMP_DEFAULT_ACCESS_LEVEL' in site config");
    exit ($PS_EXIT_CONFIG_ERROR);
}

# look up the default data store product.
my $defaultDSProduct = metadataLookupStr($ipprc->{_siteConfig}, 'PSTAMP_DATA_STORE_PRODUCT');
if (!defined $defaultDSProduct) {
    # Don't panic yet that it's not defined.
    #  exit ($PS_EXIT_CONFIG_ERROR);
    $defaultDSProduct = ''; # set to empty string to simplify some comparisions below
    print STDERR "Warning PSTAMP_DATA_STORE_PRODUCT not found in the site config.\n" if  $product eq 'NULL';
}

my $productIsDefault = 0;
if ($product and $defaultDSProduct) {
    $productIsDefault = ($product eq $defaultDSProduct);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

#
# Read the keywords from the extension header
#
my $fields_output;
{
    my $command = "echo $request_file_name | $fields -x 0 EXTNAME EXTVER REQ_NAME ACTION EMAIL";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    # note fields doesn't return zero when it succeeds.
    $fields_output = join "", @$stdout_buf;
}
my (undef, $extname, $extver, $req_name, $action, $email) = split " ", $fields_output;

# make sure the file contains what we are expecting
# pstamp_parser_run.pl would not have run this program unless the request file was ok
my_die("$request_file_name does not contain EXTNAME\n", $PS_EXIT_PROG_ERROR) if !$extname;
my_die("$request_file_name is not a PS1_PS_REQUEST\n", $PS_EXIT_PROG_ERROR) if $extname ne "PS1_PS_REQUEST";
my_die("REQ_NAME not found in $request_file_name\n", $PS_EXIT_PROG_ERROR)  if (!$req_name);
my_die("wrong EXTVER $extver found in $request_file_name\n", $PS_EXIT_PROG_ERROR) if ($extver ne "1" and $extver ne "2");

if ($extver eq "1") {
    print STDERR "Version 1 postage stamp requests are no longer accepted. Please update your request tables to version 2 format.\n";
    print STDERR "Note that EMAIL will not be optional\n";

    # if need be we can hack Peter Veres' email in here if label = 'WEB.UP' and req_name like 'Peter_Veres%'

} elsif (!$email) {
    my_die("ERROR: Required parameter EMAIL not found in request file header.\n", $PSTAMP_INVALID_REQUEST) 
}


    

if ($extver >= 2) {
    # We have a version 2 file. Require that the new keywords be supplied. 
    my_die("action not supplied in version $extver request file $request_file_name\n", $PSTAMP_INVALID_REQUEST) 
        unless defined $action;

    my_die("invalid action: $action supplied in version $extver request file $request_file_name\n",
        $PSTAMP_INVALID_REQUEST)
        unless (uc($action) eq 'PROCESS' or uc($action) eq 'PREVIEW');

    my_die("email not supplied in version $extver request file $request_file_name\n", $PSTAMP_INVALID_REQUEST)
        unless $email;
} else {
    # for version 1 file the action is process and email is not used
    $action = 'PROCESS';
    $email  = 'null';
}

print "Request Header Keywords EXTVER: $extver REQ_NAME: $req_name ACTION: $action EMAIL: $email\n";

if ($new_email) {
    # this is primarily for ease in testing
    print "Setting email to command line provided value $new_email\n";
    $email = $new_email
}

# check for duplicate request name
my $duplicate_req_name = 0;
if ($req_id and !$no_update) {
    my $command = "$pstamptool -listreq  -name $req_name -not_req_id $req_id";
    # no verbose so that error message about request not found doesn't appear in parse_error.txt
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 0);
    my $exitStatus = $error_code >> 8;
    if ($success) {
        # -listreq succeeded there is already a request in the database with this name
        print STDERR "REQ_NAME $req_name has already been used\n";
        insertFakeJobForRow(undef, 0, $PSTAMP_DUP_REQUEST);
        $duplicate_req_name = 1;
        my $datestr = strftime "%Y%m%d%H%M%S.$req_id", gmtime;
        $req_name = "ERROR.$datestr";
        #exit 0;
    }
}



my $label_changed = 0;
if (0) {
# Adjust the label for requests coming in over the web upload interface
# XXX: do this later based on the user information
# We now use the access tabels to set the label.
if ($label and $label eq "WEB.UP") {
    my $lcname = lc($req_name);
    if ($lcname =~ /pitt/) {
        $label = "PITT";
        $label_changed = 1;
    } elsif ($lcname =~ /cfa/) {
        $label = "CFA";
        $label_changed = 1;
    } elsif ($lcname =~ /durham/) {
        $label = "DURHAM";
        $label_changed = 1;
    } elsif ($lcname =~ /qub/) {
        $label = "QUB";
        $label_changed = 1;
    } elsif ($lcname =~ /pypstamp/) {
        $label = "PY";
        $label_changed = 1;
    } elsif ($lcname =~ /ncu/) {
    	$label = 'NCU';
	$label_changed = 1;
    } elsif (($lcname =~ /mpe_/) or ($lcname =~ /sd_/) or ($lcname =~ /^jk/)) {
    	$label = 'MPE';
	$label_changed = 1;
    }
    print "Setting label for $req_name to $label\n" if $label_changed;
}
}

# based on the user's email address get the accessLevel, default label and data store product
my $productForUser;
my $labelForUser = $label;  # XXX: for now I pass the current label for use in hacking permissions for V1 requests.

my $accessLevel = getUsersAccessLevel($email, \$labelForUser, \$productForUser);
if ($accessLevel < 0) {
    print STDERR "Data access is forbidden for $email.\n";
    insertFakeJobForRow(undef, 0, $PSTAMP_NOT_AUTHORIZED);
}

# if the request came through the request table upload channel, change it to the user's specific label
# if any is known. Do we want to do this for any other channels?
# requests from data store will have those values.
if ($label and ($label eq 'WEB.UP' or $label eq 'WEB')) {
    if ($labelForUser) {
        $label = $labelForUser;
        $label_changed = 1;
    }
}

# If the product is the system default, set it to the product for the user@domain. That way 
# if the request came through a data store that target will be used.
# We really only need to redirect the results with greater access level than the default.
# note that for most users productForUser will be undef so the default will be used.
if ($productIsDefault or $product eq 'NULL') {
    if ($productForUser) {
        $product = $productForUser;
    }
} else {
    # if we don't have a product use default
    if ($product eq 'NULL') {
        $product = $defaultDSProduct;
    }
}

if (!$product or $product eq 'NULL') {
    # this will only happen if the site.config variable is missing.
    my_die("No product found.\n", $PS_EXIT_CONFIG_ERROR);
}

{
    # update the request's row with the new parameters.
    # The requset name will be used to set the output fileset name in the output data store.
    my $command = "$pstamptool -updatereq -req_id $req_id  -set_name $req_name";
    $command .= " -set_username $email" if $email ne 'null';
    $command .= " -set_outProduct $product";
    $command .= " -set_label $label" if $label_changed;
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            my_die("$command failed", $PS_EXIT_UNKNOWN_ERROR);
        }
    } else {
        print "skipping $command\n";
    }
}

# in these two error conditions we are done. The invoking script will finish up.
if ($duplicate_req_name) {
    exit 0;
}

if ($accessLevel < 0) {
    exit 0;
}


#
# now convert the request table to an array of metadatas
#

my $rows;
{
    my $start_request_file = gettimeofday();
    my $command = "$pstampdump $request_file_name";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        print STDERR @$stderr_buf;
    }
    if (@$stdout_buf) {
        $rows = parse_md_fast($mdcParser, join "", @$stdout_buf);
    }
    my $dtime_request_file = gettimeofday() - $start_request_file;
    print "Time to read and parse request file: $dtime_request_file\n";
}

my $nRows = $rows ? scalar @$rows : 0;
print "\n$nRows rows read from request file\n";

unless ($nRows) {
    # pstamp_job_run was invoked so the request file must have contained a valid header
    # a request file with no rows is invalid.
    # The print above will let the log file know a bit more.
    # Insert a faulted fake job and exit this program successfully.
    print STDERR "Invalid request file.\n";
    insertFakeJobForRow(undef, 0, $PSTAMP_INVALID_REQUEST);
    exit 0;
}


# if label is for one of the high priority channels, watch for big requests
my $watch_for_big_requests = (!($label =~ /BIG/) and ($label =~ /PSI/ or $label =~ /WEB/));

# XXX: these should be in a configuration file somewhere not hard coded
my $big_limit = 100;
my $job_big_limit = 400;

if ($watch_for_big_requests and $nRows > $big_limit) {
    $label = change_to_lower_priority_label($label);
    $watch_for_big_requests = 0;
}

my $num_jobs = 0;


foreach my $row (@$rows) {

    # validate the paramaters
    if (!checkRow($row)) {
        # when it enconters an error checkRow adds a fake job with an appropriate error code to the database
        $num_jobs++;
        next;
    }
    # initialize counter for "job number"
    $row->{job_num} = 0;
    $row->{error_code} = 0;
    $row->{accessLevel} = $accessLevel;

    $num_jobs += processRow($action, $row);

    # see whether number of jobs limit for high priority request was exceeded
    if ($watch_for_big_requests and $num_jobs > $job_big_limit) {
        $label = change_to_lower_priority_label($label);
        $watch_for_big_requests = 0;
    }
}

if (($action eq 'LIST' or $mode eq "queue_job") and ($num_jobs eq 0)) {
    # this should not happen. The processRow is required to insert a fake job for any
    # rows that did not yield any jobs.
    print STDERR "ERROR: zero jobs created for $req_name\n";
    insertFakeJobForRow(undef, 0, $PS_EXIT_PROG_ERROR);
}

exit 0;

# end of main function

sub change_to_lower_priority_label {
    my $label = shift;
    my $old_label = $label;
    $label = ($label =~ /WEB/) ? 'WEB.BIG' : 'PSI.BIG';
    print "\nChanging label for big $old_label request to $label\n";

    my $command = "$pstamptool -updatereq -req_id $req_id  -set_label $label";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        my_die("$command failed", $PS_EXIT_UNKNOWN_ERROR);
    }
    return $label;
}

sub checkRow {
        
    my $row = shift;

    # check validity of the paramters in a request specification
    # If we encounter an error for a particular row add a job with the proper fault code.
    # also adjust some paramterers like filter and set defaults for some others

    my $stage = $row->{IMG_TYPE};

    my $rownum   = $row->{ROWNUM};
    if (!validID($rownum)) {
	$rownum = 'NULL' if !defined $rownum;
        print STDERR "$rownum is not a valid ROWNUM\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }
    my $job_type = $row->{JOB_TYPE};
    if (!defined $job_type || (($job_type ne "stamp") and ($job_type ne "get_image"))) {
    	$job_type = 'NULL' if !defined $job_type;
        print STDERR "$job_type is not a valid JOB_TYPE\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }
    
    my $req_type = $row->{REQ_TYPE};
    if (($req_type ne "byid") and ($req_type ne "bycoord") and ($req_type ne "byexp") and
        ($req_type ne "byskycell") and ($req_type ne "bydiff")) {
	$req_type = 'NULL' if !defined $req_type;
        print STDERR "$req_type is not a valid REQ_TYPE\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }
    if ($job_type eq 'get_image') {
        # get_image jobs are quite expensive in terms of space so we are currently restricting them
        unless ($req_type eq 'byid' or $req_type eq 'byexp' 
            or ($req_type eq 'byskycell' and $stage eq 'stack')
            or ($stage eq 'stack_summary')) {
            print STDERR "REQ_TYPE must be 'byid' or 'byexp' JOB_TYPE 'get_image' for IMG_TYPE $stage\n";
            insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
            return 0;
        }
    }

    my $component = $row->{COMPONENT};
    if (!defined $component or (lc($component) eq "null") or (lc($component) eq "all")) {
        if ($job_type eq 'get_image' and ! ($stage eq 'stack' or $stage eq 'stack_summary')) {
            $row->{COMPONENT} = 'all';
        } else {
            $row->{COMPONENT} = $component = "";
        }
    }
    $row->{TESS_ID} = "" if !defined $row->{TESS_ID};

    my $filter  = $row->{REQFILT};
    if ($filter) {
        if (length($filter) == 1) {
            # allow single character filter cuts to work
            $row->{REQFILT} .= '%';
        }
    }
    my $mjd_min = $row->{MJD_MIN};
    if (defined($mjd_min) and !validNumber($mjd_min)) {
        print STDERR "$mjd_min is not a valid MJD_MIN\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }
    my $mjd_max = $row->{MJD_MAX};
    if (defined($mjd_max) and !validNumber($mjd_max)) {
        print STDERR "$mjd_max is not a valid MJD_MAX\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }

    my $fwhm_min = $row->{FWHM_MIN};
    if (!defined $fwhm_min) {
        $row->{FWHM_MIN} = 0;
    } elsif (!validNumber($fwhm_min)) {
        print STDERR "$fwhm_min is not a valid FWHM_MIN\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }
    my $fwhm_max = $row->{FWHM_MAX};
    if (!defined $fwhm_max) {
        $row->{FWHM_MAX} = 0;
    } elsif (!validNumber($fwhm_max)) {
        print STDERR "$fwhm_max is not a valid FWHM_MAX\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }

    my $data_group = $row->{DATA_GROUP};
    if (!defined $data_group) {
        # backwards compatability hook
        $data_group = $row->{LABEL};
        $data_group = "null" if !defined $data_group;
        $row->{DATA_GROUP} = $data_group;
    }
        
    # req_finish doesn't work if bit zero of option mask is not set;
    $row->{OPTION_MASK} |= 1;

    my $option_mask= $row->{OPTION_MASK};
    my $inverse = ($option_mask & $PSTAMP_SELECT_INVERSE) ? 1 : 0;
    $row->{inverse} = $inverse;
    my $unconvolved = ($option_mask & $PSTAMP_SELECT_UNCONV) ? 1 : 0;
    $row->{unconvolved} = $unconvolved;

    my $skycenter = $row->{skycenter} = ! ($row->{COORD_MASK} & $PSTAMP_CENTER_IN_PIXELS);

    my $wholefile = 0;
    if (($row->{WIDTH} == 0 && $row->{HEIGHT} == 0) ||
       (!$skycenter && $row->{CENTER_X} == 0 && $row->{CENTER_Y} == 0)) {
        # Secret code for returning the whole file
        $wholefile = 1;
    }

    if (!$skycenter and !$wholefile and !$component and $stage ne 'stack' and !($option_mask & $PSTAMP_USE_IMFILE_ID)){
        print STDERR "COMPONENT must be specified for pixel coordinate ROI center\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }

    if (!check_image_type($stage)) {
        print STDERR "invalid IMG_TYPE for row $rownum\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }

    if ((($job_type eq "stamp") or ($req_type eq "bycoord")) and ! validROI($row)) {
        print STDERR "invalid ROI for row $rownum\n";
        insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
        return 0;
    }

    if (($req_type eq "byexp") and ($stage eq "stack" or $stage eq 'stack_summary')) {
        print STDERR "byexp not implemented for $stage stage. row: $rownum\n";
        insertFakeJobForRow($row, 1, $PSTAMP_NOT_IMPLEMENTED);
        return 0;
    }

    if ($req_type eq "bycoord") {
        if (!$skycenter) {
            print STDERR "center must be specified in sky coordintes for bycoord";
            insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
            return 0;
        }
    }

    if (($req_type eq "byid") or ($req_type eq "bydiff")) {
        if (!validID($row->{ID})) {
            print STDERR "ID must be a positive integer for req_type $req_type\n";
            insertFakeJobForRow($row, 1, $PSTAMP_INVALID_REQUEST);
            return 0
        }
    }


    return 1;
}

sub list_targets {
    my $rowList = shift;
    my $num_jobs = 0;

    $num_jobs = 1;
    return $num_jobs;
}

sub processRow {
    my $action  = shift;
    my $row     = shift;

    my $num_jobs = 0;

    my $project  = $row->{PROJECT};

    # note: resolve_project avoids running pstamptool every time by remembering the
    # last project resolved
    my $proj_hash = resolve_project($ipprc, $project, $dbname, $dbserver);
    if (!$proj_hash) {
        insertFakeJobForRow($row, 1, $PSTAMP_UNKNOWN_PROJECT);
        $num_jobs++;
        return $num_jobs;
    }
    my $image_db   = $proj_hash->{dbname};
    my $camera     = $proj_hash->{camera};

    my $req_type    = $row->{REQ_TYPE};
    my $rownum      = $row->{ROWNUM};

    my $mjd_max;
    my $adjustedDateCuts = adjustDateCuts($row, \$mjd_max);
    if ($adjustedDateCuts and $mjd_max < 0) {
        print STDERR "User is not authorized for data from this project.\n";
        insertFakeJobForRow($row, 1, $PSTAMP_NOT_AUTHORIZED);
        $num_jobs++;
        return $num_jobs;
    }

    # Since user can get unmagicked data "by coordinate" requests can go back in time
    # to dredge unusable data from the "dark days"...
    if ($req_type eq 'bycoord' and $row->{IMG_TYPE} ne 'stack' and $row->{MJD_MIN} == 0) {
        # ... so unless the user sets mjd_min clamp it to 2009-04-01
        # XXX: This value should live in the pstampProject table not be hardcoded here
        $row->{MJD_MIN} = 54922;
    }

    if (($req_type eq 'byskycell') or ($req_type eq 'bycoord')) {
        # avoid error from print below if $id isn't needed
        $row->{ID} = "" if !$row->{ID};

        # if this user does not have unlimited access rights adjust the date cut
        # to avoid more recent data
        # We do not adjust dates for stacks since they are not well defined enough.
        # We'll need to filter them by accessLevel during the lookup
        my $stage = $row->{IMG_TYPE};
        if ($stage ne 'stack' and $adjustedDateCuts) {
            # if lower limit was supplied by user we can leave the value alone
            if ($row->{MJD_MAX}) {
                if ($row->{MJD_MAX} > $mjd_max) {
                    print STDERR "Access rights require limiting MJD_MAX to $mjd_max\n";
                    $row->{MJD_MAX} = $mjd_max;
                }
            } else {
                # no limit supplied we silently adjust it
                $row->{MJD_MAX} = $mjd_max;
            }
        }
    }
    
    # Call PS::IPP::PStamp::Job locate_images subroutine to get the images for this
    # request specification. An array reference is returned.
    my $start_locate = gettimeofday();

    print "\nCalling locate_images_for_row for row: $rownum\n";

    my $imageList = locate_images_for_row($ipprc, $image_db, $camera, $row, $verbose);

    my $dtime_locate = gettimeofday() - $start_locate;
    print "Time to locate_images for row $rownum $dtime_locate\n";

    my @rowList = ($row);
    $num_jobs += queueJobs($action, \@rowList, $imageList);

    # if this row slipped through without a job being added add one. 
    if ($row->{job_num} == 0) {
        print "row $row->{ROWNUM} produced no jobs\n";
        print STDERR "row $row->{ROWNUM} produced no jobs\n";
        my $error_code = $row->{error_code};
        $error_code =  $PSTAMP_NO_IMAGE_MATCH if !$error_code;
        insertFakeJobForRow($row, ++$row->{job_num}, $error_code);
    }

    return $num_jobs;
}

sub queueJobForImage
{
    my $row = shift;
    my $stage = shift;
    my $image = shift;
    my $need_magic = shift;
    my $action = shift;

    my $rownum = $row->{ROWNUM};
    my $option_mask = $row->{OPTION_MASK};
    my $components = $row->{components};

    $option_mask |= $PSTAMP_NO_WAIT_FOR_UPDATE if $no_updates_allowed;

    # determine if there is a cut off date for data access for this user
    my $mjd_max;
    my $adjustedDateCuts = adjustDateCuts($row, \$mjd_max);

    my $roi_string;

    # note values were checked by the function validROI()
    my $x = $row->{CENTER_X};
    my $y = $row->{CENTER_Y};
    my $w = $row->{WIDTH};
    my $h = $row->{HEIGHT};
    my $coord_mask = $row->{COORD_MASK};

    my $wholeFile = 0; 
    # For historical reasons there are two ways to specify that the entire file be returned 
    # rather than a postage stamp ...
    if ($w == 0 and $h == 0) {
        # ... The right way: width and height both zero ...
        $wholeFile = 1;
    } else {
        if ($coord_mask & $PSTAMP_CENTER_IN_PIXELS) {
            if ($x == 0 && $y == 0) {
                # ... and pixel coordinate center of 0, 0
                # I made this one up without thinking through the API clearly.
                # allowing width and height to do it works much better
                $wholeFile = 1;
            } else {
                $roi_string = "-pixcenter $x $y";
            }
        } else {
            $roi_string = "-skycenter $x $y";
        }
    }

    if ($wholeFile) {
        $roi_string = "-wholefile";
    } elsif ($coord_mask & $PSTAMP_RANGE_IN_PIXELS) {
            $roi_string .= " -pixrange $w $h";
    } else {
            $roi_string .= " -arcrange $w $h";
    }

    my $component = $image->{component};

    my $job_num = ++($row->{job_num});

    my $imagefile = $image->{image};

    if ($stage eq "stack") {
        # unconvolved stack images weren't available prior to some point in time.
        # XXX: handle this more correctly by examining the stack run's config dump file.
        # It looks like # the feature was turned on sometime around November 11, 2009. stackRun 30067 is the lowest
        # one that I found with an unconvolved image.
        my $MIN_GPC1_STACK_ID_WITH_UNCONVOLVED_IMAGES = 30067;
        if ($row->{unconvolved} and ($row->{PROJECT} eq 'gpc1') and 
            ($image->{stack_id} < $MIN_GPC1_STACK_ID_WITH_UNCONVOLVED_IMAGES)) {
            print STDERR "Unconvolved stack image is not available for stackRun.stack_id: $image->{stack_id}\n";
            insertFakeJobForRow($row, $job_num, $PSTAMP_NOT_AVAILABLE);
            return 1;
        }
    } else { 
        if ($need_magic and !$image->{magicked}) {
            # XXX: should we add a faulted job so the client can know what happened if no images come back?
            # The test for destreaked is made in locate_images now so this code never runs. This leads to no feedback
            # to users, but speeds up processing significantly
            print STDERR "skipping non-magicked image $imagefile\n" if $verbose;

            # for now assume yes.

            insertFakeJobForRow($row, $job_num, $PSTAMP_NOT_DESTREAKED);
            return 1;
        }

        if ($adjustedDateCuts) {
            my $mjd = 0;
            my $exp_id;
            if ($stage eq 'diff') {
                # diff stage doesn't have a dateobs so we need to hack it.
                # Extract dateobs from the exp_name_1
                if ($row->{PROJECT} eq 'gpc1') {
                    my $exp_name = $image->{exp_name_1};
                    if ($exp_name ne 'NULL') {
                        $mjd = 50000 + (substr($exp_name, 1, 4));
                        $exp_id = $image->{exp_id_1};
                    } else {
                        # it's stack-stack diff just let it go with mjd = 0
                        # XXX: deal with this
                    }
                }
            } else {
                $exp_id = $image->{exp_id};
                $mjd = dateobsToMJD($image->{dateobs});
            }

            if ($mjd > $mjd_max) {
                print "User is not authorized for stamps from this exposure exp_id $exp_id.\n";
                print STDERR "User is not authorized for stamps from this exposure exp_id $exp_id.\n";
                insertFakeJobForRow($row, $job_num, $PSTAMP_NOT_AUTHORIZED, $exp_id);
                return 1;
            }
        }
    }
    my $exp_id = $image->{exp_id};
            
    my $args = $roi_string ? $roi_string : "";
    if ($stage eq "raw" or $stage eq "chip") {
        $args .= " -class_id $component" if $component;
    }

    $image->{job_args} = $args;

    my $base = basename($image->{image});
    if (! $base =~ /.fits$/ ) {
        my_die("unexpected image file name found $image->{image}", $PS_EXIT_PROG_ERROR);
    }
    $base =~ s/.fits$//;

    my $filter = $image->{filter};
    if (!$filter) {
        if ($stage eq 'diff') {
            $filter = $image->{filter_1};
        }
        if (!$filter) {
            # XXX: perhaps this should be a programming error...
            print STDERR "missing filter using 'X'\n";
            $filter = 'X';
        }
    }
    # use first character of filter
    $filter = substr($filter, 0, 1);
            
    my $output_base = "$outdir/${rownum}_${job_num}_${filter}_${base}";
    write_params($output_base, $image);

    my $newState = $action eq 'PROCESS' ? "run" : "parsed";
    my $fault = 0;
    my $dep_id;

    queueUpdatesIfNeeded($action, $stage, $image, $option_mask, \$newState, \$fault, \$dep_id);

    my $command = "$pstamptool -addjob  -req_id $req_id -job_type $row->{JOB_TYPE}"
                    . " -outputBase $output_base -rownum $rownum -state $newState -options $option_mask";
    $command .= " -fault $fault" if $fault;
    $command .= " -exp_id $exp_id" if $exp_id;
    $command .= " -dep_id $dep_id" if $dep_id;

    if (!$no_update) {
        # mode eq "queue_job"
        my $start_addjob = gettimeofday();
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            print STDERR @$stderr_buf;
            # XXX TODO: now what? Should we mark the error state for the request?
            # should we keep going for other uris? If so how do we report that some
            # of the work that the request wanted isn't going to get done
            my_die("failed to queue job for request $req_id", $PS_EXIT_UNKNOWN_ERROR);
        }
        my $dtime_addjob = gettimeofday() - $start_addjob;
        print "Time to addjob for row $rownum $dtime_addjob\n";
    } else {
        print "skipping command: $command\n";
    }

    return 1;
}

# queue jobs for a collection of request specifications that have the same Images of Interest
sub queueJobs
{
    my $action = shift;
    my $rowList = shift;
    my $imageList = shift;

    my $firstRow = $rowList->[0];
    my $stage    = $firstRow->{IMG_TYPE};
    my $job_type = $firstRow->{JOB_TYPE};
    my $need_magic = $firstRow->{need_magic};

    my $num_jobs = 0;

    if ($mode eq "list_uri") {
        $num_jobs = $imageList ? scalar @$imageList : 0;
        print "List of $num_jobs Images selected for row: $firstRow->{ROWNUM}\n";
        foreach my $image (@$imageList) {
            print "$image->{image}\n";
            ++$firstRow->{job_num};
            if ($dump_params) {
                my $rownum = $firstRow->{ROWNUM};
                my $jobnum = $firstRow->{job_num};
                my $filter = substr $image->{filter}, 0, 1;
                my $output_base = "${rownum}_${jobnum}_${filter}";
                write_params($output_base, $image);
            }
        }
    } elsif ($job_type eq "get_image") {
        my $n = scalar @$rowList;

        my_die( "error: unexpected number of rows for get_image request: $n", $PS_EXIT_PROG_ERROR) if $n != 1;

        $num_jobs = queueGetImageJobs($firstRow, $imageList, $stage, $need_magic, $action);

    } else {
        if (!$imageList or (scalar @$imageList eq 0)) {
            # We didn't find any images for this set of rows. Insert a fake job to carry
            # the status back to the requestor.
            foreach my $row (@$rowList) {
                my $error_code = $row->{error_code};
                $error_code = $PSTAMP_NO_IMAGE_MATCH if !$error_code;
                insertFakeJobForRow($row, ++$row->{job_num}, $error_code);
                $num_jobs++;
            }
            return $num_jobs;
        }

        foreach my $image (@$imageList) {
            # get the array of row indices that touch this image
            my $row_index = $image->{row_index};
            if (!$row_index or scalar @$row_index == 0) {
                # XXX should this happen? Why did something get returned.
                print "image ${stage}_id: $image->{stage_id} component: $image->{component} matched no rows\n";
                next;
            }
            # XXX: TODO: eventually we may change ppstamp to be able to make multiple stamps per invocation

            foreach my $i (@$row_index) {
                my $row = $rowList->[$i];

                $num_jobs += queueJobForImage($row, $stage, $image, $need_magic, $action);
            }
        }
    }

    return $num_jobs;
}

sub queueGetImageJobs
{
    my $row = shift;
    my $imageList = shift;
    my $stage = shift;
    my $need_magic = shift;
    my $action = shift;

    my $num_jobs = 0;
    my $rownum = $row->{ROWNUM};
    my $option_mask = $row->{OPTION_MASK};

    $option_mask |= $PSTAMP_NO_WAIT_FOR_UPDATE if $no_updates_allowed;

    # For dist_bundle we need
    #  --camera from $image
    #  --stage 
    #  --stage_id from $image
    #  --component from $image
    #  --path_base 
    #  --outdir global to this script

    # loop over images
    foreach my $image (@$imageList) {
        my $stage_id = $image->{stage_id};
        my $component = $image->{component};

        # skip faulted components for now. Should we even be here?
        if ($image->{fault} > 0) {
            printf STDERR "skipping faulted component for $stage $stage_id $component\n" if $verbose;
            next;
        }

        my $job_num = ++($row->{job_num});

        my $imagefile = $image->{image};
        if (($stage ne "stack") and ($need_magic and !$image->{magicked})) {
            # we only get here if req_type is (byid or byexp). For other types the test for magicked is performed
            # in locate_images because it's much more efficient to do the test in the database.
            # For these two modes we fall through to here in order to give feedback to the requestor as
            # to why the request failed to queue jobs.
            print STDERR "skipping non-magicked image $imagefile\n" if $verbose;
            insertFakeJobForRow($row, $job_num, $PSTAMP_NOT_DESTREAKED);
            $num_jobs++;

            next;
        }
        my $exp_id = $image->{exp_id};
            
        my $output_base = "$outdir/${rownum}_${job_num}";

        write_params($output_base, $image);

        my $newState = $action eq 'PROCESS' ? "run" : "parsed";
        my $fault = 0;
        my $dep_id;

        queueUpdatesIfNeeded($action, $stage, $image, $option_mask, \$newState, \$fault, \$dep_id);

        $num_jobs++;
        my $command = "$pstamptool -addjob  -req_id $req_id -job_type $row->{JOB_TYPE}"
                        . " -outputBase $output_base -rownum $rownum -state $newState -options $option_mask";
        $command .= " -fault $fault" if $fault;
        $command .= " -exp_id $exp_id" if $exp_id;
        $command .= " -dep_id $dep_id" if $dep_id;

        if (!$no_update) {
            # mode eq "queue_job"
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                print STDERR @$stderr_buf;
                # XXX TODO: now what? Should we mark the error state for the request?
                # should we keep going for other uris? If so how do we report that some
                # of the work that the request wanted isn't going to get done
                my_die("failed to queue job for request $req_id", $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print "skipping command: $command\n";
        }
    }
    if ( $num_jobs == 0 ) {
        print STDERR "no jobs for row $rownum\n" if $verbose;
        insertFakeJobForRow($row, 1, $PSTAMP_NO_IMAGE_MATCH);
        $num_jobs = 1;
	$row->{job_num} = 1;
    }
    return $num_jobs;
}
sub insertFakeJobForRow
{
    my $row = shift;
    my $job_num = shift;
    my $fault = shift;
    my $exp_id = shift;

    my ($job_type, $rownum);
    if ($row) {
        $job_type = $row->{JOB_TYPE};
        $rownum = $row->{ROWNUM};
        $rownum = 0 if !defined $rownum;
        if ($job_type) {
            if (($job_type ne "stamp") and ($job_type ne "get_image")) {
                print STDERR "invalid job type: $job_type found in row $rownum\n";
                $job_type = "none";
            }
        } else {
            print STDERR "undefined job type found in row $rownum\n";
            $job_type = "none";
        }
    } else {
        $job_type = "none";
        $rownum = 0;
    }

    my $command = "$pstamptool -addjob  -req_id $req_id -job_type $job_type"
                        . " -rownum $rownum -state stop -fault $fault";
    $command .= " -exp_id $exp_id" if $exp_id and $exp_id ne 'NULL';

    if (!$no_update) {
        # mode eq "queue_job"
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            print STDERR @$stderr_buf;
            # XXX TODO: now what? Should we mark the error state for the request?
            # should we keep going for other uris? If so how do we report that some
            # of the work that the request wanted isn't going to get done
            my_die("failed to queue job for request $req_id", $PS_EXIT_UNKNOWN_ERROR);
        }
    } else {
        print "skipping command: $command\n";
    }
}

sub same_images_of_interest {
    my $r1 = shift;
    my $r2 = shift;

    return 0 if (($r1->{REQ_TYPE} eq "bycoord")   or ($r2->{REQ_TYPE} eq "bycoord"));
    return 0 if (($r1->{JOB_TYPE} eq "get_image") or ($r2->{JOB_TYPE} eq "get_image"));
    return 0 if ($r1->{REQ_TYPE} ne $r2->{REQ_TYPE});
    return 0 if ($r1->{IMG_TYPE} ne $r2->{IMG_TYPE});
    return 0 if ($r1->{ID}       ne $r2->{ID});
    return 0 if ($r1->{TESS_ID}  ne $r2->{TESS_ID});
    return 0 if ($r1->{COMPONENT}  ne $r2->{COMPONENT});
    return 0 if ($r1->{REQFILT}  ne $r2->{REQFILT});
    return 0 if ($r1->{DATA_GROUP}    ne $r2->{DATA_GROUP});
    return 0 if ($r1->{MJD_MIN}  ne $r2->{MJD_MAX});
    return 0 if ($r1->{MJD_MAX}  ne $r2->{MJD_MAX});
    return 0 if ($r1->{OPTION_MASK}  ne $r2->{OPTION_MASK});
    return 0 if ($r1->{PROJECT}  ne $r2->{PROJECT});
    return 0 if ($r1->{inverse}  ne $r2->{inverse});
    return 0 if ($r1->{unconvolved}  ne $r2->{unconvolved});
    # don't combine requests in pixel coordinates
    return 0 if (($r1->{COORD_MASK} & $PSTAMP_CENTER_IN_PIXELS) || ($r2->{COORD_MASK} & $PSTAMP_CENTER_IN_PIXELS));

    return 1;
}

sub validNumber
{
    my $val = shift;

    return 0 if !defined $val;

    return ($val =~ /^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/);
}
sub validID
{
    my $val = shift;

    return 0 if !$val;

    return  ! ($val =~ /\D/);
}

sub validROI
{
    my $row = shift;
    return 0 if !validNumber($row->{CENTER_X});
    return 0 if !validNumber($row->{CENTER_Y});
    return 0 if !validNumber($row->{WIDTH});
    return 0 if !validNumber($row->{HEIGHT});

    return 1;
}

sub findRow
{
    my $rownum = shift;
    my $rowList = shift;

    foreach my $row (@$rowList) {
        return $row if $row->{ROWNUM} eq $rownum;
    }
    return undef;
}

sub get_dependent 
{
    my ($action, $r_jobState, $r_fault, $r_dep_id, $imagedb, $state, $stage, $stage_id, $component, $need_magic) = @_;

    # chipRun's can be in full state if destreaking is necessary
    if (($state ne 'cleaned') and ($state ne 'update') and ($state ne 'goto_cleaned') and 
        ($stage ne 'chip' and $state eq 'full')) {
        my_die("$stage $stage_id is in unexpected state $state", $PS_EXIT_PROG_ERROR);
    }

    if (($stage eq 'diff') and ($stage_id <= 22778)) {
    	print STDERR "diff_id $stage_id cannot be updated\n";
	$$r_dep_id = 0;
	$$r_fault = $PSTAMP_GONE;
	$$r_jobState = 'stop';
	return;
    }

    my $dep_id;
    my $command = "$pstamptool -getdependent -stage $stage -stage_id $stage_id -imagedb $imagedb -component $component -outdir $outdir";
    $command .= " -need_magic" if $need_magic;
#    $command .= ' -hold' if $action eq 'PREVIEW';

    if ($label) {
        # compute rlabel for the run.
        # XXX: This bit of policy shouldn't be buried so deeply in the code
        # For now use one that implies 'postage stamp server' 'update' 'request_label"
        my $rlabel = "ps_ud_" . $label;
        $command .= " -rlabel $rlabel";
    }

    if (!$no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            my $fault = $error_code >> 8;
            print STDERR "$command failed with fault $fault\n";
            if ($fault < $PSTAMP_FIRST_ERROR_CODE) {
                # pstamptool returns an error if an existing depenent is faulted
                # Set the object to not available even if the fault < $PSTAMP_FIRST_ERROR_CODE
                # which is nominally a recoverable error in order to keep
                # the request from faulting (which can be very expensive if
                # there are a lot of requests)
                $fault = $PSTAMP_NOT_AVAILABLE
            }
            if ($fault >= $PSTAMP_FIRST_ERROR_CODE) {
                $$r_dep_id = 0;
                $$r_fault = $fault;
                $$r_jobState = 'stop';
                return;
            }
            # for now just die. Later pstamptool will return the whole dependent so we can
            # examine the fault code
            my_die("$command failed with unexpected fault value: $fault", $PS_EXIT_UNKNOWN_ERROR);
        } else {
            my $output = join "", @$stdout_buf;
            chomp $output;
            $dep_id = $output;
            #
            # XXX: need to fault the request or something
            my_die("pstamptool -getdependent returned invalid dep_id", $PS_EXIT_PROG_ERROR) if !$dep_id;
        }
    } else {
        print STDERR "skipping $command\n";
        $dep_id = 42;
    }
    
    $$r_dep_id = $dep_id;
    $$r_fault = 0;
    $$r_jobState = 'run';
}

sub queueUpdatesIfNeeded {
    my $action = shift;
    my $stage = shift;
    my $image = shift;
    my $option_mask = shift;
    my $r_newState = shift;
    my $r_fault = shift;
    my $r_dep_id = shift;

    my $need_magic = 0;

    if ($stage ne 'raw') {
        my $run_state = $image->{state};
        my $data_state = $image->{data_state};
        $data_state = $run_state if $stage eq 'stack';
        my $component_fault = $image->{fault};
        my $stage_id = $image->{stage_id};
        my $component = $image->{component};

        if ($component_fault eq $PSTAMP_GONE) {
            # image is gone and it's not coming back
            print STDERR "$stage $stage_id $component is gone\n";
            $$r_newState = 'stop';
            $$r_fault = $PSTAMP_GONE;
        } elsif (($run_state =~ /purged/) or ($run_state =~ /scrubbed/) or ($run_state eq 'drop')) {
            # image is gone and it's not coming back
            print STDERR "$stage $stage_id $component has state: $run_state data_state: $data_state\n";
            $$r_newState = 'stop';
            $$r_fault = $PSTAMP_GONE;
        } elsif (($run_state eq "error_cleaned") or ($data_state eq 'error_cleaned')) {
            # if cleanup had an error don't get the user's hopes up set fault to gone
            print STDERR "$stage $stage_id $component has state: $run_state data_state: $data_state\n";
            $$r_newState = 'stop';
            $$r_fault = $PSTAMP_GONE;
        } elsif (($data_state ne 'full') or ($need_magic and ($image->{magicked} < 0)) or ($run_state eq 'goto_cleaned')) {
   
            if ($stage eq 'stack') {
                print STDERR "Stamps cannot be made from cleaned stacks.\n";
                $$r_newState = 'stop';
                $$r_fault = $PSTAMP_GONE;
            }
            if (!$$r_fault) {
                # wait for update unless the customer asks us not to
                if (($option_mask & $PSTAMP_NO_WAIT_FOR_UPDATE)) {
                    $$r_newState = 'stop';
                    $$r_fault = $PSTAMP_NOT_AVAILABLE;
                } elsif ($need_magic and !$image->{magicked}) {
                    $$r_newState = 'stop';
                    $$r_fault = $PSTAMP_NOT_DESTREAKED;
                } else {
                    # cause the image to be re-made
                    # set up to queue an update run
                    my $require_magic = ($need_magic or $image->{magicked});
                    get_dependent($action, \$$r_newState, \$$r_fault, $r_dep_id, $image->{imagedb}, 
                        $run_state, $stage, $image->{stage_id}, $image->{component}, $require_magic );
                }
            }
        }
    }
}

sub write_params {
    my $output_base = shift;
    my $image = shift;

    # write the contents of this "image" as a metadata config doc
    # Simply treat all values as strings. This is ok since we are only going to
    # read it from another perl script
    my $mdc_file = "${output_base}.mdc";
    open P, ">$mdc_file" or my_die("failed to open $mdc_file", $PS_EXIT_UNKNOWN_ERROR);

    print P "params METADATA\n";

    foreach my $key (keys %$image) {
        my $value = $image->{$key};
        if (defined $value) {
            printf P "  %-20s STR     %s\n", $key, $value;
        } else {
            printf P "  %-20s STR     NULL\n", $key;
        }
    }

    print P "END\n";
    close P or my_die("failed to close $mdc_file", $PS_EXIT_UNKNOWN_ERROR);
}

sub check_image_type
{
    my $img_type = shift;
    if (!$img_type) {
	    print STDERR "NULL IMG_TYPE supplied\n";
	    return 0;
    }
    if (($img_type eq "raw") or ($img_type eq "chip") or ($img_type eq "warp") or
	($img_type eq "stack") or ($img_type eq 'stack_summary') or ($img_type eq "diff")) {
	return 1;
    } else {
	print STDERR "$img_type is not a valid IMG_TYPE\n";
	return 0;
    }
}

# get user's access level default label and output product
sub getUsersAccessLevel {
    my $email = shift;
    my $r_label = shift;
    my $r_product = shift;

    # $r_label is a copy of the initial label. We change it to a user specific one
    # if one is found.
    my $label = $$r_label;

    # default settings
    $$r_label   =  undef;
    $$r_product =  undef;
    my $level = -1;

    if (!$email) {
        # No email provided. 
        # For now in order to temporarily continue to support the version 1 request format.
        # set the access rights based on the assigned label.
        # Before deployment this will be forbidden.
        if ($label eq 'IFA' or $label eq 'QUB' or $label =~ 'MOPS') {
            $level = 2;
        } else {
            $level = $defaultAccessLevel;
        }
        return $level
    }

    # we've got an email check the domain
    my ($user, $domain) = split '@', $email;
    unless ($user and $domain) {
        print STDERR "Error $email is not an acceptable email adddress.\n";
        return -1;
    } 

    my $cmd = "$pstamptool -listuser -user $user -domain $domain";
    my $results = runToolAndParse($cmd, $verbose);
    my $userinfo = $results->[0];
    if ($userinfo) {
        $level = $userinfo->{accessLevel};
        if ($userinfo->{userProduct}) {
            $$r_product = $userinfo->{userProduct};
        } elsif ($userinfo->{domainProduct}) {
            $$r_product = $userinfo->{domainProduct};
        }
        if ($userinfo->{userLabel}) {
            $$r_label = $userinfo->{userLabel};
        } elsif ($userinfo->{domainLabel}) {
            $$r_label = $userinfo->{domainLabel};
        }
    } else {
        my $cmd = "$pstamptool -listdomain -domain $domain";
        my $results = runToolAndParse($cmd, $verbose);
        my $domaininfo = $results->[0];
        if ($domaininfo) {
            $level = $domaininfo->{accessLevel};
            if ($domaininfo->{defaultProduct}) {
                $$r_product = $domaininfo->{defaultProduct};
            }
            if ($domaininfo->{defaultLabel}) {
                $$r_label = $domaininfo->{defaultLabel};
            }
    } else {
            # no specific user or domain information found in the database. Use the defaults
            $level = $defaultAccessLevel;
            $$r_label =  undef;
            $$r_product =  undef;
        }
    }

    return $level;
}


# determine the latest exposure date that a user may have access to
# based on the user's accessLevel.
sub adjustDateCuts {
    my $row = shift;
    my $r_mjd_max = shift;
    my $adjusted = 0;

    my $accessLevel = $row->{accessLevel};
    my $project = $row->{PROJECT};
    my $mjd_max_user = $row->{MJD_MAX};

    my $key = "$project.$accessLevel";

    my $levelInfo = $accessLevelCache{$key};

    if (!$levelInfo) {
        my $cmd = "$pstamptool -listaccesslevel -accessLevel $accessLevel -project_name $project";
        my $results = runToolAndParse($cmd, $verbose);
        if (!$results or !$results->[0]) {
            my_die("failed to find accessLevel info for project: $project level: $accessLevel\n", $PS_EXIT_CONFIG_ERROR);
        }
        $levelInfo = $results->[0];
        $accessLevelCache{$key} = $levelInfo;
    }

    my $mjd_max_thisLevel = $levelInfo->{mjd_max};
    if ($mjd_max_thisLevel)  {
        # This project has a limit for this level
        # adjust the user supplied limit unless it is less than the maximum

        if (!$mjd_max_user or ($mjd_max_user > $mjd_max_thisLevel)) {
            $$r_mjd_max = $mjd_max_thisLevel;
            $adjusted = 1;
        }
    }

    return $adjusted;
}

sub dateobsToMJD {
    my $dateobs = shift;

    my ($date, $time) = split "T", $dateobs;

    my ($year, $mon, $day) = split "-", $date;

    my ($hr, $min, $sec) = split ":",   $time;
    my $fraction = $sec - int($sec);
    $sec = int($sec);

    my $ticks = timegm($sec, $min, $hr, $day, $mon-1, $year-1900);
    $ticks += $fraction;

    # I'm not allowing for leap seconds here but for our purposes here that is ok
    my $mjd = 40587.0 + ($ticks / 86400.);
    return $mjd;
}

sub my_die
{
    my $msg = shift;
    my $fault = shift;

    print STDERR $msg;

    # we don't fault the request here pstamp_parser_run.pl handles that if necessary

    exit $fault;
}
