#!/usr/bin/env perl

# Copyright (C) 2006-2007  Joshua Hoblitt
#
# $Id: 14_root.t,v 1.1 2007-09-26 00:51:22 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::More tests => 21;

=head1 NAME

t/14_root.t - tests DataStore::Root

=head1 SYNOPSIS
    
    prove t/14_root.t

=cut

use DataStore::Root;

can_ok('DataStore::Root', qw(
    new
    request
));

# ->new()

{
    my $dsp = DataStore::Root->new( uri => 'http://example.org/index.txt' );

    isa_ok($dsp, 'DataStore::Root');
}

eval {
    my $dsp = DataStore::Root->new(
        uri => 'http://example.org/index.txt',
        foo => 1
    );
};
like($@, qr/not listed in the validation options/,
    '->new() fails whe passed extra params');

eval {
    my $dsp = DataStore::Root->new(
        uri             => 'http://example.org/index.txt',
        last_fileset    => '12buckelyourshoe',
        foo => 1,
    );
};
like($@, qr/not listed in the validation options/,
    '->new() fails whe passed extra params');

eval {
    my $dsp = DataStore::Root->new( uri => 'http://example.org' );
};
like($@, qr/uri ends with \/index.txt/,
    '->new() fails when uri param does not end with /index.txt');

eval {
    my $dsp = DataStore::Root->new( uri => 'http://example.org/index.html' );
};
like($@, qr/uri ends with \/index.txt/,
    '->new() fails when uri param does not end with /index.txt');

eval {
    my $dsp = DataStore::Root->new( uri => '://example.org/index.txt' );
};
like($@, qr/is valid http uri/,
    '->new() fails when uri param is not http protocol');

eval {
    my $dsp = DataStore::Root->new;
};
like($@, qr/Mandatory parameter 'uri'/,
    '->new() fails when not passed a uri param');

eval {
    my $dsp = DataStore::Root->new(
        last_fileset    => '12buckelyourshoe',
    );
};
like($@, qr/not listed in the validation options/,
    '->new() fails when not passed a uri param');

# ->request()

use Net::HTTPServer;

sub bar 
{
    my $req = shift;             # Net::HTTPServer::Request object
    my $res = $req->Response();  # Net::HTTPServer::Response object

    my $string = <<END;
# productID | Most recent | Time              |type | Description       |
gpc1|otis_test|2006-08-30T17:41:59Z|image|Gigapixel Camera 1|
skyprobe|o4348i0010o05|2007-09-04T23:00:06Z|image|Skyprobe|
ucam|video0102|2007-01-17T14:48:10Z|image|Microcam|
allskycam|o4367a1075o01|2007-09-24T05:21:44Z|image|All Sky Camera|
END

    $res->Print($string);

    return $res;
}

$|++;

$SIG{CHLD} = 'IGNORE';
my $pid = open(my $child, "-|");

unless ($pid) {
    my $server = new Net::HTTPServer( port => 'scan', log => '/dev/null' );

    # send port number to parent
    print $server->Start(), "\n";

    $server->RegisterURL('/index.txt', \&bar);
    $server->Process();  # Run forever

    exit -1;
}

my $port = <$child>;
chomp $port;

{
    my $dsp = DataStore::Root->new( uri => "http://localhost:$port/index.txt" );

    isa_ok($dsp->request, 'DataStore::Response');
}

{
    my $dsp = DataStore::Root->new( uri => "http://localhost:$port/index.txt" );

    my $dsr = $dsp->request;

    ok($dsr->is_success, "response->is_success() is correct");
    is($dsr->code, 200, "response->code() is correct");
    is($dsr->status_line, "200 OK", "response->status_line() is correct");
    is(ref $dsr->data, 'ARRAY', "response->data is arrayref is correct");
    isa_ok($dsr->request, 'DataStore::Record');

    my $data = $dsr->data;
    is(scalar @$data, 4, "data has the correct number of rows");
    isa_ok(@$data[0], 'DataStore::Product', "data has correct type of rows");
    isa_ok(@$data[1], 'DataStore::Product', "data has correct type of rows");
    isa_ok(@$data[2], 'DataStore::Product', "data has correct type of rows");
    isa_ok(@$data[3], 'DataStore::Product', "data has correct type of rows");
}

# cleanup HTTP server
kill 9, $pid;

eval {
    my $dsp = DataStore::Root->new( uri => 'http://example.org/index.txt' );

    $dsp->request( foo => 1 );
};
like($@, qr/not listed in the validation options/,
    '->request() fails whe passed extra params');
