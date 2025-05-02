use Module::Build;
# See perldoc Module::Build for details of how this works

my $build_pkg   = eval { require Apache::TestMB }
                ? 'Apache::TestMB'
                : 'Module::Build';

my $class = $build_pkg->subclass(code => <<'EOF');
use Cwd;

my $pkg_dir = "./nebclient";

# sub ACTION_code {
#     my $self = shift;
# 
#     $self->SUPER::ACTION_code(@_);
# 
#     my $old_pwd = getcwd();
#     chdir $pkg_dir;
# 
#     unless (-e "configure") {
#         system("./autogen.sh") == 0 or die "install failed: $?";
#     }
# 
#     chdir $old_pwd;
# }
# 
# sub ACTION_build {
#     my $self = shift;
# 
#     $self->SUPER::ACTION_build(@_);
# 
#     my $old_pwd = getcwd();
#     chdir $pkg_dir;
# 
#     unless (-e "Makefile") {
#         system("sh ./configure") == 0 or die "build failed: $?";
#     }
# 
#     system("make") == 0 or die "build failed: $?";
# 
#     chdir $old_pwd;
# }
# 
# sub ACTION_install {
#     my $self = shift;
# 
#     $self->SUPER::ACTION_install(@_);
# 
#     my $old_pwd = getcwd();
#     chdir $pkg_dir;
# 
#     system("make install") == 0 or die "install failed: $?";
# 
#     chdir $old_pwd;
# }
# 
# sub ACTION_clean {
#     my $self = shift;
# 
#     $self->SUPER::ACTION_clean(@_);
# 
#     my $old_pwd = getcwd();
#     chdir $pkg_dir;
# 
#     system("make clean") == 0 or die "install failed: $?";
# 
#     chdir $old_pwd;
# }
EOF

$class->new(
    module_name         => 'Nebulous',
    dist_version_from   => 'lib/Nebulous/Client.pm',
    author              => 'Joshua Hoblitt <jhoblitt@cpan.org>',
    license             => 'gpl',
    create_makefile_pl  => 'passthrough',
    requires            => {
        'Digest::MD5'           => 0,
        'File::Copy'            => 0,
        'Time::HiRes'           => 0,
        'Class::Accessor::Fast' => 0,
        'File::Spec'            => 0,
        'File::Spec::Functions' => 0,
        'Log::Log4perl'         => '0.48',
        'Params::Validate'      => '0.73',
        'SOAP::Lite'            => '0.69',
        'URI'                   => '1.30',
        'Getopt::Long'          => 0,
        'Pod::Usage'            => 0,
    },
    recommends          => {
        'Nebulous::Server::Apache'  => 0,
        'Nebulous::Server'          => 0,
        'Nebulous::Server::SOAP'    => 0,
        'Test::Nebulous'            => 0,
        'Test::Cmd'                 => '1.05',
# gentoo: 'Apache2::SOAP'             => 0,
        'Apache::SOAP'             => 0, # needed instead for ubuntu 18.04
        'Apache::DBI'               => '1.05',
        'Apache::Test'              => '1.27',  # bundles Apache::TestMB
        'Apache::TestMB'            => 0,
        'Test::More'                => '0.49',
        'Test::URI'                 => '1.06',
        'Test::Distribution'        => '1.22',
    },
    script_files        => [qw(
        bin/neb-cat
        bin/neb-copies
        bin/neb-cp
        bin/neb-cull
        bin/neb-df
        bin/neb-insert
        bin/neb-locate
        bin/neb-ls
        bin/neb-migrate
        bin/neb-mv
        bin/neb-replicate
        bin/neb-repair
        bin/neb-rm
        bin/neb-shift
        bin/neb-stat
        bin/neb-swap
        bin/neb-touch
        bin/neb-xattr
        bin/whichnode
    )],
)->create_build_script;
