#!/bin/env perl
###
### pstampparser_run.pl
###     Run the request parser for a given request id
### This script should be called request_parser.pl since it handles more than postage
### stamp requests
###

use warnings;
use strict;

use Sys::Hostname;
use Getopt::Long qw( GetOptions );
use File::Basename qw( basename dirname);
use File::Copy;
use POSIX qw( strftime );
use Carp;
use IPC::Cmd 0.36 qw( can_run run );

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw( :standard );
use PS::IPP::PStamp::RequestFile qw( :standard );

my $req_id;
my $uri;
my $redirect_output;
my $product;
my $outdir;
my $label;
my $need_magic;
my $verbose;
my $dbname;
my $dbserver;

GetOptions(
    'req_id=s'          =>  \$req_id,
    'uri=s'             =>  \$uri,
    'product=s'         =>  \$product,
    'outdir=s'          =>  \$outdir,
    'label=s'           =>  \$label,
    'need_magic'        =>  \$need_magic,
    'redirect-output'   =>  \$redirect_output,
    'verbose'           =>  \$verbose,
    'dbname=s'          =>  \$dbname,
    'dbserver=s'         =>  \$dbserver,
);

if ($verbose) {
    my $host = hostname();
    print "\n\n";
    print "Starting script $0 on $host\n\n";
}

$need_magic = 0;

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $pstampparse = can_run('pstampparse.pl') or (warn "Can't find pstampparse.pl" and $missing_tools = 1);
my $dqueryparse = can_run('dqueryparse.pl') or (warn "Can't find dqueryparse.pl" and $missing_tools = 1);
my $dsget = can_run('dsget') or (warn "Can't find dsget" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}


my_die("--req_id --uri --product are required", $req_id, $PS_EXIT_CONFIG_ERROR)
    if !defined($req_id) or
       !defined($uri) or
       !defined($product);

my $ipprc = PS::IPP::Config->new(); # IPP Configuration

my $outputDataStoreRoot = metadataLookupStr($ipprc->{_siteConfig}, 'DATA_STORE_ROOT');
exit ($PS_EXIT_CONFIG_ERROR) unless defined $outputDataStoreRoot; # lookup failure outputs a message
my $defaultOutputRoot = $outputDataStoreRoot;

my $pstamp_workdir = metadataLookupStr($ipprc->{_siteConfig}, 'PSTAMP_WORKDIR');
exit ($PS_EXIT_CONFIG_ERROR) unless defined $pstamp_workdir; # lookup failure outputs a message

if (!$dbserver) {
    $dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

if (!$outdir or ($outdir eq "NULL")) {
    # outdir is where all of the files generated for this request are placed
    # NOTE: this location needs to be kept in sync with the web interface ( request.php )
    my $datestr = strftime "%Y/%m/%d", gmtime;
    my $datedir = "$pstamp_workdir/$datestr";
    if (! -e $datedir ) {
        my $rc = system "mkdir -p $datedir";
        if ($rc) {
            my $status = $rc >> 8;
            $status = $PS_EXIT_CONFIG_ERROR if !$status;
            my_die( "failed to create working directory $datedir for request id $req_id", $req_id,
                $status);
        }
    }

    $outdir = "$datedir/$req_id";
}

if (! -e $outdir ) {
    mkdir $outdir or my_die("failed to create working directory $outdir for request id $req_id", $req_id,
        $PS_EXIT_CONFIG_ERROR);
}
    

if ($redirect_output) {
    my $logDest = "$outdir/psparse.$req_id.log";
    $ipprc->redirect_output($logDest);
}

my $defaultDSProduct = metadataLookupStr($ipprc->{_siteConfig}, 'PSTAMP_DATA_STORE_PRODUCT');
exit ($PS_EXIT_CONFIG_ERROR) unless defined $defaultDSProduct;
    
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files


my $fn = basename($uri);
my $new_uri = "$outdir/$fn";
if ($uri =~ /^http:/) {
    # if the uri is an http uri download the file 
    my $command = "$dsget --uri $uri --filename $new_uri --timeout 120";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        my_die("Unable to perform $command error code: $error_code", $req_id, $error_code >> 8);
    }
} elsif ($uri ne $new_uri) {
    # copy the request file into outdir
    if (-e $new_uri) {
        # file exists already delete it incase the previous copy is bogus
        unlink $new_uri or my_die("failed to unlink $new_uri", $req_id, $PS_EXIT_UNKNOWN_ERROR);
    }
    if (! copy $uri, $new_uri) {
        my_die ("failed to copy request file $uri to workdir $outdir", $req_id, $PS_EXIT_UNKNOWN_ERROR);
    }
}
$uri = $new_uri;

my_die("request file $uri not found", $req_id, $PS_EXIT_UNKNOWN_ERROR) if ! -e $uri;

#  if product was not defined (in database), use the default
if ($product eq "NULL") {
    $product = $defaultDSProduct;
}

my $parse_cmd;
my $request_type;
my $reqType;    # for the database

my $request_fault = $PSTAMP_INVALID_REQUEST;

# default action is to process the request after parsing. This can be overridden by
# PREVIEW mode for pstamp requests
my $action = 'PROCESS';

if (-r $uri) {
    # run the appropriate parse command to parse the queue the jobs for this request
    # first check the extension header to find the EXTNAME
    $request_type = find_request_type($uri, \$action);

    if ($request_type) {
        print STDERR "request_type for $req_id is $request_type\n" if $verbose;
        if ($request_type eq "PS1_PS_REQUEST") {
            $reqType = 'pstamp';
            $parse_cmd = "$pstampparse";
            $parse_cmd .= " --label $label" if $label;
            $parse_cmd .= " --need_magic" if $need_magic;
            $request_fault = 0;
        } elsif ($request_type eq "MOPS_DETECTABILITY_QUERY") {
            $reqType = 'dquery';
            $parse_cmd = "$dqueryparse";
	    $parse_cmd .= " --label $label" if $label;
            $request_fault = 0;
        } else {
            print STDERR "Unknown request type $request_type found in $uri";
        }
    } else {
        print STDERR "No EXTNAME found keyword in $uri";
    }
} else {
    if (-e $uri) {
        print STDERR "Request file $uri is not readable";
    } else {
        print STDERR "Request file $uri does not exist";
    }
}

if (!$parse_cmd) {
    # can't go any farther, set fault and set request state to run 
    # request_finish.pl will clean up, perhaps notifiying the operator of the input data store
    # that they sent us a request file that we don't understand

    my $command = "$pstamptool -updatereq -req_id $req_id -set_state run";
    $command   .= " -set_reqType unknown";
    $command   .= " -set_fault $request_fault";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
    exit $request_fault;
}

$parse_cmd .= " --mode queue_job --req_id $req_id --product $product --outdir $outdir --file $uri";
$parse_cmd .= " --dbname $dbname" if $dbname;
$parse_cmd .= " --dbserver $dbserver" if $dbserver;
$parse_cmd .= " --verbose" if $verbose;

my $newState;
my $fault;
{
    my $error_file_name = "$outdir/parse_error.txt";
    # get rid of any error file from previous attempt to parse this request
    unlink $error_file_name if (-e $error_file_name);

    # Run the parser

    my $command = "$parse_cmd";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    # save the contents of stderr (if any) to a file. This is relevant if
    # the file was parseable but one or more of the rows in the request file generated an error
    my $errbuf = join "", @$stderr_buf;
    if ($errbuf) {
        if (!open OUT, ">$error_file_name") {
            print STDERR ("unable to open parse_error file $error_file_name");
        } else {
            print OUT "$errbuf";
            close(OUT);
        }
        print STDERR $errbuf if $verbose;
    }

    if ($success) {
        # XXX: This bit of the postage stamp request API has slipped in here because we need to control
        # the new state of the request
        if ($action eq 'PROCESS') {
            $newState = 'run';
        } elsif ($action eq 'PREVIEW') {
            $newState = 'parsed';
        } else {
            print STDERR "WARNING Ignoring unexpected value for ACTION found in request header: $action\n";
        }
    } else {
        $fault = $error_code >> 8;
    }
}

#
# update the state of this request from 'new' to 'run' and set the uri to the downloaded location
#
{
    my $command = "$pstamptool -updatereq -req_id $req_id";
    $command   .= " -set_state $newState" if $newState;
    $command   .= " -set_outdir $outdir";
    $command   .= " -set_reqType $reqType" if $reqType;
    $command   .= " -set_uri $new_uri" if $new_uri;
    $command   .= " -set_fault $fault" if $fault;
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
}

# Note: We do not return $fault here. If there was a fatal error we've already exited.
# If we got a fault it's due to bad input from the user we've set things up for this to be
# handled by the task pstamp.request.finish

exit 0;


sub find_request_type {
    # find the EXTNAME in the input fits table
    my $file_name = shift;
    my $r_action = shift;

    my $out = `echo $file_name | fields -x 0 EXTNAME ACTION`;

    if ($out) {
        # output from fields is filename value
        my ($dummy, $extname, $action) = split " ", $out;

        # Set the action if it is defined in the request header
        # XXX:consider doing this only if extname is PS1_PS_REQUSET
        $$r_action = $action if ($action);

        return $extname;
    } else {
        return undef;
    }
}

sub my_die {
    my $msg = shift;
    my $req_id = shift;
    my $fault = shift;

    carp($msg);

    if (!$req_id) {
        exit $PS_EXIT_CONFIG_ERROR;
    }

    my $command = "$pstamptool -updatereq -req_id $req_id  -set_fault $fault";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
    exit $fault;
}
