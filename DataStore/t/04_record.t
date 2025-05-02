#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 04_record.t,v 1.1 2006-02-28 03:56:46 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::More tests => 7;
use DataStore::Record;

=head1 NAME

t/04_record.t - tests DataStore::Record

=head1 SYNOPSIS
    
    prove t/04_record.t

=cut

can_ok('DataStore::Record', qw(
    new
    uri
    request
));

# ->new()

{
    my $rec = DataStore::Record->new;

    isa_ok($rec, 'DataStore::Record');
}

{
    # w/o trailing slash
    my $rec = DataStore::Record->new(
        uri => 'http://example.org/foo'
    );

    isa_ok($rec, 'DataStore::Record');
}

{
    # w/ trailing slash
    my $rec = DataStore::Record->new(
        uri => 'http://example.org/foo/'
    );

    isa_ok($rec, 'DataStore::Record');
}

# ->uri()

{
    # default value
    my $rec = DataStore::Record->new;
    is($rec->uri, 'http://example.org/', '->uri() returned default uri');
}

{
    # default value
    my $rec = DataStore::Record->new(
        uri => 'http://example.org/foo/'
    );
    is($rec->uri, 'http://example.org/foo/', '->uri() returned correct uri');
}

# ->request()

eval {
    my $rec = DataStore::Record->new;

    $rec->request;
};
like($@, qr/must override the ->request\(\) method/,
    '->request() dies when not overridden');
