#!/usr/bin/env perl

use strict;
use warnings FATAL => qw( all );

use lib "./lib";

use Nebulous::Client;

my $neb = Nebulous::Client->new(
    proxy   => 'http://localhost:80/nebulous'
);

my $key = shift || 'foobar';
my $kids = shift || 1;

foreach my $id ( 1..$kids ) {
    my $pid = fork;

    unless ( $pid )  {
        my $fname = "${key}_$id";
        my $fh = $neb->open_create( $fname );
        die "can't create file $fname" unless $fh;

        print $fh "fooby\n";

        close $fh;

        $fh = $neb->open( $fname, 'read' ) or die "can't open file";
        close $fh;

        $neb->lock( $fname, 'read' );
        $neb->unlock( $fname, 'read' );
        $neb->replicate( $fname );
        $neb->cull( $fname );
        $neb->find( $fname );
        $neb->copy( $fname, $fname . "_copy" );
        $neb->move( $fname, $fname . "_move" );
        $neb->delete( $fname . "_copy" );
        $neb->delete( $fname . "_move" );

        exit 0;
    }
}

while ( $kids ) {
    wait();
    $kids --;
}
