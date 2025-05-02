# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: Parser.pm,v 1.15 2007-09-26 03:41:00 jhoblitt Exp $

package DataStore::File::Parser;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.02';

use base qw( Class::Accessor::Fast );

use Carp qw( carp );
use Data::Validate::URI qw( is_uri ); 
use DataStore::Utils qw( $STD_FIELD $BYTE_FIELD $MD5_FIELD %KNOWN_FILE_TYPES );
use DataStore::File;
use Params::Validate qw( validate validate_pos SCALAR );

__PACKAGE__->mk_ro_accessors(qw(base_uri));

=pod

=head1 NAME

DataStore::File::Parser - parses the DataStore 'File' list format

=head1 SYNOPSIS

    use DataStore::File::Parser;

    my $parser = DataStore::File::Parser->new(
        base_uri => 'http://example.org/',
    );

    my @data = $parser->parse($str);
        or
    my $data = $parser->parse($str);


=head1 DESCRIPTION

This class parses a DataStore listing of I<File>s into an array of L<DataStore::File> objects.

=head1 USAGE

=head2 Import Parameters

This module accepts no arguments to it's C<import> method and exports no
I<symbols>.

=head2 Methods

=head3 Constructors

=over 4

=item * C<new()>

Basic constructor.

    my $parser = DataStore::File::Parser->new(
        base_uri => 'http://example.org/',
    );

Accepts an optional hash and returns a L<DataStore::FileSet::Parser> object.

=over 4

=item * base_uri

The base of the URI to set in the created L<DataStore::File> objects.

This key is optional and defaults to C<http://example.org/>.

=back

=cut

sub new
{
    my $class = shift;

    my %p = validate(@_,
        {
            base_uri => {
                type        => SCALAR,
                callbacks   => {
                    'is valid http uri' =>
                        sub { is_uri($_[0]) and $_[0] =~ /^http:/ },
                    'uri ends with /index.txt' =>
                        sub { $_[0] =~ m|/index.txt$| },

                },
                default =>  'http://example.org/index.txt',
            },
        },
    );

    my $self = bless {}, ref $class || $class;

    $p{base_uri} =~ qr|(.*?/)index.txt|;
    $self->{base_uri} = $1;

    return $self;
}


=back

=head3 Object Methods

=over 4

=item * C<base_uri()>

Basic accessor.

=item * C<parse()>

Accepts a string and returns a list in list context or an arrayref is scalar
context.  An empty list or undef is returned if the string contained no rows.

=cut

sub parse
{
    my $self = shift;

    my ($doc) = validate_pos(@_,
        {
            type    => SCALAR,
            regex   => qr/\S+/, # string with at least 1 non WS char
        }
    );

    my @data;
    my $lineno = 1;
LINE: foreach my $line (split /\n/, $doc) {
        # blank lines
        next LINE if $line =~ /^\s*$/;

        # comment lines   
        next LINE if $line =~ /^\s*\#/;

        my @fields = split /\|/, $line;

        # at least fileid, bytes, md5sum, and type fields are required
        if (scalar @fields < 4) {
            carp "line $lineno: not enough fields: $line";
            next LINE;
        }

        foreach my $field (@fields) {
            # fields are not allowed to contain \#
            if ($field =~ /\#/) {
                carp "line $lineno: field $field: contains #: $line";
                next LINE;
            }

            # strip leading and trailing whitespace
            $field =~ s/^\s+//;
            $field =~ s/\s+$//;
        }

        my ($fileid, $bytes, $md5sum, $type) = @fields;

        # validate format of fileid and type
        foreach my $field ($fileid, $type) {
            unless ($field =~ $STD_FIELD) {
                carp "line $lineno: field $field:"
                   . " does not conform to $STD_FIELD: $line";
                next LINE;
            }
        }

        # validate format of bytes
        unless ($bytes =~ $BYTE_FIELD) {
            carp "line $lineno: field $bytes:"
               . " does not conform to $BYTE_FIELD: $line";
            next LINE;
        }

        # validate format of md5sum, hex encoded
        unless ($md5sum =~ $MD5_FIELD) {
            # allow md5sum to be empty
            unless ($md5sum =~ /^\s*$/) {
                carp "line $lineno: field $md5sum:"
                . " does not conform to $MD5_FIELD: $line";
                next LINE;
            }
            $md5sum = undef;
        }

        unless (exists $KNOWN_FILE_TYPES{$type}) {
            carp "line $lineno: type $type unknown: $line";
            next LINE;
        }

	my @extra = @fields[4 .. $#fields] if $#fields >= 4;

        my %p = (
            fileid  => $fileid,
            bytes   => $bytes,
            type    => $type,
            extra   => \@extra, 
            uri     => $self->base_uri . $fileid,
        );
        $p{md5sum} = $md5sum if $md5sum;

        # fifo  
        push @data, DataStore::File->new(%p);
    } continue {
        $lineno++;
    }

    return unless @data;
    return wantarray ? @data : \@data;
}

=back

=head1 SEE ALSO

L<DataStore::File>, L<DataStore::FileSet::Parser>

=cut

1;

__END__
