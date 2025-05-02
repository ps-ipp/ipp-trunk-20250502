## simtest.pro : automatic full-suite simulated IPP test : -*- sh -*-

## there are four features which define a simtest run:
## the camera : any camera can be used, the sim... cameras are defined to be quick tests
## the sequence : this file defines the set of observations generated
## the auto : the automation file defines the sequence of analysis performed
## the recipe : the ppsim recipe defines the features which are included in the generate data

## these are not completely independent: a certain sequence may be
## required to yield a sensible result for a specific auto, and a
## specific recipe may also be needed

if ($?PPSIM_RECIPE == 0) 
  $PPSIM_RECIPE = default
end
if ($?SIMTEST_CAMERA == 0)
  $SIMTEST_CAMERA = SIMTEST
end
if ($?SIMTEST_SEQUENCE == 0)
  $SIMTEST_SEQUENCE = simtest.basic.config
end
if ($?SIMTEST_AUTO == 0)
  $SIMTEST_AUTO = simtest.basic.auto
end
if ($?SIMTEST_THREADS == 0) 
  $SIMTEST_THREADS = 0
end

$SIMTEST_RAWDIR = raw
$SIMTEST_WORKDIR = work
  

macro simtest
  if ($0 != 4)
    echo "USAGE: simtest (dbname) (hostname) (init)"
    echo " if (init) == run    : just run the analysis tasks"
    echo " if (init) == inject : (re)create the database and (re)inject the images"
    echo " if (init) == new    : restart completely from scratch"
    echo ""
    echo "there are additional options which may be specified by setting the following variable names:" 
    echo "  PPSIM_RECIPE     : define the recipe to use when generating the fake data"
    echo "  SIMTEST_CAMERA   : define an alternate camera (otherwise uses entry in sequence file)"
    echo "  SIMTEST_SEQUENCE : define the set of observations generated (simtest.basic.config)"
    echo "  SIMTEST_AUTO     : define the analysis steps to perform (simtest.basic.auto)"
    echo "  SIMTEST_THREADS  : set the number of threads for the processing node (0)"
    echo ""
    echo "the following macros can be used to set up specific simtest suites:"
    echo "  simtest.setup.basic : run standard simtest suite"
    echo "  simtest.setup.detverify : run detrend creation and detrend verification"
    echo "  simtest.setup.flatcorr : run a flat-field correction demonstration"
    echo "  simtest.setup.ctemask : run a ctemask demonstration"
    break
  end

  $dbname = $1
  $hostname = $2
  $init = $3
  if (("$init" != "run") && ("$init" != "inject") && ("$init" != "new"))
    echo "USAGE: simtest (dbname) (hostname) (init)"
    echo " if (init) == run    : just run the analysis tasks"
    echo " if (init) == inject : (re)create the database and (re)inject the images"
    echo " if (init) == new    : restart completely from scratch"
    break
  end    
 
  if (("$init" == "new") || ("$init" == "inject"))  
    # XXX this will fail and exit the script if the db does not exist or is old...
    # XXX this step is too dangerous and was eliminated
    exec pxadmin -delete -dbname $dbname

    # XXX this gives warnings if the db exists...
    # XXX this step is too dangerous and was eliminated
    exec pxadmin -create -dbname $dbname

    if ("$init" == "new")
      # the labels "wait" and "proc" are special names used in automate.pro
    
      # make a reference database
      mkref refcat 10

      $ppsim = "ppSimSequence $SIMTEST_SEQUENCE simtest.mkimages simtest.inject -path $SIMTEST_RAWDIR -workdir $SIMTEST_WORKDIR -dbname $dbname -label wait -dvodb DVO -tess_id TESS -refcat refcat.catdir"
      if ("$PPSIM_RECIPE" != "default") 
         $ppsim = $ppsim -ppsim_recipe $PPSIM_RECIPE
      end
      $ppsim = $ppsim -camera $SIMTEST_CAMERA

      exec $ppsim

      exec source simtest.mkimages

      ## XXX this will need to be optional as well 
      ## we could use the book to find OBSTYPE = OBJECT, get CENTER.RA, CENTER.DEC 
      ## or else we could have ppSimSequence generate an appropriate skycells call
      file TESS/Images.dat found
      if ($found == 0)
        # this command is approriate only to the dither pattern and camera define in simtest.basic.config
        exec skycells -mode local -scale 0.2 -center 270.75 -23.70 -size 0.3 0.3 -nx 3 -ny 3 -D CATDIR TESS
      end
    end

    exec chiptool -dbname $dbname -block -label wait
    exec source simtest.inject
  end

  module pantasks.pro
  module automate.pro

  module.tasks

  add.label proc

  if ($SIMTEST_THREADS == 0) 
    controller host add $hostname
  else
    controller host add $hostname -threads $SIMTEST_THREADS
  end

  add.database $dbname

  automate.load $SIMTEST_AUTO $SIMTEST_CAMERA $dbname

  tasks.revert.off

  run
end

# XXX add a test function to pxadmin to check for db existence

## currently defined options for sequence, etc:

# sequence: simtest.basic.config, simtest.flatcorr.config
# auto: simtest.basic.auto, simtest.flatcorr.auto

macro simtest.setup.basic
  $PPSIM_RECIPE = default
  $SIMTEST_SEQUENCE = simtest.basic.config
  $SIMTEST_AUTO = simtest.basic.auto
end

macro simtest.setup.nebulous.basic
  $PPSIM_RECIPE = default
  $SIMTEST_SEQUENCE = simtest.basic.config
  $SIMTEST_AUTO = simtest.nebulous.basic.auto
  $SIMTEST_USER = `whoami`
  $SIMTEST_RAWDIR = neb://anyhost/simtest.$SIMTEST_USER/raw
  $SIMTEST_WORKDIR = neb://anyhost/simtest.$SIMTEST_USER/work
end

macro simtest.setup.detverify
  $PPSIM_RECIPE = default
  $SIMTEST_SEQUENCE = simtest.detverify.config
  $SIMTEST_AUTO = simtest.detverify.auto
end

macro simtest.setup.ctemask
  $PPSIM_RECIPE = BADCTE.TEST
  $SIMTEST_SEQUENCE = simtest.ctemask.config
  $SIMTEST_AUTO = simtest.ctemask.auto
end

macro simtest.setup.flatcorr
  $PPSIM_RECIPE = FLATCORR
  $SIMTEST_SEQUENCE = simtest.flatcorr.config
  $SIMTEST_AUTO = simtest.flatcorr.auto
end

# basic options for the these images (filter, location, obstype)
$BaseOptions = -type OBJECT -filter r -skymags 20.86 -ra 270.70 -dec -23.70 -pa 0.0
$BaseOptions = $BaseOptions -Df PSASTRO:DVO.GETSTAR.MAX.RHO 50000.0
$BaseOptions = $BaseOptions -nx 2500 -ny 2500

# options for the reference image
$RefOptions = $BaseOptions -exptime 100.0 -seeing 1.0
$RefOptions = $RefOptions -D PSF.MODEL PS_MODEL_GAUSS
$RefOptions = $RefOptions -Df STARS.SIGMA.LIM 0.5
$RefOptions = $RefOptions -Db PSF.CONVOLVE T

# create a reference database of fake stars to be used by ppSim below
macro mkref
  if ($0 != 3)
    echo "mkref (refbase) (density)"
    break
  end

  local refbase
  $refbase = $1

  exec rm -rf $refbase.catdir
  exec rm -f $refbase.fits
  
  $RefOptions = $RefOptions -Df STARS.DENSITY $2

  # create an image with fake sources and insert the resulting cmf file into a dvodb
  $RefConfig = -camera SIMTEST -recipe PPSIM STACKTEST.MAKE -D PSASTRO:PSASTRO.CATDIR $refbase.catdir

  exec ppSim $RefOptions $RefConfig $refbase
  
  file synth.photcodes found
  if (not($found))
    echo "making photcodes file"
    mkphotcodes synth.photcodes
  end

  exec addstar -D CAMERA simtest -D CATDIR $refbase.catdir -accept-astrom -photcode SYNTH.r -D PHOTCODE_FILE synth.photcodes $refbase.cmf -quick-airmass
  exec relphot -averages -D CATDIR $refbase.catdir -update -region 260 280 -33 -13
end

# if we run this test as a stand-alone program somewhere, we may need to create a local copy of the photcode file:
macro mkphotcodes
  if ($0 != 2)
    echo "USAGE: mkphotcodes (filename)"
    break
  end

  exec /bin/rm -f $1
  output $1
  echo "#                                           airmass      color                         astrometry  mag    photom  astrom mask    photom mask"
  echo "# code  name                type    zero  slope offset c1    c2   slope   zero  equiv  sys scale   scale  sys     poor   bad     poor   bad"
  echo "  1     g_SYNTH              sec   0.000  0.000 0.000     1     3 0.0000     0    21   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  2     r_SYNTH              sec   0.000  0.000 0.000     2     3 0.0000     0    22   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3     i_SYNTH              sec   0.000  0.000 0.000     2     3 0.0000     0    23   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  4     z_SYNTH              sec   0.000  0.000 0.000     3     4 0.0000     0    24   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  5     y_SYNTH              sec   0.000  0.000 0.000     4     5 0.0000     0    25   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3001  SYNTH.g              ref   0.000  0.000 0.000     -     - 0.0000     0     1   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3002  SYNTH.r              ref   0.000  0.000 0.000     -     - 0.0000     0     2   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3003  SYNTH.i              ref   0.000  0.000 0.000     -     - 0.0000     0     3   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3004  SYNTH.z              ref   0.000  0.000 0.000     -     - 0.0000     0     4   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3005  SYNTH.y              ref   0.000  0.000 0.000     -     - 0.0000     0     5   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  output stdout
end

