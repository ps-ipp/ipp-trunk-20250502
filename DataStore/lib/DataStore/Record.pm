# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: Record.pm,v 1.7 2006-03-16 22:03:53 jhoblitt Exp $

package DataStore::Record;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.01';

use base qw( Class::Accessor::Fast );

use Carp qw( carp croak );
use Data::Validate::URI qw( is_uri );
use Params::Validate qw( validate_with validate SCALAR );

use vars qw( @BASE_FIELDS );

@BASE_FIELDS = qw( uri );

__PACKAGE__->mk_ro_accessors(@BASE_FIELDS);

=pod

=head1 NAME

DataStore::Record - base class of DataStore list records or rows

=head1 SYNOPSIS

    use base qw( DataStore::Record );

=head1 DESCRIPTION

=head1 USAGE

=head2 Import Parameters

This module accepts no arguments to it's C<import> method and exports no
I<symbols>.

=head2 Methods

=head3 Constructors

=over 4

=item * C<new()>

Basic constructor.

    my $dsp = DataStore::Record->new(
        uri => 'http://example.com/',
    );

Accepts a mandatory hash and returns a L<DataStore::Record> object.

=over 4

=item * uri

A valid I<HTTP> URI as a string. 

=back

=cut

sub new
{
    my $class = shift;

    my %p = validate_with(
        params  => \@_,
        spec    => {
            uri => {
                type        => SCALAR,
                callbacks   => {
                    'is valid http uri' =>
                        sub { is_uri($_[0]) and $_[0] =~ /^http:/ },
                },
                default =>  'http://example.org/',
            },
        },
        allow_extra => 1,
    );

    my $self = bless \%p, ref $class || $class;

    return $self;
}

=back

=head3 Object Methods

=over 4

=item * C<uri()>

=cut

=item * C<request()>

This method must be overloaded in sub-classes.

=cut

sub request
{
    croak "sub classes of " . __PACKAGE__
        . " must override the ->request() method";
}

1;

__END__
