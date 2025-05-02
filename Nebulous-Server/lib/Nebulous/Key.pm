# Copyright (c) 2004  Joshua Hoblitt
#
# $Id: Keys.pm,v 1.3 2008-09-09 02:18:47 jhoblitt Exp $

package Nebulous::Key;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '0.03';

use base qw( Exporter Class::Accessor::Fast );

use File::Spec;
use URI::file;
use Carp qw( croak );
use URI;
use overload '""' => \&_stringify_key;

our @EXPORT_OK = qw(
    parse_neb_key
    parse_neb_volume
);

__PACKAGE__->mk_ro_accessors(qw( path volume hard_volume ));

sub parse_neb_key
{
    my ($key, $volume) = @_;

    croak "key param is not optional" unless defined $key;
    croak "too many params" if scalar @_ > 2;

    # white space is not allowed
    if ($key =~ qr/\s+/) {
        croak "keys and URIs may not contain whitespace";
    }

    my $uri = URI->new($key);
    my $scheme = $uri->scheme;
    my $path = $uri->opaque;
    
    my $volume_name;
    my $hard_volume;
    # if this is a valid uri 
    if (defined $scheme) {
        # if so, does it use the neb scheme?
        croak "URI does not use the 'neb' scheme"
            unless $scheme eq 'neb'; 

        # neb:path (not allowed)
        # neb://<volume name>/...
        # neb:/path... (leading '/' is stripped)
        # neb:///path... (same as neb:/path)

        # volume specifier must be at least one character
        # strip off volume component if it exists
        $path =~ s|^//([^/]+)||;

        # a new is not allowed to be just a volume specifier, it must have a
        # path component to it
        unless (length $path) {
            croak "neb URI scheme requires a path component"; 
        }
        
        # ignore key supplied volume name if one is passed as a parameter
        $volume ||= $1;
        my $volume_info = parse_neb_volume($volume);

        # magically eat the "any" volume
        if (defined $volume_info->{volume} and $volume_info->{volume} ne 'any') {
            $volume_name = $volume_info->{volume};
            $hard_volume = $volume_info->{hard_volume};
        }

        # require a leading slash if there is no volume name
        if ((not defined $volume_name) and (not $path =~ m|^/|)) {
            croak "neb URI scheme requires a leading slash, eg. neb:/";
        }
    } else {
        my $volume_info = parse_neb_volume($volume);

        # magically eat the "any" volume
        if (defined $volume_info->{volume} and $volume_info->{volume} ne 'any') {
            $volume_name = $volume_info->{volume};
            $hard_volume = $volume_info->{hard_volume};
        }
    }

    # strip leading slashes
    $path =~ s|^/+||;

    # strip leading '.' or '..'
    $path =~ s|^\.{1,2}||;

    # remove multiple /'s and trailing slashes
    $path = File::Spec->canonpath($path);

    return __PACKAGE__->new({
        volume      => $volume_name,
        hard_volume => $hard_volume,
        path        => $path,
    });
}


sub parse_neb_volume
{
    my $volume = shift;
    return unless defined $volume;

    my $hard_volume;
    # check to see if there is a tilde and remove it if found
    if (defined $volume and $volume =~ s/^~//) {
        $hard_volume = 1;
    }

    return({ volume => $volume, hard_volume => $hard_volume });
}


sub _stringify_key
{
    my $self = shift;

    my $path        = $self->path;
    my $volume      = $self->volume || '';
    my $hard_volume = $self->hard_volume ? '~' : '';

    return "neb://${hard_volume}${volume}/$path";
}


1;

__END__

=pod

=head1 NAME

Nebulous::Key - Nebulous Keys Explained

=head1 DESCRIPTION

The Nebulous system interprets it's storage object keys in a I<slightly>
magical manner.  It supports both plain vanilla C<key strings> and keys in the
form of an L<URI>.  In the L<URI> form a specific storage volume can be implied
(as a request, not a requirement).  Both key forms are subject to mangling such
that I<IF> the key I<looks> like a C<POSIX> semantics file path, it is
canocalized.

=head1 RESERVED SYNTAX

This particular syntax is disallowed and is reserved for future use.

    <neb:<phrase>/<path>

=head1 EXAMPLES

=head2 Key Strings

    key:        foo/bar/baz/quix
    mangled to: foo/bar/baz/quix
    volume:     any

    key:        /foo/bar/baz/quix
    mangled to: foo/bar/baz/quix
    volume:     any

    key:        //foo/bar/baz/quix
    mangled to: foo/bar/baz/quix
    volume:     any

    key:        ///foo/bar/baz/quix
    mangled to: foo/bar/baz/quix
    volume:     any

    key:        foo////bar/baz/quix
    mangled to: foo/bar/baz/quix
    volume:     any

    key:        foo/bar/baz/quix/
    mangled to: foo/bar/baz/quix
    volume:     any

=head2 URI w/ volume name

neb://<volume>/<path>

    key:        neb://foo/bar/baz/quix
    mangled to: bar/baz/quix
    volume:     foo

    key:        neb://foo//bar/baz/quix
    mangled to: bar/baz/quix
    volume:     foo

    key:        neb://foo///bar/baz/quix
    mangled to: bar/baz/quix
    volume:     foo

    key:        neb://foo/bar///baz/quix
    mangled to: bar/baz/quix
    volume:     foo

    key:        neb://foo/bar/baz/quix/
    mangled to: bar/baz/quix
    volume:     foo

=head2 URI w/o volume name

neb:/<path>

    key:        neb:/foo/bar/baz/quix
    mangled to: foo/bar/baz/quix
    volume:     any

    key:        neb:///foo/bar/baz/quix
    mangled to: foo/bar/baz/quix
    volume:     any

    key:        neb://///foo/bar/baz/quix
    mangled to: foo/bar/baz/quix
    volume:     any

=head2 Forbidden Keys

=head3 Key with whitespace

    "/ foo/bar/baz/quix"
    " /foo/bar/baz/quix"
    "/foo/bar/baz/quix "

=head3 URI with whitespace

    "neb ://foo/bar/baz/quix"
    "neb:// foo/bar/baz/quix"
    " neb://foo/bar/baz/quix"
    "neb://foo/bar/baz/quix "

=head3 URI with out a volume requires a leading slash

    neb:foo/bar/baz/quix

=head1 CREDITS

Just me, myself, and I.

=head1 SUPPORT

Please contact the author directly via e-mail.

=head1 AUTHOR

Joshua Hoblitt <jhoblitt@cpan.org>

=head1 COPYRIGHT

Copyright (C) 2008  Joshua Hoblitt.  All rights reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA  02111-1307, USA.

The full text of the license can be found in the LICENSE file included with
this module, or in the L<perlgpl> Pod as supplied with Perl 5.8.1 and later.

=head1 SEE ALSO

L<Nebulous::Server>, L<Nebulous::Client>, L<nebclient(3)>

=cut
