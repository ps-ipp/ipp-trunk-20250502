# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: DataStore.pm,v 1.11 2008-05-13 03:12:52 jhoblitt Exp $

package DataStore;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.08';

=pod

=head1 NAME

DataStore - client interface to a DataStore server

=head1 SYNOPSIS

    use DataStore;

    # equivalent to:

    use DataStore::File::Parser;
    use DataStore::File;
    use DataStore::FileSet::Parser;
    use DataStore::FileSet;
    use DataStore::Product;
    use DataStore::Response;
    use DataStore::Utils;

=head1 DESCRIPTION

This is a convenience module so that that don't have to individualy load (C<use
...;>) all of the common DataStore inteface modules.  Please see the POD of the
individual modules for usage information.  If this is your browsing of the
documentation you may want to start with L<DataStore::Product>.

=head1 USAGE

=head2 Import Parameters

This module accepts no arguments to it's C<import> method and exports no
I<symbols>.

=head2 Methods

None.

=head1 EXAMPLE PROGRAM

    #!/usr/bin/perl

    use strict;
    use warnings;

    # loads DataStore::*
    use DataStore;

    my $dsp = DataStore::Product->new(
        uri => 'http://example.org/productid/',
        last_fileset => 'foobarbaz',
    );

    # returns a DataStore::Response object
    my $response = $dsp->request;

    unless ($response->is_success) {
        die $response->status;
    }

    # arrayref of DataStore::FileSet objects
    my $filesets = $response->data;

    # the query could be a success but still return no filesets
    unless ($filesets) {
        warn "no filesets returned";
        exit(0);
    }

    # returns a DataStore::Response object
    my $response2 = @$filesets[0]->request;

    unless ($response2->is_success) {
        die $response2->status;
    }

    # arrayref of DataStore::File objects
    my $files = $response->data;

    # the query could be a success but still return no files (is that legal?)
    unless (@$files) {
        warn "no files returned";
        exit(0);
    }

    # requires a filename
    my $response3 = @$files[0]->request( filename => '/dev/null' );
    unless ($response3->is_success) {
        warn $response3->status;
        die;
    }

    # $response3->data is '/dev/null'

=cut

use DataStore::File::Parser;
use DataStore::File;
use DataStore::FileSet::Parser;
use DataStore::FileSet;
use DataStore::Product::Parser;
use DataStore::Product;
use DataStore::Root;
use DataStore::Response;
use DataStore::Utils;

=head1 CREDITS

Just me, myself, and I.

=head1 SUPPORT

Please contact the author directly via e-mail.

=head1 AUTHOR

Joshua Hoblitt <jhoblitt@cpan.org>

=head1 COPYRIGHT

Copyright (C) 2006  Joshua Hoblitt.  All rights reserved.

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

L<DataStore::File::Parser>, L<DataStore::File>, L<DataStore::FileSet::Parser>, L<DataStore::FileSet>, L<DataStore::Product>, L<DataStore::Response>

=cut

1;

__END__
