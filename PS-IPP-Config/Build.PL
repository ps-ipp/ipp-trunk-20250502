use Module::Build;
# See perldoc Module::Build for details of how this works

Module::Build->new(
    module_name         => 'PS::IPP::Config',
    dist_version_from   => 'lib/PS/IPP/Config.pm',
    author              => 'Paul Price <price@ifa.hawaii.edu> and Joshua Hoblitt <jhoblitt@cpan.org>',
    license             => 'gpl',
    create_makefile_pl  => 'passthrough',
    requires            =>  {
        'Carp'                      => 0,
        'Class::Accessor::Fast'     => '0.19',
        'File::Spec'                => '0.87',
        'Getopt::Long'              => '2.33',
	'IPC::Cmd'                  => '0.36',
        'PS::IPP::Metadata::Config' => '1.00',
        'Statistics::Descriptive'   => '2.6',
        'URI'                       => 0,
    },
    recommends          => {
        'Test::Distribution'        => '1.22',
    },
    dist_abstract => 'Configuration module for Pan-STARRS IPP.',

)->create_build_script;
