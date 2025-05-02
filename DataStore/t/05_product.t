#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 05_product.t,v 1.6 2007-09-25 23:37:44 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::More tests => 23;

=head1 NAME

t/05_product.t - tests DataStore::Product

=head1 SYNOPSIS
    
    prove t/05_product.t

=cut

use DataStore::Product;

can_ok('DataStore::Product', qw(
    new
    request
));

# ->new()

{
    my $dsp = DataStore::Product->new( uri => 'http://example.org/index.txt' );

    isa_ok($dsp, 'DataStore::Product');
}

{
    my $dsp = DataStore::Product->new(
        uri             => 'http://example.org/index.txt',
        last_fileset    => '12buckelyourshoe',
    );

    isa_ok($dsp, 'DataStore::Product');
}

eval {
    my $dsp = DataStore::Product->new( uri => 'http://example.org/index.txt', foo => 1 );
};
like($@, qr/not listed in the validation options/,
    '->new() fails whe passed extra params');

eval {
    my $dsp = DataStore::Product->new(
        uri             => 'http://example.org/index.txt',
        last_fileset    => '12buckelyourshoe',
        foo => 1,
    );
};
like($@, qr/not listed in the validation options/,
    '->new() fails whe passed extra params');

eval {
    my $dsp = DataStore::Product->new( uri => 'http://example.org' );
};
like($@, qr/uri ends with \/index.txt/,
    '->new() fails when uri param does not end with /index.txt');

eval {
    my $dsp = DataStore::Product->new( uri => 'http://example.org/index.html' );
};
like($@, qr/uri ends with \/index.txt/,
    '->new() fails when uri param does not end with /index.txt');

eval {
    my $dsp = DataStore::Product->new( uri => '://example.org/index.txt' );
};
like($@, qr/is valid http uri/,
    '->new() fails when uri param is not http protocol');

eval {
    my $dsp = DataStore::Product->new(
        uri             => 'http://example.org/index.txt',
        last_fileset    => '++12buckelyourshoe',
    );
};
like($@, qr/is valid fileset ID/,
    '->new() fails when last_fileset param is not a valid fileset ID');

eval {
    my $dsp = DataStore::Product->new;
};
like($@, qr/Mandatory parameter 'uri'/,
    '->new() fails when not passed a uri param');

eval {
    my $dsp = DataStore::Product->new(
        last_fileset    => '12buckelyourshoe',
    );
};
like($@, qr/Mandatory parameter 'uri'/,
    '->new() fails when not passed a uri param');

# ->request()

use Net::HTTPServer;

sub bar 
{
    my $req = shift;             # Net::HTTPServer::Request object
    my $res = $req->Response();  # Net::HTTPServer::Response object

    my $string = <<END;
# filesetID|time registered    |type   |telescope pointing         |etime|f|airmass|
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
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
    my $dsp = DataStore::Product->new( uri => "http://localhost:$port/index.txt" );

    isa_ok($dsp->request, 'DataStore::Response');
}

{
    my $dsp = DataStore::Product->new( uri => "http://localhost:$port/index.txt" );

    my $dsr = $dsp->request;

    ok($dsr->is_success, "response->is_success() is correct");
    is($dsr->code, 200, "response->code() is correct");
    is($dsr->status_line, "200 OK", "response->status_line() is correct");
    is(ref $dsr->data, 'ARRAY', "response->data is arrayref is correct");
    isa_ok($dsr->request, 'DataStore::Record');

    my $data = $dsr->data;
    is(scalar @$data, 4, "data has the correct number of rows");
    isa_ok(@$data[0], 'DataStore::FileSet', "data has correct type of rows");
    isa_ok(@$data[1], 'DataStore::FileSet', "data has correct type of rows");
    isa_ok(@$data[2], 'DataStore::FileSet', "data has correct type of rows");
    isa_ok(@$data[3], 'DataStore::FileSet', "data has correct type of rows");
}

# cleanup HTTP server
kill 9, $pid;

eval {
    my $dsp = DataStore::Product->new( uri => 'http://example.org/index.txt' );

    $dsp->request( foo => 1 );
};
like($@, qr/not listed in the validation options/,
    '->request() fails whe passed extra params');
