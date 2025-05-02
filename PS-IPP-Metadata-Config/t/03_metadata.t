#!/usr/bin/perl

# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: 03_metadata.t,v 1.6 2006-11-25 01:15:47 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./lib );

#$::RD_TRACE = 1;

use Test::More tests => 13;
use PS::IPP::Metadata::Config;

my $config_parser = PS::IPP::Metadata::Config->new;

{
my $example = q{
CELL      METADATA
 EXTNAME   STR   CCD00
 BIASSEC   STR   BSEC-00
 CHIP      STR   CHIP.00
 NCELL     S32   24
END
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "SDRS example parsed");

    my $tree = [
        {
            name    => 'CELL',
            class   => 'metadata',
            value   => [
                {
                    name    => 'EXTNAME',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'CCD00'
                },
                {
                    name    => 'BIASSEC',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'BSEC-00'
                },
                {
                    name    => 'CHIP',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'CHIP.00'
                },
                {
                    name    => 'NCELL',
                    class   => 'scalar',
                    type    => 'S32',
                    value   => 24,
                },
            ],
        }
    ];

    is_deeply( $config, $tree, "SDRS example structure");
}

{
my $example = q{
CELL      METADATA          # foo
 EXTNAME   STR   CCD00      # bar
 BIASSEC   STR   BSEC-00    # baz 
 CHIP      STR   CHIP.00    # zab
 NCELL     S32   24         # rab
END                         # oof
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "SDRS example + comments parsed");

    my $tree = [
        {
            name    => 'CELL',
            class   => 'metadata',
            value   => [
                {
                    name    => 'EXTNAME',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'CCD00',
                    comment => 'bar',
                },
                {
                    name    => 'BIASSEC',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'BSEC-00',
                    comment => 'baz',
                },
                {
                    name    => 'CHIP',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'CHIP.00',
                    comment => 'zab',
                },
                {
                    name    => 'NCELL',
                    class   => 'scalar',
                    type    => 'S32',
                    value   => 24,
                    comment => 'rab',
                },
            ],
            comment => 'foo',
        }
    ];

    is_deeply( $config, $tree, "SDRS example + comments structure");
}

{
my $example = q{
CELL      METADATA
    FOO METADATA
        BAR     STR BAZ
        PING    STR PONG
    END
    
    EXTNAME   STR   CCD00
    BIASSEC   STR   BSEC-00
    CHIP      STR   CHIP.00
    NCELL     S32   24
END
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "nested metadata parsed");

    my $tree = [
        {
            name    => 'CELL',
            class   => 'metadata',
            value   => [
                {
                    name    => 'FOO',
                    class   => 'metadata',
                    value   => [
                        {
                            name    => 'BAR',
                            class   => 'scalar',
                            type    => 'STR',
                            value   => 'BAZ',
                        },
                        {
                            name    => 'PING',
                            class   => 'scalar',
                            type    => 'STR',
                            value   => 'PONG',
                        },
                    ],
                },
                {
                    name    => 'EXTNAME',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'CCD00',
                },
                {
                    name    => 'BIASSEC',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'BSEC-00',
                },
                {
                    name    => 'CHIP',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'CHIP.00',
                },
                {
                    name    => 'NCELL',
                    class   => 'scalar',
                    type    => 'S32',
                    value   => 24,
                },
            ],
        }
    ];

    is_deeply( $config, $tree, "nested metadata structure");
}

{
my $example = q{
FOO1 METADATA
    FOO2 METADATA
        FOO3 METADATA
            FOO4 METADATA
                FOO5 METADATA
                    FOO6 METADATA
                        BAR     STR BAZ
                        PING    STR PONG
                    END
                    BAR     STR BAZ
                    PING    STR PONG
                END
                BAR     STR BAZ
                PING    STR PONG
            END
            BAR     STR BAZ
            PING    STR PONG
        END
        BAR     STR BAZ
        PING    STR PONG
    END
    BAR     STR BAZ
    PING    STR PONG
END
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "deeply nested metadata");

    my $tree = [
        {
            name    => 'FOO1',
            class   => 'metadata',
            value   => [
                {
                    name    => 'FOO2',
                    class   => 'metadata',
                    value   => [
                        {
                            name    => 'FOO3',
                            class   => 'metadata',
                            value   => [
                                {
                                    name    => 'FOO4',
                                    class   => 'metadata',
                                    value   => [
                                        {
                                            name    => 'FOO5',
                                            class   => 'metadata',
                                            value   => [
                                                {
                                                    name    => 'FOO6',
                                                    class   => 'metadata',
                                                    value   => [
                                                        {
                                                            name    => 'BAR',
                                                            class   => 'scalar',
                                                            type    => 'STR',
                                                            value   => 'BAZ',
                                                        },
                                                        {
                                                            name    => 'PING',
                                                            class   => 'scalar',
                                                            type    => 'STR',
                                                            value   => 'PONG',
                                                        },
                                                    ],
                                                },
                                                {
                                                    name    => 'BAR',
                                                    class   => 'scalar',
                                                    type    => 'STR',
                                                    value   => 'BAZ',
                                                },
                                                {
                                                    name    => 'PING',
                                                    class   => 'scalar',
                                                    type    => 'STR',
                                                    value   => 'PONG',
                                                },
                                            ],
                                        },
                                        {
                                            name    => 'BAR',
                                            class   => 'scalar',
                                            type    => 'STR',
                                            value   => 'BAZ',
                                        },
                                        {
                                            name    => 'PING',
                                            class   => 'scalar',
                                            type    => 'STR',
                                            value   => 'PONG',
                                        },
                                    ],
                                },
                                {
                                    name    => 'BAR',
                                    class   => 'scalar',
                                    type    => 'STR',
                                    value   => 'BAZ',
                                },
                                {
                                    name    => 'PING',
                                    class   => 'scalar',
                                    type    => 'STR',
                                    value   => 'PONG',
                                },
                            ],
                        },
                        {
                            name    => 'BAR',
                            class   => 'scalar',
                            type    => 'STR',
                            value   => 'BAZ',
                        },
                        {
                            name    => 'PING',
                            class   => 'scalar',
                            type    => 'STR',
                            value   => 'PONG',
                        },
                    ],
                },
                {
                    name    => 'BAR',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'BAZ',
                },
                {
                    name    => 'PING',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'PONG',
                },
            ],
        },
    ];

    is_deeply( $config, $tree, "nested metadata structure");
}

{
my $example = q{
FOO1 METADATA
    BAR     STR BAZ
    PING    STR PONG
    FOO2 METADATA
        BAR     STR BAZ
        PING    STR PONG
        FOO3 METADATA
            BAR     STR BAZ
            PING    STR PONG
            FOO4 METADATA
                BAR     STR BAZ
                PING    STR PONG
                FOO5 METADATA
                    BAR     STR BAZ
                    PING    STR PONG
                    FOO6 METADATA
                        BAR     STR BAZ
                        PING    STR PONG
                    END
                END
            END
        END
    END
END
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "deeply nested metadata");
}

{
my $example = q{
FOO1 METADATA
    BAR     STR BAZ
    FOO2 METADATA
        BAR     STR BAZ
        FOO3 METADATA
            BAR     STR BAZ
            FOO4 METADATA
                BAR     STR BAZ
                FOO5 METADATA
                    BAR     STR BAZ
                    FOO6 METADATA
                        BAR     STR BAZ
                        PING    STR PONG
                    END
                    PING    STR PONG
                END
                PING    STR PONG
            END
            PING    STR PONG
        END
        PING    STR PONG
    END
    PING    STR PONG
END
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "deeply nested metadata");
}

{
my $example = q{
FOO1 METADATA
    FOO2 METADATA
        BAR     STR BAZ
        PING    STR PONG
    END
    FOO3 METADATA
        BAR     STR BAZ
        PING    STR PONG
    END
END
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "two metadata at the same depth");
}

{
my $example = q{
foo METADATA
END
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );

    ok( defined( $config ), "empty METADATA");

    my $tree = [
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [],
        },
    ];

    is_deeply( $config, $tree, "empty METADATA structure");
}
