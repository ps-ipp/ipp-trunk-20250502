#!/usr/bin/env perl

# Copyright (C) 2009  Joshua Hoblitt

=head1 NAME

t/70_neb-ls.t - tests neb-ls

=head1 SYNOPSIS
    
    prove t/70_neb-ls.t

=cut

use strict;
use warnings;

use Apache::Test qw( -withtestmore );
plan tests => 31;

use lib qw( ./lib ./t );

use Test::Cmd;
use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };
my $neb_url  = "http://$hostport/nebulous";

my $cmd = 'bin/neb-ls';

# last ditch effort to make sure neb-ls is executable
chmod 0755, 'bin/neb-ls';

my $test = Test::Cmd->new(prog => $cmd, workdir => '');
isa_ok($test, 'Test::Cmd');

# test if NEB_SERVER env var not set
undef $ENV{'NEB_SERVER'} if defined $ENV{'NEB_SERVER'};
Test::Nebulous->setup;
{
    $test->run(args => '');
    missing_args(2, "Required options: --server");
}

# NEB_SERVER set (and used in the following tests)
Test::Nebulous->setup;
{
    $ENV{NEB_SERVER} = $neb_url;

    $test->run(args => '');
    is($? >> 8, 0, "exit code");
}

Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('foo');

    $test->run(args => '');
    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^foo$/,      "stdout");
    like($test->stderr, qr/^$/,         "stderr");
}

Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('foo');
    $neb->create('bar');

    $test->run(args => '');

    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^bar\s+foo$/,  "stdout");
    like($test->stderr, qr/^$/,         "stderr");
}

Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('foo');

    my $locations = $neb->find_instances( "foo" );
    my $diskfile = _get_file_path( @$locations[0] );
    my $filestats = `ls -l $diskfile`;

    $test->run(args => '-l');

    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^$filestats$/,  "stdout");
    like($test->stderr, qr/^$/,         "stderr");
}

Test::Nebulous->setup;
{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('foo');
    $neb->create('bar');

    $test->run(args => '-c');

    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^bar\nfoo\n$/,   "stdout");
    like($test->stderr, qr/^$/,             "stderr");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('a/foo');

    $test->run(args => '');

    is($? >> 8, 0, "exit code");
    like($test->stdout, qr|^a/$|,           "stdout");
    like($test->stderr, qr/^$/,             "stderr");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('a/foo');

    $test->run(args => 'a');

    is($? >> 8, 0, "exit code");
    like($test->stdout, qr|^a/foo$|,        "stdout");
    like($test->stderr, qr/^$/,             "stderr");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('a/foo');
    $neb->create('a/bar');

    $test->run(args => 'a');

    is($? >> 8, 0, "exit code");
    like($test->stdout, qr|^a/bar\s+a/foo$|,  "stdout");
    like($test->stderr, qr/^$/,             "stderr");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('a/foo');
    $neb->create('foo');

    $test->run(args => 'a');

    is($? >> 8, 0, "exit code");
    like($test->stdout, qr|^a/foo$|,        "stdout");
    like($test->stderr, qr/^$/,             "stderr");
}

Test::Nebulous->setup;

{
    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create('a/foo');
    $neb->create('a/b/foo');

    $test->run(args => 'a');

    is($? >> 8, 0, "exit code");
    like($test->stdout, qr|^a/b/\s+a/foo$|,        "stdout");
    like($test->stderr, qr/^$/,             "stderr");
}

Test::Nebulous->cleanup;

sub missing_args
{
    my ($exit, $errstr) = @_;

    is($? >> 8, $exit, "error code is: $exit");
    like($test->stderr, qr/$errstr/, "error string is: $errstr");
}
