#!/usr/bin/env perl

use strict;
use warnings FATAL => qw( all );

use lib "./lib";

use Benchmark qw( timethese );
use Nebulous::Client;

my $neb = Nebulous::Client->new(
    proxy   => 'http://localhost:80/nebulous'
);

my $key = shift || 'foobar';

eval { $neb->delete( $key ); };

my $fh = $neb->create( $key );
close $fh;


timethese( -3,
    {
        'stat' => sub {
            $neb->stat( $key );
        },
    }
);

$neb->delete( $key );

timethese( -3,
    {
        'create/delete' => sub {
            my $fh = $neb->create( $key );
            close $fh;
            $neb->delete( $key );
        },
    }
);
