use Module::Build;
# See perldoc Module::Build for details of how this works

Module::Build->new(
    module_name         => 'Nebulous',
    dist_version_from   => 'lib/Nebulous/Server.pm',
    author              => 'Joshua Hoblitt <jhoblitt@cpan.org>',
    license             => 'gpl',
    create_makefile_pl  => 'passthrough',
    requires            => {
        'Apache2::Const'        => 0,
        'Cache::Memcached'      => 0,
        'Class::Accessor::Fast' => 0,
        'Config::YAML'          => '1.42',
        'DBD::mysql'            => '3.0007',
        'DBI'                   => '1.53',
        'File::Basename'        => 0,
        'File::ExtAttr'         => '1.03',
# needed for Ubuntu, not Gentoo
        'Digest::SHA'           => 0,
# not needed for Ubuntu
#       'Digest::SHA1'          => 0,
#       'File::Mountpoint'      => '0.01',
#       'Filesys::Df'           => '0.92',
        'File::Path'            => '1.08',
        'File::Spec'            => 0,
        'File::Spec::Functions' => 0,
        'File::Temp'            => 0,
        'Log::Dispatch::Email::MailSend' => 0,
        'Log::Log4perl'         => '0.48',
        'Net::Server::Daemonize'=> '0.05',
        'Params::Validate'      => '0.73',
        'SOAP::Lite'            => '0.69',
        'SQL::Interp'           => '1.06',
        'URI'                   => '1.30',
    },
    build_requires      => {
        'Test::More'            => '0.49',
#        'Test::URI'             => '1.06',
#        'Test::DBUnit'          => '0.20',
    },
    recommends          => {
        'Test::Distribution'    => '1.22',
# needed for Gentoo, not Ubuntu
#       'Apache2::SOAP'         => 0,
# needed for Ubuntu, not Gentoo
        'Apache::SOAP'         => 0,
        'Apache::DBI'           => '1.05',
    },
    script_files        => [qw(
        bin/neb-cabadd
        bin/neb-voladd
        bin/neb-voladm
        bin/neb-aliasadd
        bin/neb-insedit
        bin/neb-host
        bin/nebdiskd
    )],
)->create_build_script;

# these scripts are no longer installed (2019.10.31)
#       bin/neb-admin
#       bin/neb-fsck
