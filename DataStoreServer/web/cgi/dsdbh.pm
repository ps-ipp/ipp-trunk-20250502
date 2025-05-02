# create a database handle based on the configuration given
# in environment variables

sub getDBHandle {
    my $dbserver = $ENV{DBSERVER};
    my $dbuser   = $ENV{DBUSER};
    my $dbpass   = $ENV{DBPASSWORD};
    my $dbname   = $ENV{DBNAME};

    die "database enviornment not set up" unless defined($dbserver) and defined($dbuser)
        and defined($dbpass) and defined($dbname);

    my $dsn = "DBI:mysql:host=$dbserver;database=$dbname";

    my $dbh = DBI->connect($dsn, $dbuser, $dbpass) or die "Cannot connect to server\n";

    return $dbh;
}

return 1;
