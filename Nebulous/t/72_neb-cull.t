#!/usr/bin/env perl

# Copyright (C) 2009  Joshua Hoblitt

=head1 NAME

t/72_neb-cull.t - tests neb-cull

=head1 SYNOPSIS
    
    prove t/72_neb-cull.t

=cut

use strict;
use warnings;

use Apache::Test qw( -withtestmore );
plan tests => 41;

use lib qw( ./lib ./t );

use Test::Cmd;
use Nebulous::Client;
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };
my $neb_url  = "http://$hostport/nebulous";


my $cmd = 'bin/neb-cull';

# last ditch effort to make sure $cmd executable
chmod 0755, $cmd;

my $test = Test::Cmd->new(prog => $cmd, workdir => '');
isa_ok($test, 'Test::Cmd');

# NEB_SERVER env var not set
undef $ENV{'NEB_SERVER'} if defined $ENV{'NEB_SERVER'};

## problem test
## Test::Nebulous->setup;
## {
##     $ENV{NEB_SERVER} = $neb_url;
## 
##     my $key = 'foo';
## 
##     my $neb = Nebulous::Client->new(
##         proxy => $neb_url,
##     );
##     $neb->create($key);
## 
##     $test->run(args => "--one_only " . $key);
## 
##     ## my $line1 = $test->stdout;
##     ## print "stdout: $line1\n";
##     ## 
##     ## my $line2 = $test->stderr;
##     ## print "stderr: $line2\n";
## 
##     is($neb->stat($key)->[6], 1, "correct # of instances");
##     is($? >> 8, 255, "exit code");
##     like($test->stdout, qr/^$/, "stdout");
##     like($test->stderr, qr/no instances/, "stderr");
## }
## die "stop";

## Test::Nebulous->setup;
## {
##     my $key = 'foo';
## 
##     my $neb = Nebulous::Client->new(
##         proxy => $neb_url,
##     );
##     $neb->create($key);
##     $neb->replicate($key);
## 
##     $test->run(args => "--server $neb_url $key");
## 
##     my $line = $test->stderr;
##     print "stderr: $line\n";
## 
##     is($neb->stat($key)->[6], 1, "correct # of instances");
##     is($? >> 8, 0, "exit code");
##     like($test->stdout, qr/^$/, "stdout");
##     like($test->stderr, qr/^$/, "stderr");
## }
## die "TEST";

Test::Nebulous->setup;
{
    $test->run(args => '');
    missing_args(2, "Required options: --server");
}

# NEB_SERVER set
Test::Nebulous->setup;
{
    $ENV{NEB_SERVER} = $neb_url;

    $test->run(args => '');
    missing_args(2, "missing key operand");
}

Test::Nebulous->setup;
{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);

    $test->run(args => "--server $neb_url $key");
#   $test->run(args => $key);

    is($neb->stat($key)->[6], 1, "correct # of instances");
    is($? >> 8, 255, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/failed to cull Nebulous key/, "stderr");
}

Test::Nebulous->setup;

{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);
    $neb->replicate($key);

    $test->run(args => $key);

    my $line = $test->stderr;

    # the default min_copies is 2:
    is($neb->stat($key)->[6], 2, "correct # of instances");
    is($? >> 8, 255, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/not enough instances/, "stderr");
}

Test::Nebulous->setup;

{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);
    $neb->replicate($key);
    $neb->replicate($key);

    $test->run(args => $key);

    is($neb->stat($key)->[6], 2, "correct # of instances");
    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/^$/, "stderr");
}

## problem test
Test::Nebulous->setup;

{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);

    $test->run(args => "--one_only " . $key);

    is($neb->stat($key)->[6], 1, "correct # of instances");
    is($? >> 8, 255, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/not enough instances/, "stderr");
}

Test::Nebulous->setup;

{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);
    $neb->replicate($key);

    $test->run(args => "--one_only " . $key);

    is($neb->stat($key)->[6], 1, "correct # of instances");
    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/^$/, "stderr");
}

Test::Nebulous->setup;

{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);
    $neb->replicate($key);
    $neb->replicate($key);

    $test->run(args => "--one_only " . $key);

    is($neb->stat($key)->[6], 1, "correct # of instances");
    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/^$/, "stderr");
}

Test::Nebulous->setup;

{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);
    $neb->replicate($key);

    $test->run(args => "--min_copies 1 " . $key);

    is($neb->stat($key)->[6], 1, "correct # of instances");
    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/^$/, "stderr");
}

Test::Nebulous->setup;

{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);

    $test->run(args => "--min_copies 1 " . $key);

    is($neb->stat($key)->[6], 1, "correct # of instances");

    is($? >> 8, 255, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/not enough instances/, "stderr");
}

Test::Nebulous->setup;

{
    my $key = 'foo';

    my $neb = Nebulous::Client->new(
        proxy => $neb_url,
    );
    $neb->create($key);
    $neb->replicate($key);
    $neb->replicate($key);

    $test->run(args => "--min_copies 2 " . $key);

    is($neb->stat($key)->[6], 2, "correct # of instances");
    is($? >> 8, 0, "exit code");
    like($test->stdout, qr/^$/, "stdout");
    like($test->stderr, qr/^$/, "stderr");
}


Test::Nebulous->cleanup;

sub missing_args
{
    my ($exit, $errstr) = @_;

    is($? >> 8, $exit, "error code is: $exit");
    like($test->stderr, qr/$errstr/, "error string is: $errstr");
}
