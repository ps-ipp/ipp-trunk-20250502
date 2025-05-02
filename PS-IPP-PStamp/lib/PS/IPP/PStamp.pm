
package PS::IPP::PStamp;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '1.0';

=pod

=head1 NAME

PStamp - Perl Module of Postage Stamp Server functions

=head1 SYNOPSIS

    use PStamp;

    # equivalent to:

    use Pstamp::RequestFile;

=head1 DESCRIPTION

This is a convenience module so that that don't have to individualy load (C<use
...;>) all of the common PStamp inteface modules.  Please see the POD of the
individual modules for usage information. 

=head1 USAGE

=head2 Import Parameters

This module accepts no arguments to it's C<import> method and exports no
I<symbols>.

=head2 Methods

None.

=head1 EXAMPLE PROGRAM

=cut

use PStamp::RequestFile qw( :standard );
use PStamp::Job qw( :standard );

=head1 CREDITS


=head1 SUPPORT

Please contact the author directly via e-mail.

=head1 AUTHOR

Bill Sweeney

=head1 COPYRIGHT

Copyright (C) 2008  Bill Sweeney.  All rights reserved.

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

L<PStamp::RequestFile>

=cut

1;

__END__
