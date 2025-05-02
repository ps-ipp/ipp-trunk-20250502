#!/usr/bin/perl

# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: 06_multi.t,v 1.8 2006-10-11 02:49:05 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./lib );

#$::RD_TRACE = 1;

use Test::More tests => 15;
use PS::IPP::Metadata::Config;

{
my $example = q{
foo MULTI
foo S8      -1
foo STR     bar baz
foo BOOL    T
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    ok( defined( $config ), "basic MULTI");
}

{
my $example = q{
foo MULTI               # foo
foo S8      -1          # bar
foo STR     bar baz     # baz
foo BOOL    T           #
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    ok( defined( $config ), "MULTI with comments");
}

{
my $example = q{
foo MULTI
foo S8      -1
foo STR     bar baz
foo BOOL    T
bar METADATA
    foo MULTI
    foo S8      -1
    foo STR     bar baz
    foo BOOL    T
END
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    ok( defined( $config ), "MULTI not visible in lower scopes");
}

{
my $example = q{
foo MULTI
foo METADATA
    bar BOOL    T
END
foo METADATA
    bar BOOL    T
END
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );

    my $tree = [
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [{
                            name    => 'bar',
                            class   => 'scalar',
                            type    => 'BOOL',
                            value   => 1,
                        }],
        },
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [{
                            name    => 'bar',
                            class   => 'scalar',
                            type    => 'BOOL',
                            value   => 1,
                        }],
        },
    ];

    is_deeply( $config, $tree, "MULTI METADATA structure, declared" );
}

{
my $example = q{
foo METADATA
    bar BOOL    T
END
foo METADATA
    bar BOOL    T
END
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );

    my $tree = [
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [{
                            name    => 'bar',
                            class   => 'scalar',
                            type    => 'BOOL',
                            value   => 1,
                        }],
        },
    ];

    is_deeply( $config, $tree, "MULTI METADATA structure, not declared" );
}

{
my $example = q{
foo MULTI
TYPE bar a b c
foo  bar x y z
foo  bar x y z
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    ok( defined( $config ), "MULTI TYPE");

    my $tree = [
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [
                {
                    name    => 'a',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'x',
                },
                {
                    name    => 'b',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'y',
                },
                {
                    name    => 'c',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'z',
                },
            ],
        },
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [
                {
                    name    => 'a',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'x',
                },
                {
                    name    => 'b',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'y',
                },
                {
                    name    => 'c',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'z',
                },
            ],
        },
    ];

    is_deeply( $config, $tree, "MULTI TYPE structure, not declared" );
}

{
my $example = q{
TYPE bar a b c
foo MULTI
foo  bar x y z
foo  bar x y z
};

    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    ok( defined( $config ), "MULTI TYPE");

    my $tree = [
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [
                {
                    name    => 'a',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'x',
                },
                {
                    name    => 'b',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'y',
                },
                {
                    name    => 'c',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'z',
                },
            ],
        },
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [
                {
                    name    => 'a',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'x',
                },
                {
                    name    => 'b',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'y',
                },
                {
                    name    => 'c',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'z',
                },
            ],
        },
    ];

    is_deeply( $config, $tree, "MULTI TYPE structure, not declared" );
}

{
my $example = q{
TYPE bar a b c
foo  bar x y z
foo  bar x y z
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );

    my $tree = [
        {
            name    => 'foo',
            class   => 'metadata',
            value   => [
                {
                    name    => 'a',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'x',
                },
                {
                    name    => 'b',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'y',
                },
                {
                    name    => 'c',
                    class   => 'scalar',
                    type    => 'STR',
                    value   => 'z',
                },
            ],
        },
    ];

    is_deeply( $config, $tree, "MULTI TYPE structure, not declared" );
}

{
my $example = q{
foo MULTI
bar METADATA
    foo S8      -1
    foo STR     bar baz
END
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    # ok because of overwrite
    ok( defined( $config ), "MULTI not in scope");
}

{
my $example = q{
bar METADATA
    foo MULTI
END
foo S8      -1
foo STR     bar baz
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    # ok because of overwrite
    ok( defined( $config ), "MULTI not in scope");
}

{
my $example = q{
bar METADATA
    foo MULTI
END
baz METADATA
    foo S8      -1
END
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    # ok because of overwrite
    ok( defined( $config ), "MULTI not in scope");
}

{
my $example = q{
foo MULTI
foo MULTI
foo S8      -1
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    ok( !defined( $config ), "MULTI redeclaration");
}

{
my $example = q{
foo S8      -1
foo STR     bar baz
};
    my $config = PS::IPP::Metadata::Config->new->parse( $example );
    # ok because of overwrite
    ok( defined( $config ), "missing MULTI declaration");
}
