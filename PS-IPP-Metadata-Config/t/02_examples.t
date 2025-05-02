#!/usr/bin/perl

# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: 02_examples.t,v 1.5 2006-07-12 02:49:53 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use lib qw( ./lib );

#$::RD_TRACE = 1;

use Test::More tests => 9;
use PS::IPP::Metadata::Config;

my $config_parser = PS::IPP::Metadata::Config->new;

{
    my $config = $config_parser->parse(undef);
    is($config, undef, "parsing undef returns undef");
}

{
    my $config = $config_parser->parse('');
    is($config, undef, "parsing an empty string returns undef");
}

{
    my $config = $config_parser->parse("\n\n\n\n\n");
    is($config, undef, "parsing an empty string returns undef");
}

{
my $example = q{
Double     F64     1.23456789      # This is a comment
Float    F32 0.98765#This is a comment too
String  STR This is the string that forms the value #comment

 # This is a comment line and is to be ignored
boolean     BOOL    T # The value of `boolean' is `true'
 
@primes U8  2,3 5 7,11,13 17 #   These are prime numbers

comment MULTI # The rest of this line is ignored, but `comment' is set to be non-unique
comment STR This
comment STR     is
comment STR       a
comment STR        non-unique
comment STR                  key
Float F64 1.23456 # This generates a warning, and, if `overwrite' is `false', is ignored
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "SDRS example parsed");

    my $tree = [
        {
            name    => 'Double',
            class   => 'scalar',
            type    => 'F64',
            value   => '1.23456789',
            comment => 'This is a comment',
        },
        {
            name    => 'Float',
            class   => 'scalar',
            type    => 'F32',
            value   => '0.98765',
            comment => 'This is a comment too',
        },
        {
            name    => 'String',
            class   => 'scalar',
            type    => 'STR',
            value   => 'This is the string that forms the value',
            comment => 'comment',
        },
        {
            name    => 'boolean',
            class   => 'scalar',
            type    => 'BOOL',
            value   => 1,
            comment => q{The value of `boolean' is `true'},
        },
        {
            name    => 'primes',
            class   => 'vector',
            type    => 'U8',
            value   => [qw( 2 3 5 7 11 13 17 )],
            comment => 'These are prime numbers',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'This',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'is',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'a',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'non-unique',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'key',
        },
    ];

    is_deeply( $config, $tree, "SDRS example structure, no overwrite" );
}

{
my $example = q{
Double     F64     1.23456789      # This is a comment
Float    F32 0.98765#This is a comment too
String  STR This is the string that forms the value #comment

 # This is a comment line and is to be ignored
boolean     BOOL    T # The value of `boolean' is `true'
 
@primes U8  2,3 5 7,11,13 17 #   These are prime numbers

comment MULTI # The rest of this line is ignored, but `comment' is set to be non-unique
comment STR This
comment STR     is
comment STR       a
comment STR        non-unique
comment STR                  key
Float F64 1.23456 # This generates a warning, and, if `overwrite' is `false', is ignored
};
    $config_parser->overwrite( 1 );
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "SDRS example parsed");

    my $tree = [
        {
            name    => 'Double',
            class   => 'scalar',
            type    => 'F64',
            value   => '1.23456789',
            comment => 'This is a comment',
        },
        {
            name    => 'String',
            class   => 'scalar',
            type    => 'STR',
            value   => 'This is the string that forms the value',
            comment => 'comment',
        },
        {
            name    => 'boolean',
            class   => 'scalar',
            type    => 'BOOL',
            value   => 1,
            comment => q{The value of `boolean' is `true'},
        },
        {
            name    => 'primes',
            class   => 'vector',
            type    => 'U8',
            value   => [qw( 2 3 5 7 11 13 17 )],
            comment => 'These are prime numbers',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'This',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'is',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'a',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'non-unique',
        },
        {
            name    => 'comment',
            class   => 'scalar',
            type    => 'STR',
            value   => 'key',
        },
        {
            name    => 'Float',
            class   => 'scalar',
            type    => 'F64',
            value   => '1.23456',
            comment => q{This generates a warning, and, if `overwrite' is `false', is ignored},
        },
    ];

    is_deeply( $config, $tree, "SDRS example structure, with overwrite" );
}

{
my $example = q{
# these are examples of camera definition variables:
# skyprobe
NCELL       S32    1
NCHIP       S32    1
#                  FILENAME    EXTNAME  REGION      CHIP      BIASSEC     
CELL.00     STR    %f00.%x     PHU      [0,0:0,0]   CHIP.00   [0,0:0,0] 
                   
# megacam-raw      
NCELL       S32    72
NCHIP       S32    36
#                  FILENAME    EXTNAME  DATASEC     CHIP      BIASSEC     
CELL.00     STR    %f.%x       AMP00    [0,0:0,0]   CHIP.00   BIASSEC
CELL.01     STR    %f.%x       AMP01    [0,0:0,0]   CHIP.00   [2100,2110:0,4096]   
CELL.02     STR    %f.%x       AMP02    [0,0:0,0]   CHIP.01   [0,0:0,0]   
CELL.03     STR    %f.%x       AMP03    [0,0:0,0]   CHIP.01   [0,0:0,0]   

# megacam-splice
NCELL       S32    72
NCHIP       S32    36
#                  FILENAME    EXTNAME  REGION      CHIP      BIASSEC   TRIMSEC
CELL.00     STR    %f.%x       CCD00    ASEC-00     CHIP.00   BSEC-00   DSEC-00
CELL.01     STR    %f.%x       CCD00    ASEC-01     CHIP.00   BSEC-01   DSEC-01
CELL.02     STR    %f.%x       CCD01    ASEC-00     CHIP.01   BSEC-00   DSEC-00
CELL.03     STR    %f.%x       CCD01    ASEC-01     CHIP.01   BSEC-01   DSEC-01

# cfh12k-split
NCELL       S32    12
NCHIP       S32    12
#                  FILENAME    EXTNAME  REGION      CHIP      BIASSEC     
CELL.00     STR    %f/%f00.%x  PHU      [0,0:0,0]   CHIP.00   [0,0:0,0]   
CELL.01     STR    %f/%f01.%x  PHU      [0,0:0,0]   CHIP.01   [0,0:0,0]   
CELL.02     STR    %f/%f02.%x  PHU      [0,0:0,0]   CHIP.02   [0,0:0,0]   

# cfh12k-mef
NCELL       S32    12
NCHIP       S32    12
#                  FILENAME    EXTNAME  REGION      CHIP      BIASSEC     
CELL.00     STR    %f.%x       CHIP00   [0,0:0,0]   CHIP.00   [0,0:0,0]   
CELL.01     STR    %f.%x       CHIP01   [0,0:0,0]   CHIP.01   [0,0:0,0]   
CELL.02     STR    %f.%x       CHIP02   [0,0:0,0]   CHIP.02   [0,0:0,0]   

#- REGION can be defined by a header keyword in IRAF format or by a explicit IRAF format 
#- what is default for NAXIS1,2 for IRAF format?
#- Nreadout is always NAXIS3?

# recipe file:
# this makes the assumption that, for a given camera, all chips &
# cells have the same recipe.  this is probably a good start, but may
# not cut it in general. eg, it is already clear that for 

# recipe file must be a function of time and camera.
# 

# BIAS:
BIAS.IMAGE                 STR    NONE
BIAS.IMAGE  		   STR    FILE:bias.fits
BIAS.IMAGE  		   STR    DB:BEST
BIAS.IMAGE  		   STR    DB:CLOSE

BIAS.OVERSCAN 		   STR    HEADER:BIASSEC
BIAS.OVERSCAN 		   STR    RECIPE:[0,0:0,0]
BIAS.OVERSCAN 		   STR    NONE

BIAS.OVERSCAN.STATS 	   STR    MEDIAN
BIAS.OVERSCAN.STATS 	   STR    MEAN

BIAS.OVERSCAN.FIT          STR    SPLINE
BIAS.OVERSCAN.FIT.NPTS     S32    5

BIAS.OVERSCAN.FIT          STR    POLYNOMIAL
BIAS.OVERSCAN.FIT.ORDER    S32    3
BIAS.OVERSCAN.FIT.NBIN     S32    5
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "GENE example");
}

{
my $example = q{
# The raw MegaCam data comes off the telescope with each of the chips stored in extensions of a MEF file.

# How to identify this type
RULE	METADATA
	TELESCOP	STR	CFHT 3.6m
	DETECTOR	STR	MegaCam
	EXTEND		BOOL	T
	NEXTEND		S32	72
END

# How to read this data
PHU		STR	FPA	# The FITS file represents an entire FPA
EXTENSIONS	STR	CELLS	# The extensions represent cells
EXTENSION_KEY	STR	EXTNAME	# You get the extensions by looking at the EXTNAME header
IDENTIFIER	STR	OBSID	# We identify the observation by the observation Id in the header

# What's in the FITS file?
CONTENTS	METADATA
	TYPE	CELL	CHIP	CELLTYPE
	# Extension name, chip
	amp00	CELL	ccd00	science
	amp01	CELL	ccd00	science
	amp02	CELL	ccd01	science
	amp03	CELL	ccd01	science
	guide	CELL	guide	guide		# A guide CCD thrown in, just for fun
END

# Specify the cell data
CELLS	METADATA
	science	METADATA	# A science CCD
		TYPE	LOCATION	SOURCE	VALUE
		BIASSEC	LOCATION	VALUE	[1:10,1:4096];[1035:1084,1:4096]
		TRIMSEC	LOCATION	VALUE	[11:1034,1:4096]
	#	BIASSEC	LOCATION	HEADER	BIASSEC
	#	TRIMSEC	LOCATION	HEADER	TRIMSEC
	END
	guide	METADATA	# A guide CCD
		TYPE	LOCATION	SOURCE	VALUE
		BIASSEC	LOCATION	VALUE	[1:10,1:1024];[1035:1084,1:1024]
		TRIMSEC	LOCATION	VALUE	[11:1034,1:1024]
	#	BIASSEC	LOCATION	HEADER	BIASSEC
	#	TRIMSEC	LOCATION	HEADER	TRIMSEC
	END
		
END


# How to translate PS concepts into FITS headers
TRANSLATION	METADATA
	AIRMASS         STR             AIRMASS
	EXPTIME         STR             EXPOSURE
	DARKTIME        STR             EXPOSURE # No specific darktime header; use exposure time
	FILTER          STR             FILTER
	DATE-OBS        STR             DATE-OBS
	TIME-OBS        STR             TIME-OBS
	POSANGLE        STR             POSANG
	RA              STR             OBJ-RA
	DEC             STR             OBJ-DEC
END

# Default PS concepts that may be specified by value
DEFAULTS	METADATA
	RADECSYS	STR		ICRS
END

# How to translation PS concepts into database lookups
DATABASE	METADATA
	TYPE		dbEntry		TABLE		COLUMN		GIVENDBCOL	GIVENPS
	GAIN            dbEntry         Camera          gain            chipId,cellId	CHIP,CELL
	READNOISE       dbEntry         Camera          readNoise       chipId,cellId	CHIP,CELL

# A database entry refers to a particular column (COLUMN) in a
# particular table (TABLE), given certain PS concepts (GIVENPS) that
# match certain database columns (GIVENDBCOL).

END
};
    my $config = $config_parser->parse( $example );

    ok( defined( $config ), "Paul/megacam example");
}
