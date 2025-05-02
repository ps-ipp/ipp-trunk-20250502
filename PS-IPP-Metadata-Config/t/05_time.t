#!/usr/bin/perl

# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: 05_time.t,v 1.5 2006-11-22 23:54:50 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./lib );

#$::RD_TRACE = 1;

use Test::More tests => 15;
use PS::IPP::Metadata::Config;

my $config_parser = PS::IPP::Metadata::Config->new;

{
my $example =<<END;
mytime  MULTI
mytime  UTC     NULL
mytime  UT1     NULL
mytime  TAI     NULL
mytime  TT      NULL
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "NULL times parsed");

    my $tree = [
        {
            name    => 'mytime',
            class   => 'scalar',
            type    => 'UTC',
            value   => undef,
        },
        {
            name    => 'mytime',
            class   => 'scalar',
            type    => 'UT1',
            value   => undef,
        },
        {
            name    => 'mytime',
            class   => 'scalar',
            type    => 'TAI',
            value   => undef,
        },
        {
            name    => 'mytime',
            class   => 'scalar',
            type    => 'TT',
            value   => undef,
        },
    ];

    is_deeply( $config, $tree, "NULL times structure" );
}

{
my $example =<<END;
mytime  MULTI
mytime  UTC     NULL    # comment
mytime  UT1     NULL    # comment
mytime  TAI     NULL    # comment
mytime  TT      NULL    # comment
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "NULL times with comments parsed");

    my $tree = [
        {
            name    => 'mytime',
            class   => 'scalar',
            type    => 'UTC',
            value   => undef,
            comment => 'comment',
        },
        {
            name    => 'mytime',
            class   => 'scalar',
            type    => 'UT1',
            value   => undef,
            comment => 'comment',
        },
        {
            name    => 'mytime',
            class   => 'scalar',
            type    => 'TAI',
            value   => undef,
            comment => 'comment',
        },
        {
            name    => 'mytime',
            class   => 'scalar',
            type    => 'TT',
            value   => undef,
            comment => 'comment',
        },
    ];

    is_deeply( $config, $tree, "NULL times with comments structure" );
}

{
my $example =<<END;
recently    MULTI
recently    UTC     2005-03-18T16:05:00
recently    UT1     2005-03-18T16:05:00
recently    TAI     2005-03-18T16:05:00
recently    TT      2005-03-18T16:05:00
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "basic ISO8601 parsed");

    my $tree = [
        {
            name    => 'recently',
            class   => 'scalar',
            type    => 'UTC',
            value   => DateTime::Format::ISO8601->parse_datetime("2005-03-18T16:05:00Z"),
        },
        {
            name    => 'recently',
            class   => 'scalar',
            type    => 'UT1',
            value   => DateTime::Format::ISO8601->parse_datetime("2005-03-18T16:05:00Z"),
        },
        {
            name    => 'recently',
            class   => 'scalar',
            type    => 'TAI',
            value   => DateTime::Format::ISO8601->parse_datetime("2005-03-18T16:05:00Z"),
        },
        {
            name    => 'recently',
            class   => 'scalar',
            type    => 'TT',
            value   => DateTime::Format::ISO8601->parse_datetime("2005-03-18T16:05:00Z"),
        },
    ];

    is_deeply( $config, $tree, "basic ISO8601 structure" );
}

{
my $example =<<END;
recently    MULTI
recently    UTC     2005-03-18T16:05:00Z
recently    UT1     2005-03-18T16:05:00Z
recently    TAI     2005-03-18T16:05:00Z
recently    TT      2005-03-18T16:05:00Z
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "basic ISO8601 w/Z parsed");

    my $tree = [
        {
            name    => 'recently',
            class   => 'scalar',
            type    => 'UTC',
            value   => DateTime::Format::ISO8601->parse_datetime("2005-03-18T16:05:00Z"),
        },
        {
            name    => 'recently',
            class   => 'scalar',
            type    => 'UT1',
            value   => DateTime::Format::ISO8601->parse_datetime("2005-03-18T16:05:00Z"),
        },
        {
            name    => 'recently',
            class   => 'scalar',
            type    => 'TAI',
            value   => DateTime::Format::ISO8601->parse_datetime("2005-03-18T16:05:00Z"),
        },
        {
            name    => 'recently',
            class   => 'scalar',
            type    => 'TT',
            value   => DateTime::Format::ISO8601->parse_datetime("2005-03-18T16:05:00Z"),
        },
    ];

    is_deeply( $config, $tree, "basic ISO8601 w/Z structure" );
}

{
my $example =<<END;
recently    MULTI
recently    UTC     2005-03-18T16:05:00.000001Z
recently    UT1     2005-03-18T16:05:00.000001Z
recently    TAI     2005-03-18T16:05:00.000001Z
recently    TT      2005-03-18T16:05:00.000001Z
END
    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "basic IS8601 with fractional seconds parsed");
}

{
my $example =<<END;
recently    MULTI
recently    UTC     2005-03-18T16:05:00Z    # foo
recently    UT1     2005-03-18T16:05:00Z    # bar
recently    TAI     2005-03-18T16:05:00Z    # baz
recently    TT      2005-03-18T16:05:00Z    #
END
    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "ISO8601 with comments parsed");
}

{
my $example =<<END;
recently    MULTI
recently    UTC     2005-03-18T16:05:00.000001Z    # foo
recently    UT1     2005-03-18T16:05:00.000001Z    # bar
recently    TAI     2005-03-18T16:05:00.000001Z    # baz
recently    TT      2005-03-18T16:05:00.000001Z    #
END
    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "ISO8601 with comments and fractional seconds parsed");
}

#diag("epoch time format maybe deprecated");
#
#{
#my $example =<<END;
#recently    MULTI
#recently    UTC     123456, 5000, 1
#recently    UT1     123456, 5000
#recently    TAI     123456, 5000
#recently    TT      123456, 5000
#END
#    my $config = $config_parser->parse( $example );
#    ok( defined( $config ), "basic epoch time parsed");
#}
#
#{
#my $example =<<END;
#recently    MULTI
#recently    UTC     123456, 5000, 1         # foo
#recently    UT1     123456, 5000            # bar
#recently    TAI     123456, 5000            # baz
#recently    TT      123456, 5000            #
#END
#    my $config = $config_parser->parse( $example );
#    ok( defined( $config ), "epoch time with comments parsed");
#}

{
my $example =<<END;
broken      UTC     2005-03-18 16:05:00
END
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "bad format");
}

{
my $example =<<END;
broken      UT1     2005-03-18 16:05:00
END
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "bad format");
}

{
my $example =<<END;
broken      TAI     2005-03-18 16:05:00
END
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "bad format");
}

{
my $example =<<END;
broken      TT      2005-03-18 16:05:00
END
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "bad format");
}
