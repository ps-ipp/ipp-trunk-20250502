# Copyright (c) 2004  Joshua Hoblitt
#
# $Id: Log.pm,v 1.9 2008-05-16 21:09:18 jhoblitt Exp $

package Nebulous::Client::Log;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '0.01';

use Log::Log4perl;
use File::Spec;

sub init {
    my $log_path;
    if (defined $ENV{HOME}) {
    	$log_path = $ENV{HOME};
    } else {
    	$log_path = File::Spec->tmpdir();
    }
    # my $log_path = $ENV{HOME} or File::Spec->tmpdir();

    my $conf = <<END;
    log4perl.category.Nebulous.Client = FATAL, Screen
#log4perl.category.Nebulous.Client = DEBUG, CLIENTLOGFILE, Screen

    log4perl.appender.Screen        = Log::Log4perl::Appender::Screen
    log4perl.appender.Screen.stderr = 1
    log4perl.appender.Screen.layout = Log::Log4perl::Layout::PatternLayout
    log4perl.appender.Screen.layout.ConversionPattern = %d | %H | %p | %M - %m%n

    log4perl.appender.CLIENTLOGFILE           = Log::Log4perl::Appender::File
    log4perl.appender.CLIENTLOGFILE.filename  = $log_path/nebulous_client.log
    log4perl.appender.CLIENTLOGFILE.mode      = append
    log4perl.appender.CLIENTLOGFILE.layout    = Log::Log4perl::Layout::PatternLayout
    log4perl.appender.CLIENTLOGFILE.layout.ConversionPattern = %d{yyyy-MM-dd HH:mm:ss} | %H | %p | %M - %m%n
END

    Log::Log4perl::init( \$conf );
}

1;

__END__
