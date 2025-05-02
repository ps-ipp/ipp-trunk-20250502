use Module::Build;
# See perldoc Module::Build for details of how this works

Module::Build->new(
    module_name         => 'DataStore',
    dist_version_from   => 'lib/DataStore.pm',
    author              => 'Joshua Hoblitt <jhoblitt@cpan.org>',
    license             => 'gpl',
    create_makefile_pl  => 'passthrough',
    requires            => {
        'Carp'                  => 0,
        'Data::Validate::URI'   => '0.01',
        'Digest::MD5::File'     => '0.03',
        'IPC::Cmd'              => '0.36',
        'File::Basename'        => 0,
        'File::Temp'            => '0.16',
        'File::Path'            => 0,
        'File::stat'            => 0,
        'LWP::UserAgent'        => 0,
        'Params::Validate'      => '0.77',
        'YAML'                  => '0.58',
    },
    build_requires      => {
        'CGI'                   => '3',
        'Net::HTTPServer'       => '1.1.1',
        'Test::Cmd'             => '1.05',
        'Test::More'            => 0,
        'Test::Warn'            => '0.08',
    },
    recommends          => {
        'Test::Distribution'    => '1.22',
    },
    script_files        => [qw(
        scripts/dsget
        scripts/dsgetfileset
        scripts/dsproductls
        scripts/dsleech
        scripts/dsfilesetls
        scripts/dsrootls
    )],
)->create_build_script;
