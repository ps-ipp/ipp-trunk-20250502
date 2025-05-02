#!/bin/env perl

# program to create a fits binary table with EXTNAME PS1_PS_TARGETLIST

use warnings;
use strict;

use Astro::FITS::CFITSIO qw( :constants );
Astro::FITS::CFITSIO::PerlyUnpacking(1);
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use Math::Trig;
use Data::Dumper;

use constant EXTNAME => 'PS1_PS_TARGETLIST'; # Extension name for output table
use constant EXTVER =>  2;

my ( $input,			# Name of input Detectabilty Query table
     $output,			# Name of output table
     $save_temps,		# Save temporary files?
     );

GetOptions(
	   'input|i=s'    => \$input,
	   'output|o=s'   => \$output,
	   'save-temps'   => \$save_temps,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --input --output",
           -exitval => 3)
    unless defined $input 
    and defined $output;


# The header kewords
my $header = [
        { name =>  'REQ_NAME', 
                    writetype => TSTRING, 
                    comment => 'Postage Stamp Request Name',
                    value => undef
        },
        { name =>  'REQ_ID', 
                    writetype => TLONGLONG, 
                    comment => 'Postage Stamp Server Request id',
                    value => undef
        },
];

# Specification of columns to write
my $columns = [ 
        # rownum from original request
        { name => 'ROWNUM',   type => 'V', writetype => TULONG }, 

        # single exposure results
        { name => 'FPA_ID',  type => '20A', writetype => TSTRING }, 
        { name => 'EXP_ID',  type => 'V',   writetype => TULONG },
        { name => 'CHIP_ID', type => 'V',   writetype => TULONG },
        { name => 'CHIP_READY', type => '2A',  writetype => TSTRING},
#        { name => 'CAM_ID',  type => 'V',   writetype => TULONG },
        { name => 'WARP_ID', type => 'V',   writetype => TULONG },
        { name => 'WARP_READY', type => '2A',  writetype => TSTRING},

        # stack columns
        { name => 'STACK_ID', type => 'V',  writetype => TULONG },
        { name => 'STACK_FAILED', type => '2A',  writetype => TSTRING},
        { name => 'SEEING',   type => 'D',   writetype => TDOUBLE },

        # stack and diff specific columns
        { name => 'TESS_ID',  type => '64A', writetype => TSTRING },    
        { name => 'SKYCELL_ID', type => '64A', writetype => TSTRING },    
        { name => 'RUN_TYPE', type => '64A', writetype => TSTRING },    

        # actual start time of exposure
        { name => 'MJD_OBS',  type => 'D',   writetype => TDOUBLE },
        # field center at midpoint of expsure, in degrees
        { name => 'RA_OBS',  type => 'D',   writetype => TDOUBLE },
        # field center at midpoint of expsure, in degrees
        { name => 'DEC_OBS', type => 'D',   writetype => TDOUBLE },
        # actual filter
        { name => 'FILTER',  type => '16A', writetype => TSTRING }, 
        # exposure time of parent image
        { name => 'EXPTIME', type => 'D',   writetype => TDOUBLE },

        # the following are copied from the original pstamp request

        # image selection parameters
        { name => 'PROJECT',    type => '16A', writetype => TSTRING }, 
        { name => 'SURVEY_NAME',type => '64A', writetype => TSTRING },    
        { name => 'IPP_RELEASE',type => '64A', writetype => TSTRING },    
        { name => 'REQ_TYPE',   type => '16A', writetype => TSTRING },        
        { name => 'IMG_TYPE',   type => '16A', writetype => TSTRING },       
        { name => 'ID',         type => '16A', writetype => TSTRING },            
        { name => 'DATA_GROUP', type => '64A', writetype => TSTRING },    

        # error code from processing this row
        { name => 'ERROR_CODE',type => 'V',  writetype => TULONG }, 
        # error string correspoding to ERROR_CODE
        { name => 'ERROR_STR',type => '24A', writetype => TSTRING }, 

        # from original request
        { name => 'COMMENT',    type => '64A', writetype => TSTRING },    

];

my $in;
if ($input eq '-') {
    $in = \*STDIN;
} else {
    open $in, "<$input" or die "cannot open $input for reading";
}

my @colData;
my $i = 0;
foreach my $col (@$columns) {
    print "$i $col->{name}\n";
    push @colData, [];
    $i++;
}


my $numRows = read_data_for_table($in,'\|', \@colData, $header); 
if (!$numRows) {
    print STDERR "no data in $input\n";
    exit 1;
}

my $status = make_fits_table($output, EXTNAME, $numRows, \@colData, $columns, $header);

exit $status;

# TODO: put this in a module

# two utility functions that may be used to create a FITS binary
# table from hashes describing the header keywords and columns

# read_table_description reads the data for a table from a simple text file
# make_fits_table writes out the table to a named file


# A function to build a fits binary table from supplied data 
# 
sub make_fits_table {
        my $output = shift;     # name of output file
        my $extname = shift;    # extension name
        my $numRows = shift;    # number of rows in the table
        my $colData = shift;    # ref to array of arrays containing the data for each column
        my $columns = shift;    # ref to array of column descriptions (each a hash)
                                # with keys: name, type, and writetype
        my $header = shift;     # ref to array of header keyword descriptions - each a hash
                                # with keys: name, name, writetype, comment, and value
        my $status = 0;

        die "incorrect arguments" if !defined($columns);
        # note $header can be nil

        # build arrays for cfitsio
        my @colNames;			# Names of columns
        my @colTypes;			# Types of columns
        my @colWriteType;               # type to use to write

        foreach my $colSpec ( @$columns) {
            push @colNames, $colSpec->{name};
            push @colTypes, $colSpec->{type};
            push @colWriteType, $colSpec->{writetype};
        }

        if (-e $output) {
            unlink "$output" or die "failed to remove existing $output";
        }

        my $outFits = Astro::FITS::CFITSIO::create_file( $output, $status ); # Output file handle
        check_fitsio( $status );

        $outFits->create_img( 16, 0, undef, $status );
        check_fitsio( $status );

        # Create the table

        $outFits->create_tbl( BINARY_TBL(), $numRows, scalar @colNames,
                                \@colNames, \@colTypes, undef, $extname, $status );
        check_fitsio( $status );

        # if header keyword descriptions were provided add them
        if ($header) {
            foreach my $headerword ( @$header ) {
                my $value = $headerword->{value};
                unless (defined $value) {
                    print "Can't find header keyword $headerword\n";
                    next;
                }
                # zap quotation marks
                $value =~ s/\'//g;
                my $name    = $headerword->{name};
                my $type    = $headerword->{writetype};
                my $comment = $headerword->{comment};
                $outFits->write_key( $type, $name, $value, $comment, $status );
                check_fitsio( $status );
            }
        }


        for (my $i = 0; $i < scalar @colNames; $i++) {
            my $writeType = $colWriteType[$i];
            $outFits->write_col( $writeType, $i + 1, 1, 1, $numRows, $colData->[$i], $status );
            check_fitsio( $status );
        }

        $outFits->close_file( $status );

        return 0;

} # end of sub make_fits_table



# read the table contents from a file
#
# input text file format:
#   lines that begin with '#' are comment lines and are skipped.
#   other lines are data. Each data line is split into fields with the
#   provided separator
#
# if $header is not null header the first non-commented line is read to
# fill the value for each header keyword. The number of fields must match
# the number of keywords.
#
# Following the optional header data, each data line contains data for each
# row in the table. The number of fields must match the number of column
# arrays provided.

sub read_data_for_table {
    my $in      = shift;    # input file handle
    my $sep     = shift;    # string containing field separator
    my $colData = shift;    # reference to an array of arrays for the data
    my $header  = shift;    # rerence to array of header keyword descriptions

    my $line_num = 0;

    # read data for header if any data is expected
    if ($header) {
        my $nhead = @$header;
        while (my $line = <$in>) {
            $line_num++;
            next if ($line =~ /^#/);    # skip comment lines
            chomp $line;
            my @vals = split /$sep/, $line;
            my $nvals = @vals;
            die "number of header columns in input $nvals does not equal expected number of header words $nhead"
                    if (@vals != @$header);

            for (my $i=0; $i < @$header; $i++) {
                $header->[$i]->{value} = $vals[$i];
            }

            last; # only one header line
        }
    }

    my $row_num = 0;
    my $ncols = @$colData;
    while (my $line = <$in>) {
        $line_num++;
        next if ($line =~ /^#/);    # skip comment lines
        chomp $line;

        my @vals = split /$sep/, $line;
        my $nvals = @vals;
        die "number of columns $nvals in input does not equal expected number of header "
                . " words $ncols on line $line_num" if ($nvals != $ncols);

        for (my $col = 0; $col < @$colData; $col++) {
            $colData->[$col]->[$row_num] = $vals[$col];
        }
        $row_num++;
    }

    # we return the number of rows read
    return $row_num;
}

# From Astro::FITS::CFITSIO demo
sub check_fitsio
{
    my $status = shift;		# Status of FITSIO calls

    if ($status != 0) {
	my $msg;		# Message to output
	Astro::FITS::CFITSIO::fits_get_errstatus( $status , $msg );
	die "CFITSIO error: $msg\n";
    }
}
