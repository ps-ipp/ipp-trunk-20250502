#!/usr/bin/perl

# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: 04_type.t,v 1.1 2005-03-23 01:53:55 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./lib );

#$::RD_TRACE = 1;

use Test::More tests => 9;
use PS::IPP::Metadata::Config;

my $config_parser = PS::IPP::Metadata::Config->new;

{
my $example = q{
TYPE      CELL   EXTNAME   BIASSEC  CHIP
CELL.00   CELL   CCD00     BSEC-00  CHIP.00
CELL.01   CELL   CCD01     BSEC-01  CHIP.00
};
    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "basic TYPE");
}

{
my $example = q{
TYPE      CELL   EXTNAME   BIASSEC  CHIP    # comment
CELL.00   CELL   CCD00     BSEC-00  CHIP.00 # foo
CELL.01   CELL   CCD01     BSEC-01  CHIP.00 #
};
    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "TYPE with comments");
}

{
my $example = q{
TYPE      CELL   EXTNAME   BIASSEC  CHIP
CELL.00   CELL   CCD00     BSEC-00  CHIP.00
CELL.01   CELL   CCD01     BSEC-01  CHIP.00
FOO METADATA
    TYPE      CELL   EXTNAME   BIASSEC
    CELL.00   CELL   CCD00     BSEC-00
    CELL.01   CELL   CCD01     BSEC-01
    FOO METADATA
        TYPE      CELL   EXTNAME
        CELL.00   CELL   CCD00
        CELL.01   CELL   CCD01
    END
END
};
    my $config = $config_parser->parse( $example );
    ok( defined( $config ), "TYPE not visible in lower scopes");
}

{
my $example = q{
TYPE      CELL   EXTNAME   BIASSEC  CHIP
FOO METADATA
    CELL.00   CELL   CCD00     BSEC-00  CHIP.00
END
};
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "TYPE not in scope");
}

{
my $example = q{
FOO METADATA
    TYPE      CELL   EXTNAME   BIASSEC  CHIP
END
CELL.00   CELL   CCD00     BSEC-00  CHIP.00
};
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "TYPE not in scope");
}

{
my $example = q{
FOO METADATA
    TYPE      CELL   EXTNAME   BIASSEC  CHIP
END
BAR METADATA
    CELL.00   CELL   CCD00     BSEC-00  CHIP.00
END
};
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "TYPE not in scope");
}


{
my $example = q{
TYPE      CELL   EXTNAME   BIASSEC  CHIP
CELL.00   CELL   CCD00     BSEC-00
CELL.01   CELL   CCD01     BSEC-01  CHIP.00
};
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "TYPE with missing parameters");
}

{
my $example = q{
TYPE      CELL   EXTNAME   BIASSEC  CHIP
TYPE      CELL   EXTNAME   BIASSEC  CHIP
CELL.00   CELL   CCD00     BSEC-00  CHIP.00
CELL.01   CELL   CCD01     BSEC-01  CHIP.00
};
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "TYPE redefinition");
}

{
my $example = q{
CELL.00   CELL   CCD00     BSEC-00  CHIP.00
CELL.01   CELL   CCD01     BSEC-01  CHIP.00
};
    my $config = $config_parser->parse( $example );
    ok( !defined( $config ), "missing TYPE declaration");
}
