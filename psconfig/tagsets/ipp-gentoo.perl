# notes:
# Build.PL/Makefile.PL options are added to the line "perl Build.PL / perl Makefile.PL", a value of NONE is required if no options are desired
# Build.PL/Makefile.PL responses are supplied via stdin to "perl Build.PL / perl Makefile.PL", a value of NONE is required if no responses are desired

# NN    Name                           Tarball                                  Version        Build.PL/Makefile.PL Build.PL/Makefile.PL responses
# a Version of 0 is required if no specific version is desired                                 options   responses
  00    Getopt::Long                   Getopt-Long-2.36.tar.gz                  2.3            NONE      n
# 00    Module::Build                  Module-Build-0.40.tar.gz                 0.38           NONE      NONE # special comment here
  00    Module::Build                  Module-Build-0.3601.tar.gz               0.3601         NONE      NONE # special comment here
  01    ExtUtils::MakeMaker            ExtUtils-MakeMaker-6.54.tar.gz           0              NONE      NONE
  02a   Attribute::Handlers            Attribute-Handlers-0.87.tar.gz           0.79           NONE      NONE
# Params::Validate 0.92 breaks with Perl >= 5.14 (and probably earlier) because 'ref' added RegExp as a type
  02    Params::Validate               Params-Validate-0.96.tar.gz              0.96           NONE      NONE
# 02    Apache::Test                   Apache-Test-1.29.tar.gz                  1.29           NONE      NONE
  03a   Class::Singleton               Class-Singleton-1.4.tar.gz               0              NONE      NONE
  03    DateTime::TimeZone             DateTime-TimeZone-0.59.tar.gz            0              NONE      NONE
  04    DateTime::Locale               DateTime-Locale-0.33.tar.gz              0              NONE      NONE
  05    Time::Local                    Time-Local-1.17.tar.gz                   0              NONE      NONE
  06    DateTime                       DateTime-0.36.tar.gz                     0              NONE      NONE
  07    MIME::Base64                   MIME-Base64-3.07.tar.gz                  0              NONE      NONE
  08    IO::Compress::Base             IO-Compress-Base-2.003.tar.gz            0              NONE      NONE
  09    Compress::Raw::Zlib            Compress-Raw-Zlib-2.003.tar.gz           0              NONE      NONE
  10    Class::Factory::Util           Class-Factory-Util-1.6.tar.gz            0              NONE      NONE
  11    DateTime::Format::Strptime     DateTime-Format-Strptime-1.0700.tar.gz   0              NONE      NONE
  12    Net::Domain::TLD               Net-Domain-TLD-1.65.tar.gz               0              NONE      NONE
  13    Sub::Uplevel                   Sub-Uplevel-0.14.tar.gz                  0              NONE      NONE
  14    HTML::Tagset                   HTML-Tagset-3.10.tar.gz                  0              NONE      NONE
  15    Digest                         Digest-1.15.tar.gz                       0              NONE      NONE
  16    IO::Compress::Zlib::Extra      IO-Compress-Zlib-2.003.tar.gz            0              NONE      NONE
  17    version                        version-0.70.tar.gz                      0              NONE      NONE
  18    Text::Balanced                 Text-Balanced-v2.0.0.tar.gz              0              NONE      NONE
  19    DateTime::Format::Builder      DateTime-Format-Builder-0.7807.tar.gz    0              NONE      NONE
  20    ExtUtils::Manifest             ExtUtils-Manifest-1.51.tar.gz            0              NONE      NONE
  21    URI                            URI-1.35.tar.gz                          1.30           NONE      NONE
  22    Data::Validate::Domain         Data-Validate-Domain-0.05.tar.gz         0              NONE      NONE
  23    Test::Exception                Test-Exception-0.24.tar.gz               0              NONE      NONE
  24    Tree::DAG_Node                 Tree-DAG_Node-1.05.tar.gz                0              NONE      NONE
  25    Array::Compare                 Array-Compare-1.13.tar.gz                0              NONE      NONE
  26    HTML::Parser                   HTML-Parser-3.56.tar.gz                  0              NONE      NONE
  27    Digest::MD5                    Digest-MD5-2.36.tar.gz                   0              NONE      NONE
  28    Net::FTP                       libnet-1.19.tar.gz                       0              NONE      n
  29    Compress::Zlib                 Compress-Zlib-2.003.tar.gz               0              NONE      NONE
  30    Locale::Maketext::Simple       Locale-Maketext-Simple-0.18.tar.gz       0              NONE      NONE
  31    Parse::RecDescent              Parse-RecDescent-1.94.tar.gz             1.94           NONE      NONE
  32    Class::Accessor                Class-Accessor-0.30.tar.gz               0.19           NONE      NONE
  33    DateTime::Format::ISO8601      DateTime-Format-ISO8601-0.06.tar.gz      0.06           NONE      NONE
  34    CGI                            CGI.pm-3.25.tar.gz                       3              NONE      NONE
  35    Test::Cmd                      Test-Cmd-1.05.tar.gz                     1.05           NONE      NONE
  36    Net::HTTPServer                Net-HTTPServer-1.1.1.tar.gz              1.1.1          NONE      NONE
  37    LWP                            libwww-perl-5.805.tar.gz                 0              NONE      NONE
  38    Digest::MD5::File              Digest-MD5-File-0.05.tar.gz              0.03           NONE      NONE
  39    File::Temp                     File-Temp-0.18.tar.gz                    0.16           NONE      NONE
  40    Data::Validate::URI            Data-Validate-URI-0.01.tar.gz            0.01           NONE      NONE
  41    Test::Warn                     Test-Warn-0.08.tar.gz                    0              NONE      NONE
  42    YAML                           YAML-0.62.tar.gz                         0.58           NONE      y
  43    Module::Load                   Module-Load-0.10.tar.gz                  0              NONE      NONE
  44    Params::Check                  Params-Check-0.25.tar.gz                 0              NONE      NONE
  45    Template                       Template-Toolkit-2.16.tar.gz             0              NONE      n,n
  46    Statistics::Descriptive        Statistics-Descriptive-2.6.tar.gz        2.6            NONE      NONE
  47    Storable                       Storable-2.15.tar.gz                     0              NONE      NONE
  48    IO::String                     IO-String-1.08.tar.gz                    0              NONE      NONE
  49    Date::Parse                    TimeDate-1.16.tar.gz                     0              NONE      NONE
  50    Digest::SHA1                   Digest-SHA1-2.11.tar.gz                  0              NONE      NONE
  51    DB_File                        DB_File-1.814.tar.gz                     0              NONE      NONE
  52    File::NFSLock                  File-NFSLock-1.20.tar.gz                 0              NONE      NONE
  53    Heap                           Heap-0.71.tar.gz                         0              NONE      NONE
  54    Module::Load::Conditional      Module-Load-Conditional-0.16.tar.gz      0              NONE      NONE
  55    IPC::Run                       IPC-Run-0.80.tar.gz                      0              NONE      NONE
  56    Cache                          Cache-2.04.tar.gz                        0              NONE      NONE
  57    IPC::Cmd                       IPC-Cmd-0.36.tar.gz                     =0.36           NONE      NONE
  58    SOAP::Lite                     SOAP-Lite-0.69.v1.tar.gz                 0              NONE      yes,yes,no
  59    Log::Log4perl                  Log-Log4perl-1.10.v1.tar.gz              0              NONE      NONE
# 60    File::ExtAttr                  File-ExtAttr-1.04.tar.gz                 0              NONE      NONE
  61    Text::Glob                     Text-Glob-0.08.tar.gz                    0.08           NONE      NONE
  62    Number::Compare                Number-Compare-0.01.tar.gz               0.01           NONE      NONE
  63    File::Find::Rule               File-Find-Rule-0.30.tar.gz               0.30           NONE      NONE
  64    Astro::FITS::CFITSIO           Astro-FITS-CFITSIO-1.05.tar.gz           0              NONE      NONE
  65    Test::More                     Test-Simple-0.74.tar.gz                  0.49           NONE      NONE
# 66    Apache::DBI                    Apache-DBI-1.06.tar.gz                   0              NONE      NONE
# 67    Apache2::SOAP                  Apache2-SOAP-0.72.tar.gz                 0              NONE      NONE
  68    Test::URI                      Test-URI-1.08.tar.gz                     0              NONE      NONE
# 69    Sys::Statistics::Linux::DiskUsage Sys-Statistics-Linux-0.26.tar.gz      0              NONE      NONE
  70    Config::YAML                   Config-YAML-1.42.tar.gz                  0              NONE      NONE
# 72    File::ExtAttr                  File-ExtAttr-1.07.tar.gz                 0              NONE      NONE
# version 1.622 updates sv_undef and related to new namespace (PL_*) (needed as of Perl 5.13.XX)
  73    DBI                            DBI-1.622.tar.gz                         1.622          NONE      NONE
# version 4.021 updates sv_undef and related to new namespace (PL_*) (needed as of Perl 5.13.XX)
  71    DBD::mysql                     DBD-mysql-4.021.tar.gz                   4.021          NONE      NONE

# 74    Net::Server::Daemonize         Net-Server-0.97.tar.gz                   0.05           NONE      NONE
  75    File::Path                      File-Path-2.04.tar.gz                   0              NONE      NONE
  76    File::Mountpoint                File-Mountpoint-0.01.tar.gz             0.01           NONE      NONE
  77    Filesys::Df                     Filesys-Df-0.92.tar.gz                  0.92           NONE      NONE
  78    SQL::Interp                     SQL-Interp-1.06.tar.gz                  0              NONE      NONE
  79a   Mail::Send                      MailTools-2.04.tar.gz                   0              NONE      NONE
  79b   Log::Dispatch::Email::MailSend  Log-Dispatch-2.22.tar.gz                0              NONE      NONE
  80    Abstract::Meta::Class          Abstract-Meta-Class-0.13.tar.gz          0              NONE      NONE
  81    DBIx::Connection               DBIx-Connection-0.13.tar.gz              0              NONE      NONE
  82a   Pod::Escapes                   Pod-Escapes-1.04.tar.gz                  0              NONE      NONE
  82b   Pod::Simple                    Pod-Simple-3.14.tar.gz                   0              NONE      NONE
  82c   Test::Pod                      Test-Pod-1.40.tar.gz                     0              NONE      NONE
  82d   XML::NamespaceSupport          XML-NamespaceSupport-1.10.tar.gz         0              --skip    NONE
  82e   XML::SAX                       XML-SAX-0.96.tar.gz                      0              NONE      Y
  82f   Simple::SAX::Serializer        Simple-SAX-Serializer-0.05.tar.gz        0              NONE      NONE
  83    Test::Distribution             Test-Distribution-2.00.tar.gz            0              NONE      NONE
  84    Test::DBUnit                   Test-DBUnit-0.20.tar.gz                  0.20           NONE      NONE
