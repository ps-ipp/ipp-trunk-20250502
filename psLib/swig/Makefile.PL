# File : Makefile.PL
use ExtUtils::MakeMaker;
use FileHandle;

sub ReadSetup {
  my $filename = shift;
      #return val
  my %opts;

  my $fh = FileHandle->new($filename, 'r');
  while(<$fh>) {
    /([a-z]+)=(.+)/ && do {
      $opts{$1} = $2;
      next;
    };

    #default:
    die "Error:  syntaxt error in setup.txt line $_";
  }
  return %opts;
}

###############  Main ##############

my %opts = ReadSetup("setup.txt");


WriteMakefile(
	'NAME' => 'pslib',             # Name of package
	'INC' => $opts{includes},
	'LIBS' => ["$opts{ldflags}"],  # Name of custom libraries
	'OBJECT' => 'pslib_wrap.o',    # Name of object
	'OPTIMIZE' => "$opts{cflags} -w", # cflags with disabling of warnings (SWIG generated code generates warnings)
	'LD' => $opts{ld}
);
