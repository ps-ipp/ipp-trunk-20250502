# Copyright (c) 2004  Joshua Hoblitt
#
# $Id: Log.pm,v 1.9 2008-12-14 22:54:25 eugene Exp $

package Nebulous::Server::Log;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '0.02';

use Log::Log4perl;
use Nebulous::Server::Config;

sub init {
    my ($self, $config) = @_;

#    my $dsn         = $config->db->dsn;
#    my $dbuser      = $config->db->dbuser;
#    my $dbpasswd    = $config->db->dbpasswd;

    my $conf = <<END;
    log4perl.category.Nebulous.Server = WARN, SERVERLOGFILE

    log4perl.appender.SERVERLOGFILE           = Log::Log4perl::Appender::File
    log4perl.appender.SERVERLOGFILE.filename  = /tmp/nebulous_server.log
    log4perl.appender.SERVERLOGFILE.mode      = append
    log4perl.appender.SERVERLOGFILE.layout    = Log::Log4perl::Layout::PatternLayout
    log4perl.appender.SERVERLOGFILE.layout.ConversionPattern = %d{yyyy-MM-dd HH:mm:ss} | %H | %p | %M - %m%n
#   date | hostname | priority | method/sub - message\n

#    log4perl.appender.SQLLOG            = Log::Log4perl::Appender::DBI
#    log4perl.appender.SQLLOG.sql        = \
#    log4perl.appender.SQLLOG.datasource = 
#    log4perl.appender.SQLLOG.username   = 
#    log4perl.appender.SQLLOG.password   = 
    INSERT INTO log (timestamp, hostname, level, sub, message) \
    VALUES          (%d,        %H,       %p,    %M,  %m)
#        VALUES          (?,         ?,        ?,     ?,   ?)
#        log4perl.appender.SQLLOG.params.1   = %d
#        log4perl.appender.SQLLOG.params.2   = %H
#        log4perl.appender.SQLLOG.params.3   = %p
#        log4perl.appender.SQLLOG.params.4   = %M
#        log4perl.appender.SQLLOG.params.5   = %m
#        log4perl.appender.SQLLOG.usePreparedStmt = 1
    log4perl.appender.SQLLOG.bufferSize = 2
    log4perl.appender.SQLLOG.layout     = Log::Log4perl::Layout::NoopLayout
    log4perl.appender.SQLLOG.warp_message = 0
END

    if ($INC{'Apache/DBI.pm'} && $ENV{MOD_PERL}) {
        Log::Log4perl::init_once( \$conf );
    } else {
        Log::Log4perl::init( \$conf );
    }

    return 1;
}

1;

__END__
