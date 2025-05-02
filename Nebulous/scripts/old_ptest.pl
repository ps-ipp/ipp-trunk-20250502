#!/usr/bin/perl

use strict;
use warnings;

use Benchmark qw( timethese );
use PS::IPP::IData::Client;

my $idata = PS::IPP::IData::Client->new(
    proxy   => 'http://localhost:80/idata'
);

my $key = shift || 'foobar';

timethese( -10,
    {
        'create/delete' => sub {
            my $fh = $idata->create( $key );
            close $fh;
            $idata->delete( $key );
        },
    }
);
