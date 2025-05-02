use Module::Build;
# See perldoc Module::Build for details of how this works

Module::Build->new(
    module_name         => 'PS::IPP::PStamp',
    dist_version_from   => 'lib/PS/IPP/PStamp.pm',
    author              => 'Bill Sweeney',
    license             => 'gpl',
    create_makefile_pl  => 'passthrough',
    requires            => {
        'IPC::Cmd'              => '0.36',
        'PS::IPP::Config'       => '1.01',
        'PS::IPP::Metadata::List' => '0.01',
    },
)->create_build_script;
