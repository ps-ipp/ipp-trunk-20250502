sub test_prog
{
    my ($neb, $key) = @_;

    while (1) {
#    print $sock "$$ : i'm a little tea pot using key: $key\n";
        my $fh = $neb->open_create( $key )
            or child_die($sock, "can't create file $key");
        close $fh;

        $fh = $neb->open( $key, 'read' )
            or child_die("can't open file");
        close $fh;

        $neb->lock( $key, 'read' );
        $neb->unlock( $key, 'read' );
        $neb->replicate( $key );
        $neb->cull( $key );
        $neb->find( $key );
        $neb->copy( $key, $key . "_copy" );
        $neb->move( $key, $key . "_move" );
        $neb->delete( $key . "_copy" );
        $neb->delete( $key . "_move" );

#    print $sock "$$ : all done!\n";
    }

    return 1;
}

1;
