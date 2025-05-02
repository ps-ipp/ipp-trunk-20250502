# Copyright (c) 2004  Joshua Hoblitt
#
# $Id: Util.pm,v 1.13 2008-07-10 23:21:27 jhoblitt Exp $

package Nebulous::Util;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '0.02';

use base qw( Exporter );

use File::Spec::Functions;
use Log::Log4perl qw( :levels );
use URI::file;
use URI;

my @symbols = qw(
    %LEVELS
    _nuke_file
    _get_file_path
    _get_filehandle
    _open_uri
    parse_neb_key
    print_xattrs
    print_all_xattrs
    write_xattrs
    delete_xattrs
    parse_xattr_pair
);

our @EXPORT_OK      = @symbols;
our %EXPORT_TAGS    = ( standard => [ @symbols ] );

our %LEVELS = (
    off     => $OFF,
    fatal   => $FATAL,
    error   => $ERROR,
    warn    => $WARN,
    info    => $INFO,
    debug   => $DEBUG,
    all     => $ALL,
);


# XXX replace the unlink with a 'move to trash' operation
# empty the trash with a daemon on the NSF server
sub _nuke_file {
    my $path = shift;

    die "can't unlink file $path: it's a directory" 
        if -d $path;
    die "can't unlink file $path: it doesn't exist"
        unless -e $path;
    unlink $path or die "can't unlink file $path: $!";

    return 1;
}


sub _get_file_path {
    my $uri = shift;

    my $path = URI->new( $uri )->path;

    return $path;
}


sub _get_filehandle {
    my ( $path, $flags ) = @_;

    # XXX this is a attempt to work around some sort of nasty NFS bug where
    # occasionally stat()/open() on a file on an NFS mounted filesystem will
    # fail EVEN THOU THE FILE ACTUALLY EXISTS.
    #
    # The instance file attempting to be opened should always exist as it was
    # created by the Nebulous server.

    my $fh;
    for (my $i = 0; $i < 60; $i++) {
        eval {
            die "can't open file $path: file doesn't exist" 
                unless -e $path;
            CORE::open($fh, $flags, $path)
                or die "can't open file $path: $!";
        };
        if ($@ =~ qr/file doesn't exist/) {
            sleep 1;
            next;
        } 
        if ($@) {
            die $@;
        }

        last;
    }
    
    return $fh;
}


sub _open_uri {
    my ( $uri , $flags ) = @_;

    $uri = URI->new("$uri");

    my $fh = _get_filehandle($uri->file, $flags);

    return $fh;
}


sub print_all_xattrs
{
    my ($neb, $key) = @_;

    return unless defined $neb;
    return unless defined $key;

    my $xattr_names = $neb->listxattr($key) or die $neb->err;
    foreach my $name (@$xattr_names) {
        print_xattrs($neb, $key, $name) or return;
    }

    return 1;
}


sub print_xattrs
{
    my ($neb, $key, @xattr_names) = @_;

    return unless defined $neb;
    return unless defined $key;
    return unless scalar @xattr_names;

    foreach my $arg (@xattr_names) {
        my ($name, $value) = parse_xattr_pair($arg);
        die "can not process $arg because it is in name:value form"
            if defined $value;
        $value = $neb->getxattr($key, $name) or die $neb->err;
        print "$name:$value\n";
    }

    return 1;
}


sub write_xattrs
{
    my ($neb, $key, @xattr_pairs) = @_;

    return unless defined $neb;
    return unless defined $key;
    return unless scalar @xattr_pairs;

    foreach my $arg (@xattr_pairs) {
        my ($name, $value) = parse_xattr_pair($arg);
        die "can not process $arg because it is not in name:value form"
            unless defined $name and defined $value;
        die "xattr name: $name is not in the form user.name"
            unless $name =~ /^user\./;
        $neb->setxattr($key, $name, $value, "replace")
            or die $neb->err;
    }

    return 1;
}


sub delete_xattrs
{
    my ($neb, $key, @xattr_names) = @_;

    return unless defined $neb;
    return unless defined $key;
    return unless scalar @xattr_names;

    foreach my $arg (@xattr_names) {
        my ($name, $value) = parse_xattr_pair($arg);
        die "can not process $arg because it is in name:value form"
            if defined $value;
        $neb->removexattr($key, $name)
            or die $neb->err;
    }

    return 1;
}


sub parse_xattr_pair
{
    my $pair = shift;
    
    return unless defined $pair;

    no warnings qw( uninitialized );
    my ($name, $value) = split(/:/, $pair);
    use warnings;

    return ($name, $value);
}


1;

__END__
