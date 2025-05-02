#!/bin/env perl

# pstamp_finish.pl

use warnings;
use strict;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use Carp;

use Time::Local;
use Sys::Hostname;
use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Copy;
use File::Basename qw(dirname);

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw( :standard );
use PS::IPP::PStamp::RequestFile qw( :standard );
use PS::IPP::PStamp::Job qw( :standard );

my %imagedb_cache;

my ( $req_id, $req_name, $req_file, $outdir, $product, $dbname, $dbserver, $verbose, $save_temps, $redirect_output);

# the char to the right of the bar may be used as a single - alias for the longer name
# for example --input and -i are equivalent
GetOptions(
           'req_id=s'       => \$req_id,
           'req_name=s'     => \$req_name,
           'req_file=s'     => \$req_file,
           'product=s'      => \$product,
           'outdir=s'       => \$outdir,
	   'dbname=s'       => \$dbname,
	   'dbserver=s'     => \$dbserver,
	   'verbose'        => \$verbose,
	   'save-temps'     => \$save_temps,
	   'redirect-output' => \$redirect_output,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

die "usage: --req_id id --req_name name --req_file file --product product --outdir output_directory [--dbname dbname --verbose]\n"
    if !$req_id or !$req_name or !$req_file or !$product or !$outdir;

die "outdir is NULL\n" if $outdir eq "NULL";

my $ipprc = PS::IPP::Config->new(); # IPP Configuration
if ($redirect_output) {
    # XXX: what happens here if the directory does not exist? We check below
    my $logDest = "$outdir/psfinish.$req_id.log";
    $ipprc->redirect_output($logDest);
}

if (!$dbserver) {
    $dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $dsreg  = can_run('dsreg')  or (warn "Can't find dsreg"  and $missing_tools = 1);
my $pstamp_results = can_run('pstamp_results_file.pl') 
                            or (warn "Can't find pstamp_results_file.pl" and $missing_tools = 1);
my $pstampdump = can_run('pstampdump') or (warn "Can't find pstampdump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

if ($product eq "NULL") {
    # nothing more to do
    my_die("product is NULL!", $req_id, $PS_EXIT_PROG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

{
    # set the output fileset's name to the request name.
    my $fileset = $req_name;

    print STDERR "product: $product  REQ_NAME: $req_name $outdir\n" if $verbose;

    if (!-e $outdir) {
        # something must have gone wrong parsing the request
        print STDERR  "output directory $outdir does not exist\n";

        if (!mkdir $outdir) {
            my_die("cannot create output directory $outdir",$req_id, $PS_EXIT_UNKNOWN_ERROR);
        }


    } elsif (! -d $outdir ) {
        my_die("output directory $outdir exists but is not a directory", $req_id, $PS_EXIT_UNKNOWN_ERROR);
    }

    if (! -e $req_file ) {
        my_die("request file $req_file is missing", $req_id, $PS_EXIT_CONFIG_ERROR);
    }

    # this function is PS::IPP::PStamp::RequestFile::read_request_file
    my ($header, $rows) = read_request_file($req_file);

    if (!$header or !$rows) {
        # Since a request got queued, the request file must have been readable at some point 
        my_die("failed to read request file $req_file", $req_id, $PS_EXIT_CONFIG_ERROR);
    }

    # start building the list of files to be placed in the output fileset
    my ($rlf, $reglist_name) = tempfile ("$outdir/reglist.XXXX", UNLINK => !$save_temps);

    # results file
    print $rlf "results.fits|||table|\n";
    # human readable representation of the results file
    print $rlf "results.mdc|||text|\n";

    my $err_file = "parse_error.txt";
    if (-e "$outdir/$err_file" ) {
        print $rlf "$err_file|||text|\n";
    }

    my $request_fault = 0;
    {
        # The results table definition file
        my ($tdf, $table_def_name) = tempfile ("$outdir/tabledef.XXXX", UNLINK => !$save_temps);
        # data for the header
        print $tdf "$req_name|$req_id|\n";
        # get the list of jobs generated for this request
        my @jobs;
        {
            my $command = "$pstamptool -listjob -req_id $req_id";
            $command   .= " -dbname $dbname" if $dbname;
            $command   .= " -dbserver $dbserver" if $dbserver;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                die("Unable to perform $command error code: $error_code");
            }
            my $output = join "", @$stdout_buf;
            if (!$output) {
                # This should not happen. A fake job should have been entered
                my_die("Request $req_id produced no jobs", $req_id, $PS_EXIT_PROG_ERROR);
            } else {
                my $jobs = parse_md_fast($mdcParser, $output);

                @jobs = @$jobs;
            }
        }

        my $exp_info;
        foreach my $job (@jobs) {
            my $job_id = $job->{job_id};
            my $job_type = $job->{jobType};
            my $rownum = $job->{rownum};
            my $fault = $job->{fault};
            my $exp_id = $job->{exp_id};

            my $error_string = get_error_string($fault);

            if (($fault eq $PSTAMP_DUP_REQUEST) and ($req_name eq "NULL")) {
                # this request had a duplicate request name yet the parser didn't give
                # it an "ERROR style name.
                my_die("duplicate request not given a request name", $req_id, $PS_EXIT_PROG_ERROR);
            }
            my ($row, $req_info, $project) = get_request_info($rows, $rownum);

            my $proj_hash = resolve_project($ipprc, $project, $dbname, $dbserver);
            my $image_db = $proj_hash->{dbname};
            if (!$image_db and !$fault) {
                # if project isn't resolvable, the paser should have faulted this job
                my_die("failed to find imagedb for project $project", $req_id, $PS_EXIT_CONFIG_ERROR);
            }

            my $job_params = get_job_parameters($job);
            my $stage = "";
            if ($job_params) {
                $stage = $job_params->{stage};
            }

            if ($stage ne 'stack') {
                # get the metadata for the exposure (if any i.e. stack) 
                # returns an appropriate string if !$exp_id
                $exp_info = get_exposure_info($job_params, $image_db, $exp_id);
            } else {
                my $filter = $job_params->{filter};
                $filter = "0" if !$filter;
                my $mjd_obs = $job_params->{mjd_obs};
                $mjd_obs = "0" if !$mjd_obs;
                $exp_info = "$mjd_obs|0|0|$filter|0|0";
            }

            if (($job_type eq "stamp") || ($job_type eq "get_image") || ($job_type eq "none")) {
                my $jreglist = "$outdir/reglist$job_id";
                if (open JRL, "<$jreglist") {;
                    # process the reglist file to get the list of files produced by this job
                    foreach my $line (<JRL>) {

                        # XXX: we are getting many cases where the size and/or md5sum calculated by
                        # the job has changed by the time the request_finish has run
                        # Don't
                        # add line to the requests's reglist
                        # ....
    #                    print $rlf $line;


                        chomp $line;
                        my ($img_name, $reported_size, $reported_sum, $filetype) = split '\|', $line;
                        my $use_supplied_size = 1;
                        if ($use_supplied_size) {
                            print $rlf "$img_name|$reported_size|$reported_sum|$filetype|\n";
                        } else {
                            # ... instead let dsreg compute the paramters by leaving them blank
                            print $rlf "$img_name|||$filetype|\n";
                        }

                        # add line to the table definition file
                        print $tdf "$rownum|$fault|$error_string|$img_name|$job_id|";

                        # ra_deg and dec_deg are the coordinates of center of the stamp
                        # first assume that the image is compressed and check the first extension.
                        # If not found check the PHU. If that doesn't work just set them to zero.
                        # XXX do this more cleanly
                        my (undef, $ra_deg, $dec_deg) = split " ", `echo $outdir/$img_name | fields -x 0 RA_DEG DEC_DEG`;
                        if (!defined $ra_deg) {
                            (undef, $ra_deg, $dec_deg) = split " ", `echo $outdir/$img_name | fields RA_DEG DEC_DEG`;
                        }
                        $ra_deg = 0.0 if (!$ra_deg);
                        $dec_deg = 0.0 if (!$dec_deg);
                        print $tdf "$ra_deg|$dec_deg|";

                        print $tdf "$exp_info|";
                        print $tdf "$req_info|";
                        print $tdf "\n";
                    }
                    close JRL;
                } else {
                    my_die("No reglist for successful job: $job_id", $req_id, $PS_EXIT_PROG_ERROR) 
                        if $job->{state} eq 'stop' and $fault eq $PSTAMP_SUCCESS;
                    print STDERR "no reglist file for job $job_id\n" if $verbose;
                    print $tdf "$rownum|$fault|$error_string|0|$job_id|";
                    print $tdf "0|0|";       # center of (non-existent) stamp
                    print $tdf "$exp_info|";
                    print $tdf "$req_info|";
                    print $tdf "\n";
                }
            } else {
                # XXX do list jobs
                # we can probably arange things to use the code as above and skip the fileset registration
                print STDERR "Unknown jobType: $job_type";
                next;
            }
        }
        close $tdf;
        # make the results file
        {
            my $command = "$pstamp_results --input $table_def_name --output $outdir/results.fits";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                print STDERR "Unable to perform $command error code: $error_code\n";
                $request_fault = $error_code >> 8;
            } else {
                # dump a textual representation
                my $command = "$pstampdump $outdir/results.fits > $outdir/results.mdc";
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $request_fault = $error_code >> 8;
                    my_die("Unable to perform $command error code: $error_code", $req_id, $request_fault);
                }
            }
        }
    }

    close $rlf;

    if (!$request_fault) {
        # register the fileset
        my $command = "$dsreg --list $reglist_name --add $fileset --product $product --type PSRESULTS";
        $command .= " --link --datapath $outdir --ps0 $req_id";

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $request_fault = $error_code >> 8;
            my_die("Unable to perform $command error code: $error_code\n", $req_id, $request_fault);
        }
    }
    # set the request's state to stop
    {
        my $command = "$pstamptool -updatereq -req_id $req_id -set_state stop";
        $command   .= " -dbname $dbname" if $dbname;
        $command   .= " -dbserver $dbserver" if $dbserver;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            die("Unable to perform $command error code: $error_code");
        }
    }
    exit $request_fault;
}

sub my_die {
    my $msg = shift;
    my $req_id = shift;
    my $fault  = shift;

    carp($msg);

    my $command = "$pstamptool -updatereq -req_id $req_id -set_fault $fault";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
    exit $fault;
}

sub get_request_info {
    my $rows = shift;
    my $rownum = shift;

    if ($rownum eq 0) {
        my $dummy_rowinfo = "0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|";
        return (undef, $dummy_rowinfo, "none");
    }
    my $row = $rows->{$rownum};


    # these may be set to null during processing
    my $component = $row->{COMPONENT};
    $component = "null" if !$component;
    my $tess_id = $row->{TESS_ID};
    $tess_id = "null" if !$tess_id;
    my $comment = $row->{COMMENT};
    $comment = "null" if !$comment;
    my $data_group = $row->{DATA_GROUP};
    if (!defined $data_group) {
        # XXX: backwards compatibility hook. Remove "soon".
        $data_group = $row->{LABEL};
    }
    $data_group = "null" if !$data_group;

    # XXX: This is ugly, error prone and hard to change.
    # Create a results file module and provide a list of the names (we have the data in the columns)
    my $rowinfo = "$row->{PROJECT}|$row->{JOB_TYPE}|$row->{REQ_TYPE}|$row->{IMG_TYPE}|";
    $rowinfo   .= "$row->{ID}|$tess_id|$component|$data_group|$row->{OPTION_MASK}|$row->{MJD_MIN}|$row->{MJD_MAX}|";
    $rowinfo   .= "$row->{REQFILT}|$row->{COORD_MASK}|$row->{CENTER_X}|$row->{CENTER_Y}|";
    $rowinfo   .= "$row->{WIDTH}|$row->{HEIGHT}|";
    $rowinfo   .= $comment;

    return ($row, $rowinfo, $row->{PROJECT});
}

sub get_job_parameters {
    my $job = shift;
    if (!$job->{outputBase}) {
        print "get_job_parameters: $job->{job_id} has no outputBase\n";
        return undef;
    }
    my $params_file = $job->{outputBase} . '.mdc';
    if (! -e $params_file ) {
        print "get_job_parameters: $job->{job_id} has no parameters file\n";
        return undef;
    }
    open IN, "<$params_file" or die "unable to open $params_file";
    my $data = join "", (<IN>);
    close IN;
    if (! $data ) {
        print "get_job_parameters: parameters file is empty\n";
        return undef;
    }
    my $metadata = $mdcParser->parse($data) or die("Unable to parse metdata config doc");

    # no need to use parse_md_fast here
    my $results = parse_md_list($metadata);
    if (scalar @$results != 1) {
        print STDERR "get_job_params: failed to parse_md_list\n";
        return undef;
    }
    return $results->[0];
}

sub get_exposure_info {
    my $job_params = shift;
    my $image_db= shift;
    my $exp_id = shift;

    my ($dateobs, $ra, $decl, $filter, $exp_time, $exp_name);

    $dateobs  = $job_params->{dateobs};
    $ra       = $job_params->{ra};
    $decl     = $job_params->{decl};
    $filter   = $job_params->{filter};
    $exp_time = $job_params->{exp_time};
    $exp_name = $job_params->{exp_name};

    unless (defined $dateobs and defined $ra and defined $decl and defined $filter and defined $exp_time and defined $exp_name) {
        # job params don't have all of the values that we need (most likely this is a diff stage job)
        # go look up the exposure if we have one
        if (!$exp_id or !$image_db) {
            # no exposure id just return zeros
            # XXX: we could put a value in for filter, but we don't have a good place to find it

            #"$mjd_obs|$ra_obs|$dec_obs|$filter|$exp_time|$fpa_id";
            return "0|0|0|0|0|0";
        }

        my $exp;
        # get cache of exposures for this image_db
        my $exp_cache = $imagedb_cache{$image_db};
        if (defined $exp_cache) {
            $exp = $exp_cache->{$exp_id};
        }

        unless (defined $exp) {

            my $regtool = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
            if ($missing_tools) {
                warn("Can't find required tools.");
                exit ($PS_EXIT_CONFIG_ERROR);
            }

            my $command = "$regtool -processedexp -dbname $image_db -exp_id $exp_id";

            # run the tool and parse the output
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
            unless ($success) {
                # not sure if we should die here
                die "cannot get exposure information for $exp_id from image database $image_db";
            }
            my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

            my $output = join "", @$stdout_buf;
            if (!$output) {
                print STDERR "no output returned from $command\n" if $verbose;
                return undef;
            }
            my $exposures = parse_md_fast($mdcParser, $output);
            my $numExp = @$exposures;

            die "unexpected number of exposures $numExp found for exp_id: $exp_id in DB: $image_db" if $numExp != 1;

            $exp = $exposures->[0];

            unless (defined $exp_cache) {
                my %new_exp_cache;
                $exp_cache = \%new_exp_cache;
                $imagedb_cache{$image_db} = $exp_cache;
            }
            $exp_cache->{$exp_id} = $exp;
        } else {
            print "found $exp_id in cache\n";
        }
        
        #my $info = "$mjd_obs|$ra_obs|$dec_obs|$filter|$exp_time|$fpa_id";
        $dateobs  = $exp->{dateobs};
        $ra       = $exp->{ra};
        $decl     = $exp->{decl};
        $filter   = $exp->{filter};
        $exp_time = $exp->{exp_time};
        $exp_name = $exp->{exp_name};
    }

    die "failed to find exp_info for $exp_id" unless (defined $dateobs and defined $ra and defined $decl and defined $filter and defined $exp_time and defined $exp_name);
    
    use constant RADIANS_TO_DEGREES => 90. / atan2(1, 0);
    my $ra_deg   = $ra * RADIANS_TO_DEGREES;
    my $decl_deg = $decl * RADIANS_TO_DEGREES;
    my $mjd_obs = dateobs_to_mjd($dateobs);

    my $info = "$mjd_obs|$ra_deg|$decl_deg|$filter|$exp_time|$exp_name";
            
    return $info;
}

sub dateobs_to_mjd {
    my $dateobs = shift;

    # dateobs is in format: 1970-01-01T00:00:00

    my ($date, $time) = split "T", $dateobs;
    my ($year, $mon, $day) = split "-", $date;
    my ($hr, $min, $sec) = split ":", $time;

    my $ticks = timegm($sec, $min, $hr, $day, $mon-1, $year-1900);

    # dateobs is UTC convert to TAI
    # XXX: Do this properly
    if ($year >= 2009) {
        $ticks += 34;
    } else {
        $ticks += 33;
    }

    return 40587.0 + ($ticks/86400.);
}
