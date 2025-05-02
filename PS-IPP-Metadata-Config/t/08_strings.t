#!/usr/bin/perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 08_strings.t,v 1.3 2007-07-06 02:27:32 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./lib );

#$::RD_TRACE = 1;

use Test::More tests => 6;
use PS::IPP::Metadata::Config;

my $config_parser = PS::IPP::Metadata::Config->new;

{
my $example =<<END;
mystr   MULTI
mystr   STR     somevalue
mystr   STR     somevaluebeforeacomment             # some comment
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "strs parsed");

    my $tree = [
        {
            name    => 'mystr',
            class   => 'scalar',
            type    => 'STR',
            value   => 'somevalue',
        },
        {
            name    => 'mystr',
            class   => 'scalar',
            type    => 'STR',
            value   => 'somevaluebeforeacomment',
            comment => 'some comment',
        },
    ];

    is_deeply( $config, $tree, "str structure" );
}

{
my $example =<<END;
mystr   MULTI
mystr   STR     NULL 
mystr   STR     NULL    # some comment
END

    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "NULL strs parsed");

    my $tree = [
        {
            name    => 'mystr',
            class   => 'scalar',
            type    => 'STR',
            value   => undef,
        },
        {
            name    => 'mystr',
            class   => 'scalar',
            type    => 'STR',
            value   => undef,
            comment => 'some comment',
        },
    ];

    is_deeply( $config, $tree, "NULL str structure" );
}

{
my $example =<<END;
mystr   STR     # some comment
END

    my $config = $config_parser->parse( $example );
    ok(!defined( $config ), "STR without value");

    is_deeply( $config, undef, "NULL str structure" );
}
