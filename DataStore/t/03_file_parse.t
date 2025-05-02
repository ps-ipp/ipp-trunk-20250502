#!/usr/bin/env perl

# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: 03_file_parse.t,v 1.5 2007-09-26 00:44:43 jhoblitt Exp $

use strict;
use warnings;

use lib qw( ./lib ./t );

use Test::More tests => 50;
use Test::Warn;

=head1 NAME

t/03_file_parse.t - tests DataStore::File::Parser

=head1 SYNOPSIS
    
    prove t/03_file_parse.t

=cut

use DataStore::File::Parser;
#use Test::Warn;

can_ok('DataStore::File::Parser', qw(
    new
    parse
));

# ->new()

{
    my $parser = DataStore::File::Parser->new;

    isa_ok($parser, 'DataStore::File::Parser');
}

# ->parse()

eval {
    my $parser = DataStore::File::Parser->new;

    $parser->parse(undef);
};
like($@, qr/is not one of the allowed types/,
    '->parse() fails when passed undef');

eval {
    my $parser = DataStore::File::Parser->new;

    $parser->parse('');
};
like($@, qr/ did not pass regex check/,
    '->parse() fails when passed a zero length string');

warning_like {
    my $parser = DataStore::File::Parser->new;

    is($parser->parse('asdf|asdf'), undef,
        '->parse() returns undef on failure');
} qr/not enough fields/,
    '->parse() fails when there are too few fields';

warning_like {
    my $parser = DataStore::File::Parser->new;

    is($parser->parse(
    'foo+++bar|83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|chip|'), undef,
        '->parse() returns undef on failure');
} qr/does not conform/,
    '->parse() fails when the fileID field is not in the proper format';

warning_like {
    my $parser = DataStore::File::Parser->new;

    is($parser->parse(
    'otis0123456.01|a83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|chip|'), undef,
        '->parse() returns undef on failure');
} qr/does not conform/,
    '->parse() fails when the bytes field is not in the proper format';

warning_like {
    my $parser = DataStore::File::Parser->new;

    is($parser->parse(
    'otis0123456.01|83002312|zfe6a2b6564c0d4cfb3bbf1db813824ba|chip|'), undef,
        '->parse() returns undef on failure');
} qr/does not conform/,
    '->parse() fails when the md5sum field is not in the proper format';

warning_like {
    my $parser = DataStore::File::Parser->new;

    is($parser->parse(
    'otis0123456.01|83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|c+hip|'), undef,
        '->parse() returns undef on failure');
} qr/does not conform/,
    '->parse() fails when the fileID field is not in the proper format';

warning_like {
    my $parser = DataStore::File::Parser->new;

    is($parser->parse(
    'otis012#3456.01|83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|chip|'), undef,
        '->parse() returns undef on failure');
} qr/contains #/,
    '->parse() fails when a field contains #';

warning_like {
    my $parser = DataStore::File::Parser->new;

    is($parser->parse(
    'otis0123456.01|83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|chip| # foo'), undef,
        '->parse() returns undef on failure');
} qr/contains #/,
    '->parse() comments are not allowed after the last field';

{
    # multi-row example
my $example1 =<<END;
# fileID      |bytes   |md5sum                          |type|chipname|
otis0123456.01|83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|chip|ota01   |
otis0123456.02|83002312|a2b4a7d7b94dc6076c5f3f67239a48c6|chip|ota02   |
otis0123456.03|83002312|a39b1510484c7833e27454b181f13981|chip|ota03   |
otis0123456.04|83002312|7bb35e1e30f3f833c0416aea75f90304|chip|ota04   |
END
    my @results = DataStore::File::Parser->new->parse($example1);
    is(scalar @results, 4, "correct number of item returned in list context");
}

{
    # multi-row example
my $example1 =<<END;
# fileID      |bytes   |md5sum                          |type|chipname|
otis0123456.01|83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|chip|ota01   |
otis0123456.02|83002312|a2b4a7d7b94dc6076c5f3f67239a48c6|chip|ota02   |
otis0123456.03|83002312|a39b1510484c7833e27454b181f13981|chip|ota03   |
otis0123456.04|83002312|7bb35e1e30f3f833c0416aea75f90304|chip|ota04   |
END
    my $results = DataStore::File::Parser->new->parse($example1);

    is(scalar @$results, 4,
        "correct number of item returned in scalar context");

    isa_ok(@$results[0], 'DataStore::File');
    is(@$results[0]->uri, 'http://example.org/otis0123456.01', 'correct uri');
    is(@$results[0]->fileid, 'otis0123456.01', 'correct fileid');
    is(@$results[0]->bytes, '83002312', 'correct datetime');
    is(@$results[0]->md5sum, 'fe6a2b6564c0d4cfb3bbf1db813824ba', 'correct md5');
    is(@$results[0]->type, 'chip', 'correct type');

    isa_ok(@$results[1], 'DataStore::File');
    is(@$results[1]->uri, 'http://example.org/otis0123456.02', 'correct uri');
    is(@$results[1]->fileid, 'otis0123456.02', 'correct fileid');
    is(@$results[1]->bytes, '83002312', 'correct datetime');
    is(@$results[1]->md5sum, 'a2b4a7d7b94dc6076c5f3f67239a48c6', 'correct md5');
    is(@$results[1]->type, 'chip', 'correct type');

    isa_ok(@$results[2], 'DataStore::File');
    is(@$results[2]->uri, 'http://example.org/otis0123456.03', 'correct uri');
    is(@$results[2]->fileid, 'otis0123456.03', 'correct fileid');
    is(@$results[2]->bytes, '83002312', 'correct datetime');
    is(@$results[2]->md5sum, 'a39b1510484c7833e27454b181f13981', 'correct md5');
    is(@$results[2]->type, 'chip', 'correct type');

    isa_ok(@$results[3], 'DataStore::File');
    is(@$results[3]->uri, 'http://example.org/otis0123456.04', 'correct uri');
    is(@$results[3]->fileid, 'otis0123456.04', 'correct fileid');
    is(@$results[3]->bytes, '83002312', 'correct datetime');
    is(@$results[3]->md5sum, '7bb35e1e30f3f833c0416aea75f90304', 'correct md5');
    is(@$results[3]->type, 'chip', 'correct type');
}

{
    # example with blank lines & comments
my $example1 =<<END;


# fileID      |bytes   |md5sum                          |type|chipname|

                            # random comment
otis0123456.01|83002312|fe6a2b6564c0d4cfb3bbf1db813824ba|chip|ota01   |

                            # random comment

# foo
END
    my $results = DataStore::File::Parser->new->parse($example1);

    is(scalar @$results, 1, "correct number of item returned");

    isa_ok(@$results[0], 'DataStore::File');
    is(@$results[0]->fileid, 'otis0123456.01', 'correct fileid');
    is(@$results[0]->bytes, '83002312', 'correct datetime');
    is(@$results[0]->md5sum, 'fe6a2b6564c0d4cfb3bbf1db813824ba', 'correct md5');
    is(@$results[0]->type, 'chip', 'correct type');
}
