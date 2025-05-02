#!/usr/bin/env perl

use strict;
use warnings FATAL => qw( all );

use lib "./lib";
package main;

#use Nebulous::Client;
use IO::Select;
use IO::Socket;
use POSIX qw(:DEFAULT :sys_wait_h);
use Hook::LexWrap;
use Sys::Hostname;

my $print_stdout = undef;

$| = 1;

my $prog_file = shift;
my $key = shift || 'foobar';
my $kids = shift || 1;

require $prog_file;
$key = "/tmp/" . $key;

#my $s = IO::Select->new();

foreach my $id ( 1..$kids ) {
#    my ($sock_parent, $sock_child) = IO::Socket->socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC);
#    $s->add($sock_parent);

    my $pid = fork;

    unless ( $pid )  {
        # child
        my $sock_child = \*STDOUT;
        child($sock_child, $id);
        shutdown($sock_child, 2);
        exit 0;
    }
    # parent
}

$SIG{CHLD} = \&REAPER;

sub REAPER {
    while ((my $pid = waitpid(-1,WNOHANG)) > 0) {
        $kids--;
#        delete $children{$pid};
    }
    $SIG{CHLD} = \&REAPER;
}


while ($kids) {
#    foreach my $child ($s->can_read(0)) {
#        my $string = do { local $/; <$child>};
#        my @events = split(/\n/, $string) if $string;
#        print join("\n", @events), "\n" if scalar @events;
#    }
}

sub child
{
    my ($sock, $id) = @_;

    unless ($print_stdout) {
        my $filename = hostname() . "." . $$ . ".txt";
        open my $fh, ">$filename" or die "can't open $filename: $!";

        open STDOUT, ">&", $fh or die "can't reopen STDOUT: $!";
        autoflush STDOUT 1;
    }

    my $fname = "${key}_$id";

    my $neb = Nebulous::Client::Bench->new(
#    proxy   => 'http://localhost:80/nebulous'
        proxy   => 'http://ipp008:80/nebulous'
#        proxy   => 'http://alala:80/nebulous',
    );

    test_prog($neb, $fname);
}

sub child_die
{
    my $sock = shift;
    print $sock @_;
    shutdown($sock, 2);
    exit 1;
}

package Nebulous::Client::Bench;

use base qw( Nebulous::Client );

#sub new
#{
#    my $class = shift;
#    my %p = @_;
#
#    my $sock = delete $p{sock};
#    my $self = $class->SUPER::new(%p);
#    $self->{sock} = $sock;
#
#    return $self;
#}

BEGIN {
sub make_wrapper
{
    my $method = shift;

eval "sub $method {"
.'    my $self = shift;'
.'    my $smark = Time::HiRes::time();'
.'    my $ret = $self->SUPER::' . "$method" .'(@_);'
.'    my $emark = Time::HiRes::time();'
.'    printf "%-17s %-17s %s\n", $emark, " ' . "$method" . ' ", ($emark - $smark), "\n";'
.'    return $ret;'
.'}';

}

make_wrapper("create");
make_wrapper("open_create");
make_wrapper("replicate");
make_wrapper("cull");
make_wrapper("lock");
make_wrapper("unlock");
make_wrapper("setxattr");
make_wrapper("getxattr");
make_wrapper("listxattr");
make_wrapper("removexattr");
make_wrapper("find_objects");
make_wrapper("find_instances");
#make_wrapper("find");
#make_wrapper("open");
#make_wrapper("delete");
#make_wrapper("copy");
make_wrapper("move");
make_wrapper("swap");
make_wrapper("delete_instance");
make_wrapper("stat");
make_wrapper("mounts");

}

1;
