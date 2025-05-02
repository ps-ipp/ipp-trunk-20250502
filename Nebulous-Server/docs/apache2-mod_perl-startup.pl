use lib qw(/home/httpd/perl);

# enable if the mod_perl 1.0 compatibility is needed
#use Apache2::compat ();

use ModPerl::Util (); #for CORE::GLOBAL::exit

use Apache2::RequestRec ();
use Apache2::RequestIO ();
use Apache2::RequestUtil ();

use Apache2::ServerRec ();
use Apache2::ServerUtil ();
use Apache2::Connection ();
use Apache2::Log ();

use APR::Table ();

use ModPerl::Registry ();

use Apache2::Const -compile => ':common';
use APR::Const -compile => ':common';

use Apache::DBI;
use DBI;
use Nebulous::Server::SOAP;
use Nebulous::Server::Apache;
use Nebulous::Server;

my $dsn         = 'DBI:mysql:database=nebulous:host=nebulous.ipp.ifa.hawaii.edu';
my $dbuser      = 'nebulous';
my $dbpasswd    = 'XXXX';

#$Apache::DBI::DEBUG = 1;
#Apache::DBI->connect_on_init( $dsn, $dbuser, $dbpasswd );
Apache::DBI->setPingTimeOut($dsn, 10);

my $config = Nebulous::Server::Config->new(
    trace       => 'warn',
					   );
$config->add_db(
    dbindex     => 0,
    dsn         => $dsn,
    dbuser      => $dbuser,
    dbpasswd    => $dbpasswd,
		);

Nebulous::Server::SOAP->new_on_init($config);


1;
