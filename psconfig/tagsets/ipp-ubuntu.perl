# this is the full table of perl modules needed by IPP operations code.  modules needed only by Nebulous-Server
# might not be in this file. The fields below are as follows:
# |-- Group (sequence in a group should not be critical)
# |   Name                          Tarball                                  Version        Build     Responses 
# |                                 (in ../extperl/)                                        Opts      
#

# these modules needed by various modules below, so they need to be first
  00  Module::Build                 Module-Build-0.4224.tar.gz               0              NONE      NONE
  00  Module::Runtime               Module-Runtime-0.016.tar.gz              0              NONE      NONE
  00  Exporter::Tiny                Exporter-Tiny-1.002001.tar.gz            0              NONE      NONE
  00  Digest::HMAC_MD5              Digest-HMAC-1.03.tar.gz                  0              NONE      NONE
  00  XML::Parser                   XML-Parser-2.44.tar.gz                   0              NONE      NONE
  00  Net::Domain::TLD              Net-Domain-TLD-1.75.tar.gz               0              NONE      NONE
  00  Test2::Plugin::UTF8           Test2-Suite-0.000119.tar.gz              0              NONE      NONE
  00  HTTP::Date                    HTTP-Date-6.02.tar.gz		     0              NONE      NONE
  00  Try::Tiny                     Try-Tiny-0.30.tar.gz		     0              NONE      NONE

  00a LWP::MediaTypes               LWP-MediaTypes-6.04.tar.gz               0              NONE      NONE
  00a URI                           URI-1.76.tar.gz                          1.76           NONE      NONE
  00a Mail::Internet                MailTools-2.20.tar.gz           	     0              NONE      NONE
  00a Net::HTTP                     Net-HTTP-6.18.tar.gz                     0              NONE      NONE

  00b Class::Accessor::Fast           Class-Accessor-0.51.tar.gz      	     0              NONE      NONE
  00  Parse::RecDescent               Parse-RecDescent-1.967015.tar.gz	     0              NONE      NONE

# these modules are free of inter-dependencies:
  01  AnyEvent                      AnyEvent-7.15.tar.gz                     0              NONE      NONE
  01  Authen::NTLM                  NTLM-1.09.tar.gz			     0              NONE      NONE
  01  AutoLoader                    AutoLoader-5.74.tar.gz		     0              NONE      NONE
  01  CPAN::Meta::Check             CPAN-Meta-Check-0.014.tar.gz             0              NONE      NONE
  01  Class::Data::Inheritable      Class-Data-Inheritable-0.08.tar.gz	     0              NONE      NONE
  01  Class::Inspector              Class-Inspector-1.34.tar.gz		     1.34           NONE      NONE
  01  Class::Prototyped             Class-Prototyped-1.13.tar.gz	     0              NONE      NONE
  01  Class::Singleton              Class-Singleton-1.5.tar.gz		     0              NONE      NONE
  01  Encode::Locale                Encode-Locale-1.05.tar.gz		     0              NONE      NONE
  01  Data::Dump                    Data-Dump-1.23.tar.gz		     0              NONE      NONE
  01  Data::Validate::Domain        Data-Validate-Domain-0.14.tar.gz	     0              NONE      NONE
  01  Date::Manip                   Date-Manip-6.76.tar.gz		     0              NONE      NONE
  01  Date::Parse                   TimeDate-2.30.tar.gz		     0              NONE      NONE
  01  Devel::CheckOS                Devel-CheckOS-1.81.tar.gz		     0              NONE      NONE
  01  Devel::StackTrace             Devel-StackTrace-2.03.tar.gz	     0              NONE      NONE
  01  Eval::Closure                 Eval-Closure-0.14.tar.gz		     0              NONE      NONE
  01  ExtUtils::TBone               ExtUtils-TBone-1.124.tar.gz		     0              NONE      NONE
  01  File::Listing                 File-Listing-6.04.tar.gz		     0              NONE      NONE
  01  File::NFSLock                 File-NFSLock-1.29.tar.gz		     0              NONE      NONE
  01  File::ShareDir                File-ShareDir-1.116.tar.gz               0              NONE      NONE
  01  File::ShareDir::Install       File-ShareDir-Install-0.13.tar.gz	     0              NONE      NONE
  01  HTTP::Request                 HTTP-Message-6.18.tar.gz		     6.18           NONE      NONE
  01  Heap::Elem                    Heap-0.80.tar.gz			     0              NONE      NONE
  01  IO::Pipely                    IO-Pipely-0.005.tar.gz		     0              NONE      NONE
  01  IO::Pty                       IO-Tty-1.12.tar.gz			     1.12           NONE      NONE
  01  IO::String                    IO-String-1.08.tar.gz		     1.08           NONE      NONE
  01  IPC::Run3                     IPC-Run3-0.048.tar.gz		     0              NONE      NONE
  01  List::MoreUtils               List-MoreUtils-0.428.tar.gz		     0.428          NONE      NONE
  01  MIME::Entity                  MIME-tools-5.509.tar.gz		     0              NONE      NONE
  01  MIME::Types                   MIME-Types-2.17.tar.gz		     0              NONE      NONE
  01  MLDBM                         MLDBM-2.05.tar.gz			     0              NONE      NONE
  01  MRO::Compat                   MRO-Compat-0.13.tar.gz		     0              NONE      NONE
  01  Module::Implementation        Module-Implementation-0.09.tar.gz	     0              NONE      NONE
  01  Module::Metadata              Module-Metadata-1.000033.tar.gz	     0              NONE      NONE
  01  NetAddr::IP                   NetAddr-IP-4.079.tar.gz		     0              NONE      NONE
  01  Number::Compare               Number-Compare-0.03.tar.gz		     0              NONE      NONE
  01  POE::Test::Loops              POE-Test-Loops-1.360.tar.gz		     0              NONE      NONE
  01  Params::Util                  Params-Util-1.07.tar.gz		     0              NONE      NONE
  01  Path::Tiny                    Path-Tiny-0.108.tar.gz		     0              NONE      NONE
  01  Perl::OSType                  Perl-OSType-1.010.tar.gz		     0              NONE      NONE
  01  Pod::POM                      Pod-POM-2.01.tar.gz			     0              NONE      NONE
  01  Role::Tiny                    Role-Tiny-2.000006.tar.gz                0              NONE      NONE
  01  Specio                        Specio-0.43.tar.gz			     0              NONE      NONE
  01  Spiffy                        Spiffy-0.46.tar.gz			     0              NONE      NONE
  01  Sub::Exporter::Progressive    Sub-Exporter-Progressive-0.001013.tar.gz 0              NONE      NONE
  01  Sub::Identify                 Sub-Identify-0.14.tar.gz		     0              NONE      NONE
  01  Sub::Install                  Sub-Install-0.928.tar.gz		     0              NONE      NONE
  01  Sub::Uplevel                  Sub-Uplevel-0.2800.tar.gz		     0              NONE      NONE
  01  Test2::Plugin::NoWarnings     Test2-Plugin-NoWarnings-0.06.tar.gz	     0              NONE      NONE
  01  Test2::Require::Module        Test2-Suite-0.000119.tar.gz		     0              NONE      NONE
  01  Test::FailWarnings            Test-FailWarnings-0.008.tar.gz	     0              NONE      NONE
  01  Test::Fatal                   Test-Fatal-0.014.tar.gz		     0              NONE      NONE
  01  Test::File::ShareDir          Test-File-ShareDir-1.001002.tar.gz       0              NONE      NONE
  01  Test::Needs                   Test-Needs-0.002006.tar.gz		     0              NONE      NONE
  01  Test::Requires                Test-Requires-0.10.tar.gz		     0              NONE      NONE
  01  Test::Warnings                Test-Warnings-0.026.tar.gz		     0              NONE      NONE
  01  Test::Without::Module         Test-Without-Module-0.20.tar.gz	     0              NONE      NONE
# trouble building this on clean 20.04 server.  Is it needed?
# 01  Text::Diff                    Text-Diff-1.45.tar.gz		     0              NONE      NONE
  01  Text::Glob                    Text-Glob-0.11.tar.gz		     0              NONE      NONE
  01  Text::Template                Text-Template-1.55.tar.gz		     0              NONE      NONE
  01  Tie::CPHash                   Tie-CPHash-2.000.tar.gz		     0              NONE      NONE
  01  Variable::Magic               Variable-Magic-0.62.tar.gz               0              NONE      NONE
  01  XML::Parser::PerlSAX          libxml-perl-0.08.tar.gz 		     0              NONE      NONE
  01  XML::RegExp                   XML-RegExp-0.04.tar.gz		     0              NONE      NONE

  00  HTTP::Message                 HTTP-Message-6.18.tar.gz		     0              NONE      NONE -- URI, LWP::MediaTypes, Try::Tiny
  01  HTTP::Negotiate               HTTP-Negotiate-6.01.tar.gz		     6.01           NONE      NONE -- HTTP::Message

  01  Dist::CheckConflicts          Dist-CheckConflicts-0.11.tar.gz	     0              NONE      NONE -- Module::Runtime, Test::Fatal

  02  Package::Stash                Package-Stash-0.38.tar.gz		     0              NONE      NONE -- Dist::CheckConflicts, Module::Implementation, Package::Stash::XS, Test::Fatal, Test::Requires
  02  Test::Base                    Test-Base-0.89.tar.gz		     0              NONE      NONE -- Spiffy, Text::Diff
  02  Test::Warn                    Test-Warn-0.36.tar.gz		     0              NONE      NONE -- Sub::Uplevel

# the order here matters due to dependency chain:
  03a Data::OptList                 Data-OptList-0.110.tar.gz		     0              NONE      NONE -- Sub::Install
  03b Sub::Exporter                 Sub-Exporter-0.987.tar.gz		     0              NONE      NONE -- Data::OptList, Params::Util, Sub::Install
  03c Devel::GlobalDestruction      Devel-GlobalDestruction-0.14.tar.gz	     0              NONE      NONE -- Sub::Exporter::Progressive
  03d Exception::Class              Exception-Class-1.44.tar.gz		     0              NONE      NONE -- Class::Data::Inheritable, Devel::StackTrace
  03e B::Hooks::EndOfScope          B-Hooks-EndOfScope-0.24.tar.gz           0              NONE      NONE -- Sub::Exporter::Progressive, Variable::Magic
  03f namespace::clean              namespace-clean-0.27.tar.gz              0              NONE      NONE -- B::Hooks::EndOfScope, Package::Stash
  03g namespace::autoclean          namespace-autoclean-0.28.tar.gz	     0              NONE      NONE -- B::Hooks::EndOfScope, Sub::Identify, Test::Requires, namespace::clean
  03h Params::ValidationCompiler    Params-ValidationCompiler-0.30.tar.gz    0              NONE      NONE -- Eval::Closure, Exception::Class, Specio, Test2::Plugin::NoWarnings, Test2::Require::Module, Test2::V0, Test::Without::Module
  03i DateTime::Locale              DateTime-Locale-1.24.tar.gz		     1.24           NONE      NONE -- Role::Tiny, namespace::autoclean, Params::ValidationCompiler
  03j DateTime::TimeZone            DateTime-TimeZone-2.34.tar.gz	     0              NONE      NONE -- Class::Singleton, Params::ValidationCompiler, Specio::Library::Builtins, Specio::Library::String, Test::Fatal, Test::Requires, namespace::autoclean,
  03k DateTime                      DateTime-1.50.tar.gz                     0              NONE      NONE -- CPAN::Meta::Check, DateTime::Locale, Test::Warnings
  03l DateTime::Format::Strptime    DateTime-Format-Strptime-1.76.tar.gz     1.76           NONE      NONE -- DateTime::Locale, Test::Warnings
# DateTime complains about DateTime::Format::Strptime with unknown format but succeeds
# DateTime::Format::Strptime requires DateTime

  05  Params::Validate              Params-Validate-1.29.tar.gz              0              NONE      NONE
  19  DateTime::Format::Builder     DateTime-Format-Builder-0.82.tar.gz      0              NONE      NONE -- Params::Validate
  03m DateTime::Format::ISO8601     DateTime-Format-ISO8601-0.08.tar.gz      0              NONE      NONE -- DateTime?

  04a Data::Validate::IP            Data-Validate-IP-0.27.tar.gz	     0              NONE      NONE -- NetAddr::IP, Test::Requires
  04b Data::Validate::URI           Data-Validate-URI-0.07.tar.gz            0              NONE      NONE -- Data::Validate::IP

# these modules are required by IPP and do NOT depend on modules above (REALLY? NONE?)
  05  Astro::FITS::CFITSIO          Astro-FITS-CFITSIO-1.12.tar.gz           0              NONE      NONE -- libcfitsio
  05  Cache                         Cache-2.11.tar.gz                        0              NONE      NONE
  05  YAML                          YAML-1.27.tar.gz                         0              NONE      NONE
  05  Config::YAML                  Config-YAML-1.42.tar.gz                  0              NONE      NONE
  05  DBI                           DBI-1.642.tar.gz                         0              NONE      NONE
  05  IPC::Run                      IPC-Run-20180523.0.tar.gz                20180523       NONE      NONE
  05  IPC::Cmd                      IPC-Cmd-1.0201.tar.gz                    1.0201         NONE      NONE
  05  LWP                           libwww-perl-6.38.tar.gz                  6.38           NONE      NONE
  05  Log::Log4perl                 Log-Log4perl-1.49.tar.gz                 0              NONE      NONE
  05  SOAP::Lite                    SOAP-Lite-1.27.tar.gz                    0              NONE      NONE
  05  Statistics::Descriptive       Statistics-Descriptive-3.0702.tar.gz     0              NONE      NONE
# a response is needed: (n,n) or (y,y) : (build XStash, use XStash)
  05  Template                      Template-Toolkit-2.28.tar.gz             0              NONE      y,y  -- (build XStash, use XStash) : was n,n

  05  Digest::MD5::File             Digest-MD5-File-0.08.tar.gz              0              NONE      NONE -- LWP

# these modules depend on modules in groups 00 - 05
  06  DBD::Mock                     DBD-Mock-1.45.tar.gz		     0              NONE      NONE -- DBI
  06  DBIx::Interp                  SQL-Interp-1.24.tar.gz		     0              NONE      NONE -- DBI
  06  DBIx::Simple                  DBIx-Simple-1.37.tar.gz		     0              NONE      NONE -- DBI
  06  File::Find::Rule              File-Find-Rule-0.34.tar.gz		     0              NONE      NONE -- Number::Compare, Text::Glob

  07  Log::Dispatch::File           Log-Dispatch-2.68.tar.gz		     0              NONE      NONE -- Devel::GlobalDestruction, IPC::Run3, Params::ValidationCompiler, Specio, Test::Fatal, Test::Needs, namespace::autoclean
  07  Log::Dispatch::FileRotate     Log-Dispatch-FileRotate-1.36.tar.gz	     0              NONE      NONE -- Date::Manip, Path::Tiny, Test::Warn, Log::Dispatch::File
  07  MIME::Lite                    MIME-Lite-3.030.tar.gz		     0              NONE      NONE -- MIME::Types
  07  POE                           POE-1.367.tar.gz			     0              NONE      n    -- IO::Pipely, POE::Test::Loops

## the response (n) says that we do not want to skip network tests
  08  Test::YAML                    Test-YAML-1.07.tar.gz		     0              NONE      NONE -- Test::Base
  08  XML::DOM                      XML-DOM-1.46.tar.gz                      0              NONE      NONE -- XML::Parser::PerlSAX, XML::RegExp

  00c Devel::CheckLib               Devel-CheckLib-1.13.tar.gz               0              NONE      NONE -- unknown
  00d DBD::mysql                    DBD-mysql-4.050.tar.gz                   0              NONE      NONE -- unknown

