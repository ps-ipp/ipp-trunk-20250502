#!/bin/env perl
#
# pstamp_queue_reqests.pl
#
# Query registered data stores for new postage stamp requests and add them to
# the database of pending requests.
#

use warnings;
use strict;

use Getopt::Long qw( GetOptions );

use Sys::Hostname;
my $host = hostname();

my $verbose;
my $dbname;
my $dbserver;
my $limit;
my $timeout = 10;

GetOptions(
    'timeout=i'     =>  \$timeout,
    'verbose'       =>  \$verbose,
    'dbname=s'      =>  \$dbname,
    'dbserver=s'    =>  \$dbserver,
    'limit=i'       =>  \$limit,
);

if ($verbose) {
    print STDERR "\n\n";
    print STDERR "Starting script $0 on $host\n\n";
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
my $dsproductls = can_run('dsproductls') or (warn "Can't find dsproductls" and $missing_tools = 1);
my $dsfilesetls = can_run('dsfilesetls') or (warn "Can't find dsfilesetls" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

my $ipprc = PS::IPP::Config->new();
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

if (!$dbserver) {
    $dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

my @dataStores;
#Look up the list of data stores that are ready to be queried
{
    my $command = "$pstamptool -datastore -ready";
    $command .= " -dbname $dbname" if $dbname;
    $command .= " -dbserver $dbserver" if $dbserver;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        my $rc = $error_code >> 8;
        die("Unable to perform pstamptool -datastore: $rc");
    }

    if (@$stdout_buf == 0) {
        print STDERR "no data stores ready\n" if $verbose;
        exit 0;
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        die("Unable to parse metdata config doc");

    my $ds = parse_md_list($metadata);

    @dataStores = @$ds;
}

if (! @dataStores) {
    print STDERR "no postage stamp data stores found\n" if $verbose;
    exit 1;
}

# loop over the data stores
foreach my $ds (@dataStores) {
    my $lastFileset;

    # skip any data stores that aren't enabled

    if ($ds->{state} ne "enabled") {
        next;
    }

    my $outProduct = $ds->{outProduct};
    my $ds_id = $ds->{ds_id};
    my $ds_label = $ds->{label};
    $ds_label = undef if $ds_label eq "NULL";
    my @lines;
    {
        my $command = "$dsproductls --uri $ds->{uri}/index.txt";
        $command .= " --last_fileset $ds->{lastFileset}" if $ds->{lastFileset};
        $command .= " --timeout $timeout";

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            # dsproductls exit status is the http error code - 300
            my $exit_status = $error_code >> 8;

            # don't die on "common faults"
            # perhaps after some number of these we should mark the data store as disabled
            if (($exit_status == 200) or ($exit_status == 104)) {
                ## now update the last_fileset column in pstampDataStore
                update_ds_timestamp($ds_id, $dbname, $dbserver);
                next;
            }


            die("Unable to perform $command: $error_code");
        }

        if (@$stdout_buf == 0) {
            print STDERR "no new request files in data store $ds_id\n" if $verbose;
            update_ds_timestamp($ds_id, $dbname, $dbserver);
            next; # next data store
        }
        my $out_buf = join("", @$stdout_buf);
        # split raw output into lines
        @lines = split /^/, $out_buf;
    }

    #
    # each line contains a fileset
    #

    # number that we've processed
    my $numFilesets = 0;
    foreach my $line (@lines) {
        # parse the line into fields split by whitespace
        my ($uri, $fs_name, $date, $type) = split " ", $line;

        # skip comment lines
        next if ( $uri =~ /^#.*/);

        $numFilesets++;

        my @files;
        {
            my $command = "$dsfilesetls --uri $uri";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                # we don't want to die here. We need to set lastFileset
                # so that we don't re queue requsts that we have proccessed
                die("Unable to perform $command: $error_code");
            }

            if (@$stdout_buf == 0) {
                print STDERR "no file sets found\n";
                next;
            }
            my $out_buf = join "", @$stdout_buf;
            @files = split /^/, $out_buf;
        }

        #
        # For each file in the fileset add a request
        #
        foreach my $file (@files) {
            my ($req_uri, $fn, $size, $md5sum, $type, $chipname) = split " ", $file;

            # skip comment lines
            next if $req_uri =~ (/^#.*/);
            {
                my $command = "$pstamptool -addreq -uri $req_uri -ds_id $ds_id";
                $command .= " -label $ds_label" if $ds_label;
                $command .= " -dbname $dbname" if $dbname;
                $command .= " -dbserver $dbserver" if $dbserver;

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);

                unless ($success) {
                    die("Unable to perform $command: $error_code");
                }
            }
        }
        $lastFileset = $fs_name;

        {
        ## now update the last_fileset column in pstampDataStore
        my $command = "$pstamptool -ds_id $ds_id -moddatastore -set_last_fileset $lastFileset";
        $command .= " -dbname $dbname" if $dbname;
        $command .= " -dbserver $dbserver" if $dbserver;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
            unless ($success) {
                die("Unable to perform pstamptool -moddatastore: $error_code");
            }
        }
        last if ($numFilesets >= $limit);
    }
}

exit 0;

sub update_ds_timestamp {
    my $ds_id = shift;
    my $dbname = shift;
    my $dbserver = shift;

    my $command = "$pstamptool -ds_id $ds_id -moddatastore -update_timestamp";
                $command .= " -dbname $dbname" if $dbname;
                $command .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform pstamptool -moddatastore: $error_code");
    }
}
