# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: Utils.pm,v 1.13 2008-08-29 01:39:48 bills Exp $

package DataStore::Utils;

use strict;
use warnings;

use vars qw( $VERSION );
$VERSION = '0.02';

use base qw( Exporter );

use Carp qw( carp );
use Data::Validate::URI qw( is_uri );

use vars qw(
    @EXPORT_OK
    $STD_FIELD
    $TIME_FIELD
    $BYTE_FIELD
    $MD5_FIELD
    %KNOWN_FILE_TYPES
    %KNOWN_FILESET_TYPES
    %KNOWN_PRODUCT_TYPES
);

@EXPORT_OK = qw(
    @EXPORT_OK
    $STD_FIELD
    $TIME_FIELD
    $BYTE_FIELD
    $MD5_FIELD
    %KNOWN_FILE_TYPES
    %KNOWN_FILESET_TYPES
    %KNOWN_PRODUCT_TYPES
);

$STD_FIELD = qr/^[A-z0-9-+_.: ]+$/;
$TIME_FIELD = qr/^(\d{4})-(\d\d)-(\d\d) T (\d\d):(\d\d):(\d\d) Z$/x;
$BYTE_FIELD = qr/^\d+$/;
$MD5_FIELD = qr/^[0-9a-f]{32}$/;
%KNOWN_FILE_TYPES = map { $_ => 1 } qw( chip psrequest psresults pstamp chipproc warp stack diff ipp-mops table text xml tgz fits IPP-MOPS IPP-PSPS ipp-psps notset );
%KNOWN_FILESET_TYPES = map { $_ => 1 } qw( OBJECT BIAS DARK SKYFLAT DOMEFLAT OOF SHACKHARTMANN PSREQUEST PSRESULTS IPP-MOPS XRAY FOCUS MOPS_DETECTABILITY_QUERY MOPS_DETECTABILITY_RESPONSE MOPS_TRANSIENT_DETECTIONS LED notset IPP_PSPS IPP-DIST table);
%KNOWN_PRODUCT_TYPES = map { $_ => 1 } qw( image dump psrequest psresults table ipp-dist ipp-misc dqresults IPP-MOPS IPP-PSPS PSRESULTS);

=pod

=head1 NAME

DataStore::Utils - functions and/or regexs common to DataStore::* modules

=head1 SYNOPSIS

    use DataStore::Utils qw( ... );

=head1 DESCRIPTION

This class parses a DataStore listing of I<FileSet>s into an array of L<DataStore::FileSet> objects.

=head1 USAGE

=head2 Import Parameters

This module exports no I<symbols> by default. It will export these symbols upon request:

=over 4

=item * C<$STD_FIELD>

A C<qr> for the standard DataStore list field type.

=item * C<$TIME_FIELD>

A C<qr> for the standard DataStore time field type.

=item * C<$BYTE_FIELD>

A C<qr> for the standard DataStore byte field type.

=item * C<$MD5_FIELD>

A C<qr> for the standard DataStore hex encoded md5 field type.

=item * C%<KNOWN_FILE_TYPES>

A a hash of the known I<File> types.

=item * C%<KNOWN_FILESET_TYPES>

A a hash of the known I<FileSet> types.

=back

=head2 Methods

None.

=cut

1;

__END__
