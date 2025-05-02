#!/bin/env perl

###
###     PStamp/RequestFile.pm
###     subroutines and constants related to Postage Stamp Request Files
###

package PS::IPP::PStamp::RequestFile;

use strict;
use warnings;

our $VERSION = '1.0';

use base qw( Exporter );

our @EXPORT_OK = qw( 
                    read_request_file
                    get_error_string
                    $PSTAMP_CENTER_IN_PIXELS
                    $PSTAMP_RANGE_IN_PIXELS
                    $PSTAMP_SELECT_IMAGE
                    $PSTAMP_SELECT_MASK
                    $PSTAMP_SELECT_VARIANCE
                    $PSTAMP_SELECT_WEIGHT
                    $PSTAMP_SELECT_CMF
                    $PSTAMP_SELECT_SOURCES
                    $PSTAMP_SELECT_PSF
                    $PSTAMP_SELECT_BACKMDL
                    $PSTAMP_SELECT_JPEG
                    $PSTAMP_SELECT_EXP
                    $PSTAMP_SELECT_NUM
                    $PSTAMP_SELECT_UNCOMPRESSED
                    $PSTAMP_SELECT_INVERSE
                    $PSTAMP_SELECT_UNCONV
                    $PSTAMP_RESTORE_BACKGROUND
		    $PSTAMP_MULTI_OVERLAP_IMAGE
                    $PSTAMP_USE_IMFILE_ID
                    $PSTAMP_NO_WAIT_FOR_UPDATE
                    $PSTAMP_SELECT_EXPJPEG
                    $PSTAMP_SELECT_NUMJPEG
                    $PSTAMP_SUCCESS
                    $PSTAMP_FIRST_ERROR_CODE
                    $PSTAMP_SYSTEM_ERROR
                    $PSTAMP_NOT_IMPLEMENTED
                    $PSTAMP_UNKNOWN_ERROR
                    $PSTAMP_DUP_REQUEST
                    $PSTAMP_INVALID_REQUEST
                    $PSTAMP_UNKNOWN_PROJECT
                    $PSTAMP_UNKNOWN_PRODUCT
                    $PSTAMP_NO_IMAGE_MATCH
                    $PSTAMP_NOT_DESTREAKED
                    $PSTAMP_NOT_AVAILABLE
                    $PSTAMP_GONE
                    $PSTAMP_NO_JOBS_QUEUED
                    $PSTAMP_NO_OVERLAP
                    $PSTAMP_NOT_AUTHORIZED
                    $PSTAMP_NO_VALID_PIXELS
                    $PSTAMP_BG_RESTORE_NOT_AVAILABLE
                    );
our %EXPORT_TAGS = (standard => [@EXPORT_OK]);


# these values need to match the ones in pstamp/src/pstamp.h
our $PSTAMP_CENTER_IN_PIXELS = 1;
our $PSTAMP_RANGE_IN_PIXELS  = 2;

# Definition of the bits in OPTION_MASK
our $PSTAMP_SELECT_IMAGE     = 1;
our $PSTAMP_SELECT_MASK      = 2;
our $PSTAMP_SELECT_VARIANCE  = 4;
our $PSTAMP_SELECT_WEIGHT    = 4;
our $PSTAMP_SELECT_SOURCES   = 8;
our $PSTAMP_SELECT_CMF       = 8;
our $PSTAMP_SELECT_PSF       = 16;
our $PSTAMP_SELECT_BACKMDL   = 32;
our $PSTAMP_SELECT_JPEG      = 64;
our $PSTAMP_SELECT_EXP       = 128;
our $PSTAMP_SELECT_NUM       = 256;
our $PSTAMP_SELECT_UNCOMPRESSED = 512;
our $PSTAMP_SELECT_INVERSE      = 1024;
our $PSTAMP_SELECT_UNCONV       = 2048;
our $PSTAMP_RESTORE_BACKGROUND  = 4096;
# MEH -- previously unused 8192
our $PSTAMP_MULTI_OVERLAP_IMAGE  = 8192; 
our $PSTAMP_USE_IMFILE_ID      = 16384;
our $PSTAMP_NO_WAIT_FOR_UPDATE = 32768;

our $PSTAMP_SELECT_EXPJPEG     = 0x10000;
our $PSTAMP_SELECT_NUMJPEG     = 0x20000;

# these bits have been repurposed. They were only exposed to MOPS and IFA and they have adapted.
#our $PSTAMP_REQUEST_UNCENSORED = 0x10000;
#our $PSTAMP_REQUIRE_UNCENSORED = 0x20000;

# job and result codes
# NOTE: these must match the values in pstamp/src/pstamp.h
our $PSTAMP_SUCCESS          = 0;
our $PSTAMP_FIRST_ERROR_CODE = 10;
our $PSTAMP_SYSTEM_ERROR     = 10;
our $PSTAMP_NOT_IMPLEMENTED  = 11;
our $PSTAMP_UNKNOWN_ERROR    = 12;

our $PSTAMP_DUP_REQUEST      = 20;
our $PSTAMP_INVALID_REQUEST  = 21;
our $PSTAMP_UNKNOWN_PROJECT  = 22;
our $PSTAMP_UNKNOWN_PRODUCT  = 22;  #this error code was a typo it is left for compatabiliy
our $PSTAMP_NO_IMAGE_MATCH   = 23;
our $PSTAMP_NOT_DESTREAKED   = 24;
our $PSTAMP_NOT_AVAILABLE    = 25;
our $PSTAMP_GONE             = 26;  # this value is also used in ippTools
our $PSTAMP_NO_JOBS_QUEUED   = 27;
our $PSTAMP_NO_OVERLAP       = 28;
our $PSTAMP_NOT_AUTHORIZED   = 29;
our $PSTAMP_NO_VALID_PIXELS  = 30;
our $PSTAMP_BG_RESTORE_NOT_AVAILABLE = 31;


use IPC::Cmd 0.36 qw( can_run run );

use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config qw( :standard );

my @errorStrings = qw(
PSTAMP_SUCCESS
PS_EXIT_UNKNOWN_ERROR
PS_EXIT_SYS_ERROR
PS_EXIT_CONFIG_ERROR
PS_EXIT_PROG_ERROR
PS_EXIT_DATA_ERROR
PS_EXIT_TIMEOUT_ERROR
undefined
undefined
undefined
PSTAMP_SYSTEM_ERROR
PSTAMP_NOT_IMPLEMENTED
PSTAMP_UNKNOWN_ERROR
undefined
undefined
undefined
undefined
undefined
undefined
undefined
PSTAMP_DUP_REQUEST
PSTAMP_INVALID_REQUEST
PSTAMP_UNKNOWN_PROJECT
PSTAMP_NO_IMAGE_MATCH
PSTAMP_NOT_DESTREAKED
PSTAMP_NOT_AVAILABLE
PSTAMP_GONE
PSTAMP_NO_JOBS_QUEUED
PSTAMP_NO_OVERLAP
PSTAMP_NOT_AUTHORIZED
PSTAMP_NO_VALID_PIXELS
);

sub read_request_file {
    my $request_file_name = shift;
    die "need request file name\n" unless $request_file_name;

    my $verbose = shift;

    my $ipprc = PS::IPP::Config->new(); # IPP Configuration

    my $missing_tools;

    my $pstampdump  = can_run('pstampdump') or (warn "Can't find pstampdump" and $missing_tools = 1);
    my $fields  = can_run('fields') or (warn "Can't find fields" and $missing_tools = 1);

    if ($missing_tools) {
        warn("Can't find required tools.");
        exit ($PS_EXIT_CONFIG_ERROR);
    }

    # Parser for metadata config files
    my $mdcParser = PS::IPP::Metadata::Config->new;

    #
    # get the data from the extension header
    #
    my $fields_output;
    {
        my $command = "echo $request_file_name | $fields -x 0 EXTNAME EXTVER REQ_NAME ACTION EMAIL";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        $fields_output = join "", @$stdout_buf;
    }
    my (undef, $extname, $extver, $req_name, $action, $username, $email) = split " ", $fields_output;

    # make sure the file contains what we are expecting

    die "$request_file_name is not a PS1_PS_REQUEST" 
                    if !$extname or ($extname ne "PS1_PS_REQUEST");
    die "REQ_NAME not found in $request_file_name"  if (!$req_name);
    die "wrong EXTVER $extver found in $request_file_name" if ($extver ne "2" and $extver ne "1");

    my %header;
    $header{REQ_NAME} = $req_name;
    $header{EXTVER}   = $extver;
    $header{EXTNAME}  = $extname;
    if ($extver > 1) {
        $header{ACTION} = $action;
        $header{EMAIL} = $email;
    } else {
        $header{ACTION} = $action = "PROCESS";
        $header{EMAIL} = 'null';
    }

    if ($action ne "PROCESS" and $action ne 'PREVIEW') {
        die "\nunexpected request ACTION found: $action in $request_file_name";
    }


    #
    # now convert the request table to an array of hashes
    #

    my $rows;
    {
        my $command = "$pstampdump $request_file_name";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            print STDERR @$stderr_buf;
        }
        if (@$stdout_buf) {
            $rows = parse_md_fast($mdcParser, join "", @$stdout_buf);
        } else {
            print STDERR "$request_file_name contains empty request table\n";
        }
    }

    my %req_specs;
    foreach my $row (@$rows) {
        my $rownum   = $row->{ROWNUM};
        $req_specs{$rownum} = $row;

        my $job_type = $row->{JOB_TYPE};
        my $project  = $row->{PROJECT};
        my $req_type = $row->{REQ_TYPE};
        my $img_type = $row->{IMG_TYPE};
        my $id       = $row->{ID};
        my $class_id = $row->{CLASS_ID};
        my $stamp_name   = $row->{STAMP_NAME};
        my $filter   = $row->{REQFILT};
        my $mjd_min = $row->{MJD_MIN};
        my $mjd_max = $row->{MJD_MAX};
        my $x = $row->{CENTER_X};
        my $y = $row->{CENTER_Y};
        my $w = $row->{WIDTH};
        my $h = $row->{HEIGHT};
        my $coord_mask = $row->{COORD_MASK};
        my $option_mask= $row->{OPTION_MASK};

        print "$rownum $req_type $img_type $id\n" if $verbose;
    }
    return (\%header, \%req_specs);
}

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

sub get_error_string {
    my $error_code = shift;
    my $error_string;
    if ($error_code >= 0) {
        $error_string = $errorStrings[$error_code];
    }
    $error_string = "unknown" if !$error_string;
    return $error_string;
}

1;
