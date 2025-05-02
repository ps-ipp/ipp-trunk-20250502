package tap;

# our @EXPORT_OK = qw (&ok &plan_tests &done_tests);

sub ok {

    my $value = $_[0];
    my $message = $_[1];

    if ($value) {
	print "ok $ntest - $message\n";
    } else {
	print "not ok $ntest - $message\n";
	$Nfail ++;
    }
    $ntest ++;
    return 1;
}

sub plan_tests {

    $ntest = 0;
    $Ntest = $_[0];
    $Nfail = 0;
    return 1;
}

sub done_tests {

    if ($ntest != $Ntest) {
	print STDERR "planned tests ($Ntest) not equal to done tests ($ntest)\n";
    }

    if ($Nfail > 0) {
	print STDERR "failed $Nfail of $ntest tests\n";
	exit 1;
    }

    if ($Nfail == 0) {
	print STDERR "passed $ntest tests\n";
	exit 0;
    }
}

1;
