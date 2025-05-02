#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 06_fileset.t,v 1.9 2007-09-25 23:50:34 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::More tests => 19;

=head1 NAME

t/06_fileset.t - tests DataStore::FileSet

=head1 SYNOPSIS
    
    prove t/06_fileset.t

=cut

use DataStore::FileSet;

can_ok('DataStore::FileSet', qw(
    new
    request
));

# ->new()

{
    my $dsf = DataStore::FileSet->new(
        uri         => 'http://example.org/index.txt',
        fileset     => '12buckelyourshoe',
        datetime    => '2042-01-01T00:00:00Z',
        type        => 'OBJECT',
    );

    isa_ok($dsf, 'DataStore::FileSet');
}

eval {
    my $dsf = DataStore::FileSet->new(
        uri         => 'http://example.org/index.txt',
        fileset     => '12buckelyourshoe',
        datetime    => '2042-01-01T00:00:00Z',
        type        => 'OBJECT',
        foo         => 1,
    );
};
like($@, qr/not listed in the validation options/,
    '->new() fails whe passed extra params');

eval {
    my $dsf = DataStore::FileSet->new( uri => 'http://example.org' );
};
like($@, qr/uri ends with \/index.txt/,
    '->new() fails when uri param does not end with /');

eval {
    my $dsf = DataStore::FileSet->new( uri => 'http://example.org/index.html' );
};
like($@, qr/uri ends with \/index.txt/,
    '->new() fails when uri param does not end with /');

eval {
    my $dsf = DataStore::FileSet->new( uri => '://example.org/index.txt' );
};
like($@, qr/is valid http uri/,
    '->new() fails when uri param is not http protocol');

eval {
    my $dsp = DataStore::FileSet->new;
};
like($@, qr/Mandatory parameter/,
    '->new() fails when not passed all manadator params');

# ->request()

use Net::HTTPServer;

sub bar 
{
    my $req = shift;             # Net::HTTPServer::Request object
    my $res = $req->Response();  # Net::HTTPServer::Response object

    my $string = <<END;
# fileID      |bytes   |md5sum                          |type|chipname|
otis0123456.01|83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|chip|ota01   |
otis0123456.02|83002312|a2b4a7d7b94dc6076c5f3f67239a48c6|chip|ota02   |
otis0123456.03|83002312|a39b1510484c7833e27454b181f13981|chip|ota03   |
otis0123456.04|83002312|7bb35e1e30f3f833c0416aea75f90304|chip|ota04   |
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
    my $dsp = DataStore::FileSet->new(
        uri         => "http://localhost:$port/index.txt",
        fileset     => '12buckelyourshoe',
        datetime    => '2042-01-01T00:00:00Z',
        type        => 'OBJECT',
    );

    isa_ok($dsp->request, 'DataStore::Response');
}

{
    my $dsp = DataStore::FileSet->new(
        uri         => "http://localhost:$port/index.txt",
        fileset     => '12buckelyourshoe',
        datetime    => '2042-01-01T00:00:00Z',
        type        => 'OBJECT',
    );

    my $dsr = $dsp->request;

    ok($dsr->is_success, "response->is_success() is correct");
    is($dsr->code, 200, "response->code() is correct");
    is($dsr->status_line, "200 OK", "response->status_line() is correct");
    is(ref $dsr->data, 'ARRAY', "response->data is arrayref is correct");
    isa_ok($dsr->request, 'DataStore::Record');

    my $data = $dsr->data;
    is(scalar @$data, 4, "data has the correct number of rows");
    isa_ok(@$data[0], 'DataStore::File', "data has correct type of rows");
    isa_ok(@$data[1], 'DataStore::File', "data has correct type of rows");
    isa_ok(@$data[2], 'DataStore::File', "data has correct type of rows");
    isa_ok(@$data[3], 'DataStore::File', "data has correct type of rows");
}

# cleanup HTTP server
kill 9, $pid;

eval {
    my $dsf = DataStore::FileSet->new(
        uri         => 'http://example.org/index.txt',
        fileset     => '12buckelyourshoe',
        datetime    => '2042-01-01T00:00:00Z',
        type        => 'OBJECT',
    );

    $dsf->request( foo => 1 );
};
like($@, qr/not listed in the validation options/,
    '->request() fails whe passed extra params');

