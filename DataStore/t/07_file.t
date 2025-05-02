#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 07_file.t,v 1.5 2006-03-17 23:54:35 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::More tests => 14;

=head1 NAME

t/07_file.t - tests DataStore::File

=head1 SYNOPSIS
    
    prove t/07_file.t

=cut

use DataStore::File;
use File::Temp ();

can_ok('DataStore::File', qw(
    new
    request
));

# ->new()

{
    my $dsf = DataStore::File->new(
        uri         => 'http://example.org/foo',
        fileid      => '12buckelyourshoe',
        bytes       => 12345,
        md5sum      => 'fe6a2b6564c0d4cfb3bbf1db813824ba',
        type        => 'chip',
    );

    isa_ok($dsf, 'DataStore::File');
}

eval {
    my $dsf = DataStore::File->new(
        uri         => 'http://example.org/foo',
        fileid      => '12buckelyourshoe',
        bytes       => 12345,
        md5sum      => 'fe6a2b6564c0d4cfb3bbf1db813824ba',
        type        => 'chip',
        foo         => 1,
    );
};
like($@, qr/not listed in the validation options/,
    '->new() fails whe passed extra params');

eval {
    my $dsf = DataStore::File->new( uri => 'http://example.org/' );
};
like($@, qr/is valid uri filename/,
    '->new() fails when uri param does not end with /');

eval {
    my $dsf = DataStore::File->new( uri => '://example.org' );
};
like($@, qr/is valid http uri/,
    '->new() fails when uri param is not http protocol');

eval {
    my $dsf = DataStore::File->new;
};
like($@, qr/Mandatory parameter/,
    '->new() fails when not passed all manadator params');

# ->request()

use Net::HTTPServer;

sub bar 
{
    my $req = shift;             # Net::HTTPServer::Request object
    my $res = $req->Response();  # Net::HTTPServer::Response object

    my $string = "foobarbaz"; 

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

    $server->RegisterURL('/somefile', \&bar);
    $server->Process();  # Run forever

    exit -1;
}

my $port = <$child>;
chomp $port;

{
    my $dsf = DataStore::File->new(
        uri         => "http://localhost:$port/somefile",
        fileid      => '12buckelyourshoe',
        bytes       => 10,
        md5sum      => 'a0a6e1a375117c58d77221f10c5ce12e',
        type        => 'chip',
    );

    my $fh = File::Temp->new( UNLINK => 1 );
    $fh->autoflush(1);
    isa_ok($dsf->request( filename => $fh->filename ), 'DataStore::Response');
}

{
    my $dsf = DataStore::File->new(
        uri         => "http://localhost:$port/somefile",
        fileid      => '12buckelyourshoe',
        bytes       => 10,
        md5sum      => 'a0a6e1a375117c58d77221f10c5ce12e',
        type        => 'chip',
    );

    my $fh = File::Temp->new( UNLINK => 1 );
    $fh->autoflush(1);
    my $dsr = $dsf->request( filename => $fh->filename );

    ok($dsr->is_success, "response->is_success() is correct");
    is($dsr->code, 200, "response->code() is correct");
    is($dsr->status_line, "200 OK", "response->status_line() is correct");
    is($dsr->data, $fh->filename, "response->data is arrayref is correct");
    isa_ok($dsr->request, 'DataStore::Record');
}

# stop HTTP server
kill 9, $pid;

eval {
    my $dsf = DataStore::File->new(
        uri         => 'http://example.org/foo',
        fileid      => '12buckelyourshoe',
        bytes       => 12345,
        md5sum      => 'fe6a2b6564c0d4cfb3bbf1db813824ba',
        type        => 'chip',
    );
    
    $dsf->request;
};
like($@, qr/Mandatory parameter/,
    '->request() fails when passed no params');

eval {
    my $dsf = DataStore::File->new(
        uri         => 'http://example.org/foo',
        fileid      => '12buckelyourshoe',
        bytes       => 12345,
        md5sum      => 'fe6a2b6564c0d4cfb3bbf1db813824ba',
        type        => 'chip',
    );
    
    my $fh = File::Temp->new( UNLINK => 1 );
    $dsf->request(
        filename => $fh->filename,
        foo => 1,
    );
};
like($@, qr/not listed in the validation options/,
    '->request() fails whe passed extra params');
