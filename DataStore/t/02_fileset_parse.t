#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 02_fileset_parse.t,v 1.10 2007-09-25 23:50:34 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::More tests => 43;
use Test::Warn;

=head1 NAME

t/02_fileset_parse.t - tests DataStore::FileSet::Parser

=head1 SYNOPSIS
    
    prove t/02_fileset_parse.t

=cut

use DataStore::FileSet::Parser;
#use Test::Warn;

can_ok('DataStore::FileSet::Parser', qw(
    new
    parse
));

# ->new()

{
    my $parser = DataStore::FileSet::Parser->new;

    isa_ok($parser, 'DataStore::FileSet::Parser');
}

# ->parse()

eval {
    my $parser = DataStore::FileSet::Parser->new;

    $parser->parse(undef);
};
like($@, qr/is not one of the allowed types/,
    '->parse() fails when passed undef');

eval {
    my $parser = DataStore::FileSet::Parser->new;

    $parser->parse('');
};
like($@, qr/ did not pass regex check/,
    '->parse() fails when passed a zero length string');

warning_like {
    my $parser = DataStore::FileSet::Parser->new;

    is($parser->parse('asdf|asdf'), undef,
        '->parse() returns undef on failure');
} qr/not enough fields/,
    '->parse() fails when there are too few fields';

warning_like {
    my $parser = DataStore::FileSet::Parser->new;

    is($parser->parse('|2006-01-01T00:03:04Z|OBJECT'), undef,
        '->parse() returns undef on failure');
} qr/does not conform /,
    '->parse() fails when the fileset field is not in the proper format';

warning_like {
    my $parser = DataStore::FileSet::Parser->new;

    is($parser->parse('foobar|2006-01-0100:03:04Z|OBJECT'), undef,
        '->parse() returns undef on failure');
} qr/does not conform /,
    '->parse() fails when the datetime field is not in the proper format';

warning_like {
    my $parser = DataStore::FileSet::Parser->new;

    is($parser->parse('foobar|2006-01-01T00:03:04Z| '), undef,
        '->parse() returns undef on failure');
} qr/does not conform /,
    '->parse() fails when the type field is not in the proper format';

warning_like {
    my $parser = DataStore::FileSet::Parser->new;

    is($parser->parse('foo#bonkbar|2006-01-01T00:03:04Z|baz'), undef,
        '->parse() returns undef on failure');
} qr/contains #/,
    '->parse() fails when a field contains #';

warning_like {
    my $parser = DataStore::FileSet::Parser->new;

    is($parser->parse('foobar|2006-01-01T00:03:04Z|baz # foo'), undef,
        '->parse() returns undef on failure');
} qr/contains #/,
    '->parse() comments are not allowed after the last field';

{
    # multi-row example
my $example1 =<<END;
# filesetID|time registered    |type   |telescope pointing         |etime|f|airmass|
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
END
    my $parser  = DataStore::FileSet::Parser->new(
        base_uri => 'http://foo.com/index.txt'
    );
    my @results = $parser->parse($example1);

    is(scalar @results, 4, "correct number of item returned in list context");
}

{
    # multi-row example
my $example1 =<<END;
# filesetID|time registered    |type   |telescope pointing         |etime|f|airmass|
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |
END
    my $parser  = DataStore::FileSet::Parser->new(
        base_uri => 'http://foo.com/index.txt'
    );
    my $results = $parser->parse($example1);

    is(scalar @$results, 4,
        "correct number of item returned in scalar context");

    isa_ok(@$results[0], 'DataStore::FileSet');
    is(@$results[0]->fileset, 'otis0123456', 'correct fileset');
    is(@$results[0]->datetime, '2006-01-01T00:03:04Z', 'correct datetime');
    is(@$results[0]->type, 'OBJECT', 'correct type');
    is(@$results[0]->uri, 'http://foo.com/otis0123456/index.txt', 'correct uri');

    isa_ok(@$results[1], 'DataStore::FileSet');
    is(@$results[1]->fileset, 'otis0123456', 'correct fileset');
    is(@$results[1]->datetime, '2006-01-01T00:03:04Z', 'correct datetime');
    is(@$results[1]->type, 'OBJECT', 'correct type');
    is(@$results[1]->uri, 'http://foo.com/otis0123456/index.txt', 'correct uri');

    isa_ok(@$results[2], 'DataStore::FileSet');
    is(@$results[2]->fileset, 'otis0123456', 'correct fileset');
    is(@$results[2]->datetime, '2006-01-01T00:03:04Z', 'correct datetime');
    is(@$results[2]->type, 'OBJECT', 'correct type');
    is(@$results[2]->uri, 'http://foo.com/otis0123456/index.txt', 'correct uri');

    isa_ok(@$results[3], 'DataStore::FileSet');
    is(@$results[3]->fileset, 'otis0123456', 'correct fileset');
    is(@$results[3]->datetime, '2006-01-01T00:03:04Z', 'correct datetime');
    is(@$results[3]->type, 'OBJECT', 'correct type');
    is(@$results[3]->uri, 'http://foo.com/otis0123456/index.txt', 'correct uri');
}

{
    # example with blank lines & comments
my $example1 =<<END;


# filesetID|time registered    |type   |telescope pointing         |etime|f|airmass|

                            # random comment
otis0123456|2006-01-01T00:03:04Z|OBJECT|11:00:10.33 68:26:59.6 2000|30.0 |r|1.23   |

                            # random comment

# foo
END
    my $results = DataStore::FileSet::Parser->new->parse($example1);

    is(scalar @$results, 1, "correct number of item returned");

    isa_ok(@$results[0], 'DataStore::FileSet');
    is(@$results[0]->fileset, 'otis0123456', 'correct fileset');
    is(@$results[0]->datetime, '2006-01-01T00:03:04Z', 'correct datetime');
    is(@$results[0]->type, 'OBJECT', 'correct type');
}
