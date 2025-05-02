# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: Response.pm,v 1.4 2006-07-22 01:17:33 smalle Exp $

package DataStore::Response;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.01';

use base qw( Class::Accessor::Fast );

use Params::Validate qw( validate ARRAYREF BOOLEAN SCALAR UNDEF );

use vars qw( @BASE_FIELDS );

@BASE_FIELDS = qw( is_success code status_line data request );

__PACKAGE__->mk_accessors(@BASE_FIELDS);

=pod

=head1 NAME

DataStore::Response - represents a DataStore response

=head1 SYNOPSIS

    use DataStore::Response;

    my $dsr = DateStore::Response->new(
        is_success  => undef,
        code        => 500,
        status_line => 'foo',
        data        => 'bar',
        request     => DataStore::Product->new( uri => 'http://example.org/' ),
    );

    if ($data) {
        my $success     = $dsr->is_success;
        my $code        = $dsr->code;
        my $status_line = $dsr->status_line;
        my $data        = $dsr->data;
        my DataStore::Response $response = $dsp->request;
    }

=head1 DESCRIPTION

This class represent the return state of the C<request()> method in
L<DataStore::Record> sub-classes.

=head1 USAGE

=head2 Import Parameters

This module accepts no arguments to it's C<import> method and exports no
I<symbols>.

=head2 Methods

=head3 Constructors

=over 4

=item * C<new()>

Basic constructor.

    my $dsr = DateStore::Response->new(
        is_success  => undef,
        code        => 500,
        status_line => 'foo',
        data        => 'bar',
        request     => DataStore::Product->new( uri => 'http://example.org/' ),
    );

Accepts a mandatory hash and returns a L<DataStore::Product> object.

=over 4

=item * uri

A vvalid I<HTTP> URI as a string.

=item * code

An HTTP status code.

=item * status_line

An HTTP status line.

=item * data

A scalar value, an arrayref of scalar data, or undef.

=item * request

An object that I<isa> L<DataStore::Record>.

=back

=cut

sub new
{
    my $class = shift;

    my %p = validate(@_,
        {
            is_success  => {
                type        => BOOLEAN,
                callbacks   => {
                    'is 0, 1, or undef' =>
                        sub { ! defined( $_[0] ) || $_[0] == 0 || $_[0] == 1 },
                },
            },
            code        => {
                type        => SCALAR,
                regex       => qr/^\d{3}$/,
            },
            status_line => {
                type        => SCALAR,
                regex       => qr/\S+/, # string with atleast 1 non WS char

            },
            data        => {
                type        => SCALAR | ARRAYREF | UNDEF,
            },
            request     => {
                isa         => qw( DataStore::Record ),
            },
        }
    );

    my $self = bless \%p, ref $class || $class;

    return $self;
}

=back

=head3 Object Methods

=over 4

=item * C<uri()>

Basic accessor.

=item * C<code()>

Basic accessor.

=item * C<status_line()>

Basic accessor.

=item * C<data()>

Basic accessor.

=item * C<request()>

Basic accessor.

=cut

1;

__END__
