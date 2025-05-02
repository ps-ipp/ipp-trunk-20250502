use Module::Build;
# See perldoc Module::Build for details of how this works

my $class = Module::Build->subclass(code => <<'EOF');
use File::Copy;
use File::stat;

my $grammar = "config_grammar.txt";
my $parser  = "lib/PS/IPP/Metadata/Parser.pm";

sub ACTION_code {
    my $self = shift;

    if (!-e $parser || (stat($grammar)->mtime > stat($parser)->mtime)) {
        system("perl -MParse::RecDescent - $grammar PS::IPP::Metadata::Parser") == 0
            or die "Parse::RecDecent code gen failed: $?";
        move("Parser.pm", $parser) or die "move failed: $!";
    }

    $self->SUPER::ACTION_code(@_);
}
EOF

$class->new(
    module_name         => 'PS::IPP::Metadata::Config',
    dist_version_from   => 'lib/PS/IPP/Metadata/Config.pm',
    author              => 'Joshua Hoblitt <jhoblitt@cpan.org>',
    license             => 'gpl',
    create_makefile_pl  => 'passthrough',
    requires            =>  {
        'Carp'                      => 0,
        'Class::Accessor::Fast'     => '0.19',
        'Params::Validate'          => '0.72',
        'DateTime::Format::ISO8601' => '0.0402',
        'Parse::RecDescent'         => '1.94',
    },
    recommends          => {
        'Test::Distribution'    => '1.22',
    },
    script_files        => [qw(
        scripts/mdc-dump
    )],
)->create_build_script;
