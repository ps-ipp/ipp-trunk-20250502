#!/usr/bin/env perl

if (@ARGV != 1) { die "USAGE: checkpaths.pl (include/lib)\n"; }

if ($ARGV[0] eq "lib")     { &checklinker;}
if ($ARGV[0] eq "include") { &checkpreprocessor;}
die "invalid command to checkpaths.pl : $ARGV[0]\n";

sub checklinker {
    # we are going to supplement the libpath with entries reported by
    # the linker (this depends on ld --verbose 

    my $line, @lines;
    my $item, @items;

    @lines = `ld --verbose | grep SEARCH_DIR`;
    foreach $line (@lines) {
	# we expect items of the form SEARCH_DIR("path");
	# or SEARCH_DIR("=path") -- in that case we should prepend sysroo
	next unless ($line =~ m|SEARCH_DIR|);
	@items = split (";", $line);
	foreach $item (@items) {
	    next unless ($item);
	    ($p1) = $item =~ m|SEARCH_DIR\050"=(\S*)"|;
	    ($p2) = $item =~ m|SEARCH_DIR\050"(\S*)"|;
	    next if (!$p1 && !$p2);
	    if (!$p1 && $p2) {
		push @libpath, $p2;
	    }
	    if ($p1 && $p2) {
		push @libpath, $p1;
	    }
	    if ($p1 && !$p2) {
		print "programming error!\n";
		exit 4;
	    }
	}
    }
    print "@libpath\n";
    exit 0;
}

sub checkpreprocessor {
    # we are going to supplement the libpath with entries reported by
    # the linker (this depends on ld --verbose 

### #include <...> search starts here:
###  /home/eugene/src/psconfig/ipp-dev.linux/include
###  .
###  /usr/lib/gcc/i686-linux-gnu/4.6/include
###  /usr/local/include
###  /usr/lib/gcc/i686-linux-gnu/4.6/include-fixed
###  /usr/include/i386-linux-gnu
###  /usr/include
### End of search list.

    my $line, @lines;
    my $found_start;

    @lines = `cpp --verbose < /dev/null 2>&1`;
    $found_start = 0;
    foreach $line (@lines) {
	if (!$found_start) {
	    if ($line =~ m|include \<...\> search starts here|) {
		$found_start = 1;
		next;
	    } else {
		next;
	    }
	}
	chomp $line;
	if ($line =~ m|End of search list|) {
	    last;
	}
	($cleanline) = $line =~ m|\s*(\S*)|;
	push @incpath, $cleanline;
    }
    print "@incpath\n";
    exit 0;
}
