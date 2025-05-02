#!/usr/bin/perl

# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: 07_floats.t,v 1.1 2006-09-23 00:26:50 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./lib );

#$::RD_TRACE = 1;

use Test::More tests => 12;
use PS::IPP::Metadata::Config;

my $config_parser = PS::IPP::Metadata::Config->new;

{
my $example =<<END;
mynan   MULTI
mynan   F32     nan
mynan   F32     NaN
mynan   F32     nAn
mynan   F32     NAN
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "F32 nans parsed");

    my $tree = [
        {
            name    => 'mynan',
            class   => 'scalar',
            type    => 'F32',
            value   => 'nan',
        },
        {
            name    => 'mynan',
            class   => 'scalar',
            type    => 'F32',
            value   => 'nan',
        },
        {
            name    => 'mynan',
            class   => 'scalar',
            type    => 'F32',
            value   => 'nan',
        },
        {
            name    => 'mynan',
            class   => 'scalar',
            type    => 'F32',
            value   => 'nan',
        },
    ];

    is_deeply( $config, $tree, "F32 nans structure" );
}

{
my $example =<<END;
mynan   MULTI
mynan   F64     nan
mynan   F64     NaN
mynan   F64     nAn
mynan   F64     NAN
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "F64 nans parsed");

    my $tree = [
        {
            name    => 'mynan',
            class   => 'scalar',
            type    => 'F64',
            value   => 'nan',
        },
        {
            name    => 'mynan',
            class   => 'scalar',
            type    => 'F64',
            value   => 'nan',
        },
        {
            name    => 'mynan',
            class   => 'scalar',
            type    => 'F64',
            value   => 'nan',
        },
        {
            name    => 'mynan',
            class   => 'scalar',
            type    => 'F64',
            value   => 'nan',
        },
    ];

    is_deeply( $config, $tree, "F64 nans structure" );
}

my $inftree = [
    # inf
    {
        name    => 'myinf',
        class   => 'scalar',
        type    => 'F32',
        value   => '+inf',
    },
    {
        name    => 'myinf',
        class   => 'scalar',
        type    => 'F32',
        value   => '+inf',
    },
    {
        name    => 'myinf',
        class   => 'scalar',
        type    => 'F32',
        value   => '-inf',
    },
    {
        name    => 'myinf',
        class   => 'scalar',
        type    => 'F32',
        value   => '+inf',
    },
    {
        name    => 'myinf',
        class   => 'scalar',
        type    => 'F32',
        value   => '+inf',
    },
    {
        name    => 'myinf',
        class   => 'scalar',
        type    => 'F32',
        value   => '-inf',
    },
];

{
my $example =<<END;
myinf   MULTI
myinf   F32     inf
myinf   F32     +inf
myinf   F32     -inf
myinf   F32     infinity
myinf   F32     +infinity
myinf   F32     -infinity
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "F32 infs parsed");

    is_deeply( $config, $inftree, "F32 infs structure" );
}

{
my $example =<<END;
myinf   MULTI
myinf   F32     InF
myinf   F32     +InF
myinf   F32     -InF
myinf   F32     InFinity
myinf   F32     +InFinity
myinf   F32     -InFinity
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "F32 InFs parsed");

    is_deeply( $config, $inftree, "F32 InFs structure" );
}

{
my $example =<<END;
myinf   MULTI
myinf   F32     iNf
myinf   F32     +iNf
myinf   F32     -iNf
myinf   F32     iNfinity
myinf   F32     +iNfinity
myinf   F32     -iNfinity
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "F32 iNfs parsed");

    is_deeply( $config, $inftree, "F32 iNfs structure" );
}

{
my $example =<<END;
myinf   MULTI
myinf   F32     INF
myinf   F32     +INF
myinf   F32     -INF
myinf   F32     INFINITY
myinf   F32     +INFINITY
myinf   F32     -INFINITY
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "F32 INFs parsed");

    is_deeply( $config, $inftree, "F32 INFs structure" );
}
