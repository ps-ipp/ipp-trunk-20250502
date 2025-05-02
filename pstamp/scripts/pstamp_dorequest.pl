#!/bin/env perl
###
### pstamp_dorequest.pl
###
###     Excecute the jobs for a given request.
###
###     Note: This program is not part of the postage stamp server
###     It is intended for testing outside of pantasks environment
###

use warnings;
use strict;

if (@ARGV != 1) {
    die "usage: $0 request_id\n";
}

my $request_id = $ARGV[0];

my $verbosity = 0;

use Sys::Hostname;
my $host = hostname();

if ($verbosity) {
    print "\n\n";
    print "Starting script $0 on $host\n\n";
}

use IPC::Cmd 0.36 qw( can_run run );

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw($PS_EXIT_SUCCESS
		       $PS_EXIT_UNKNOWN_ERROR
		       $PS_EXIT_SYS_ERROR
		       $PS_EXIT_CONFIG_ERROR
		       $PS_EXIT_PROG_ERROR
		       $PS_EXIT_DATA_ERROR
		       $PS_EXIT_TIMEOUT_ERROR
		       metadataLookupStr
		       metadataLookupBool
		       caturi
		       );

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $ppstamp_run = can_run('ppstamp_run.pl') or (warn "Can't find ppstamp_run.pl" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my @psjobs;
#Look up the jobs for the given request_id
{
    my $command = "$pstamptool -pendingjob";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbosity);
    unless ($success) {
        die("Unable to perform pstamptool -pendingreq: $error_code");
    }

    if (@$stdout_buf == 0) {
        print STDERR "no pending pstamp jobs found\n";
        exit 1;
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        die("Unable to parse metdata config doc");

    my $jobs = parse_md_list($metadata);

    foreach my $job (@$jobs) {
        if ($job->{req_id} == $request_id) {
            # print STDERR "adding $job->{job_id} to the list\n";
            $psjobs[@psjobs] = $job;
        }
    }
}

if (! @psjobs) {
    # TODO: is this always an error, what if the job is no longer pending?
    print STDERR "no pending postage stamp jobs for request $request_id found\n";
    exit 1;
}

foreach my $job (@psjobs) {
    my $command = "$ppstamp_run $job->{job_id}";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbosity);
    unless ($success) {
        die("Unable to perform $command: $error_code");
    }
    ### print @$stdout_buf;
}

#
# Update the state of the request
#
{
    ## TODO: what about request status
    my $command = "$pstamptool -updatereq -req_id $request_id -set_state stop"; 
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbosity);
    unless ($success) {
        die("Unable to perform pstamptool -updatereq: $error_code");
    }
}

exit 0;
