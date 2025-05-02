#!/usr/bin/perl -w
package phase2;

use strict;
use pslib;
#
# Prototypes
#
sub DefineStats($);
sub DirectionFromOverscan($);
sub bias_subtract_readout($$$; @);
sub dark_subtract_readout($$$);
sub defringe_readout($$$$);
sub getScalingFactor($$$);
sub load_image($$$$);
sub make_kernel();
sub make_mask($$);
sub mask_readout($$$$$);
sub nonlinearity_correct_readout($$$$);
sub phase2($$; @);
sub pmCameraFromHeader($);
sub pmCameraValidateHeaders($$);
sub pmFPAfromHeader($$);
sub pmFlatField($$$);
sub pmRecipeDefineSection($$$);
sub pmSectiontoRectangle($);
sub pmSubtractBias($$$$$$$$);
sub psFITSReadHeaderSet($);
sub trim_readout($$$);

###############################################################################
#
# Do the real work
#
sub phase2($$; @)
{
   my($input, $outroot, $cameraname, $recipefile, $basefile, $verbose) = @_;
   $basefile ||= "base.config"; $verbose ||= 0;
   my($nFail) = 0;
   $outroot++;

   pslib::psTraceSetLevel("ps.phase2", $verbose);

   my($site);
   if ($basefile) {
      pslib::psTrace("ps.phase2", 2, "Basefile: $basefile\n");
      
      my($base) = pslib::psMetadataAlloc();
      pslib::psMetadataParseConfig($base, $basefile, 1);
      
      my($sitefile) = pslib::psMetadataLookup($base, "SITE")->get_STR();
      
      $site = pslib::psMetadataAlloc();
      pslib::psMetadataParseConfig($site, $sitefile, 1);
   } else {
      $site = undef;
   }
   
   # load only the headers from the given file input a list of headers
   my($headers) = psFITSReadHeaderSet($input);
   
   # identify camera (may be specified on command line)
   my($camera);
   if (!$cameraname) {
      $cameraname = pmCameraFromHeader(pslib::psHashLookup($headers, "PHU"));
      my($camerafile) = pslib::psMetadataLookup($site, ("CAMERA.$cameraname"))->get_STR();

      # load the camera details for the specific camera
      $camera = pslib::psMetadataAlloc();
      pslib::psMetadataParseConfig($camera, $camerafile, 0);
   }

   # recipefile is defined in the camera metadata, if not specified on command line
   if (!$recipefile) {
      if (1) {			# 
	 $recipefile = pslib::psMetadataLookup($camera, "RECIPE.PHASE2")->get_STR();
      } else {                           # an alternative make this a database lookup & a function of date
	 $recipefile = DetrendDatabaseLookup ("recipe", $camera, $headers);
      }
   }
   
   pslib::psTrace("ps.phase2", 2, "Reading recipe from $recipefile\n");
   my($recipe) = pslib::psMetadataAlloc();
   pslib::psMetadataParseConfig($recipe, $recipefile, 0);

   # END CONFIG loading code

   # check the validity of the given set of headers based on expectactions for this camera
   pslib::psTrace("ps.phase2", 3, "Validating headers\n");
   pmCameraValidateHeaders($headers, $camera);

   # construct a complete, empty (ie, no pixel data) FPA structure for this camera / header set
   # we need to construct the output container.  is it big enough to fit in memory?  assume so?
   my($FPA) = pmFPAfromHeader($headers, $camera);
   
   # I\'m pretending the extname value for each cell in part of psCell
   # Do we need to construct a list of the extname values associated with each readout?
   # Is EXTNAME invarient?  (probably not; does not always exist)
   #
   # XXX Faked psCell->{extname} to exist and be NULL in pslib.i XXX
   
   my($i, $j, $k);
   for ($i = 0; $i < $FPA->{chips}->{n}; $i++) {
      my($chip) = $FPA->{chips}->get_chip($i);

      pslib::psTrace("ps.phase2", 3, "Processing chip $i\n");
      for ($j = 0; $j < $chip->{cells}->{n}; $j++) {
	 my($cell) = $chip->{cells}->get_cell($j);
	 
	 pslib::psTrace("ps.phase2", 4, "Processing cell $j\n");
	 my($bias) = load_image("BIAS", $recipe, $cell->{metadata}, $cell->{extname});
	 my($dark) = load_image("DARK", $recipe, $cell->{metadata}, $cell->{extname});
	 my($flat) = load_image("FLAT", $recipe, $cell->{metadata}, $cell->{extname});
	 my($badpix) = load_image("BADPIX", $recipe, $cell->{metadata}, $cell->{extname});
	 my($fringe) = load_image("FRINGE", $recipe, $cell->{metadata}, $cell->{extname});
	 pslib::psTrace("ps.phase2", 5, "Read images for cell $j\n");
	 
	 for ($k = 0; $k < $cell->{readouts}->{n}; $k++) {
	    pslib::psTrace("ps.phase2", 6, "Reading readout $k\n");
	    
	    my($readout) = $cell->{readouts}->get_readout($k);
	    $readout->{image} = pslib::psImageReadSection(undef, 0, 0, 0, 0, 0, $cell->{extname}, $k, $input);

	    if (!defined($readout->{metadata})) {
	       pslib::psTrace("ps.phase2", 7, "Creating metadata\n");
	       $readout->{metadata} = pslib::psMetadataAlloc();

	       my($nbias) = 2;
	       pslib::psMetadataAddStr($readout->{metadata}, $pslib::PS_LIST_TAIL,
				       "BIASSEC", "Define bias section",
				       sprintf("[0:%d,0:%d]",
					       ($readout->{image}->{numRows} - 1, $nbias - 1)));
	       pslib::psMetadataAddStr($readout->{metadata}, $pslib::PS_LIST_TAIL,
				       "DATASEC", "Define datasection section",
				       sprintf("[0:%d,%d:%d]",
					       ($readout->{image}->{numRows} - 1, $nbias,
						$readout->{image}->{numCols} - 1)));
	    }
	    
	    my($kernel) = make_kernel();
	    my($mask) = make_mask($readout->{image}->{numRows}, $readout->{image}->{numCols});
	    
	    pslib::psTrace("ps.phase2", 7, "Bias subtraction\n");
	    bias_subtract_readout($readout, $bias, $recipe);
	    
	    pslib::psTrace("ps.phase2", 7, "Dark subtraction\n");
	    dark_subtract_readout($readout, $dark, $recipe);
	    nonlinearity_correct_readout($readout, $mask, $recipe, $camera);
	    # 
	    pslib::psTrace("ps.phase2", 7, "Trimming\n");
	    trim_readout($readout, $mask, $recipe);
	    mask_readout($readout, $mask, $badpix, $kernel, $recipe);
	    # 
	    # where do we convolve the flat with the kernel?
	    #
	    pmFlatField($readout, $mask, $flat);
	    # 
	    pslib::psTrace("ps.phase2", 7, "Defringing\n");
	    defringe_readout($readout, $mask, $fringe, $recipe);

	 }
      }
   }
}

sub bias_subtract_readout($$$; @)
{
   #"bias subtract the given readout according to the recipe (bias image may be None)
   #no mask needed yet"
   
   my($readout, $bias, $recipe, $mask) = (undef, undef, undef, undef);
   ($readout, $bias, $recipe, $mask) = @_;
   #determine overscan option
   my($biassec) = pmRecipeDefineSection($recipe, $readout->{metadata}, "BIAS.OVERSCAN");
   my($overscan) = pslib::psImageSubsection($readout->{image}, $biassec);

   # the type of statistic is defined by the recipe: BIAS.OVERSCAN.STATS
   my($statname) = pslib::psMetadataLookup ($recipe, "BIAS.OVERSCAN.STATS")->get_STR();
   my($stats) = DefineStats($statname);
   
   # identify the type of fit requested and relevant parameters
   # the type of fit is defined by the recipe: BIAS.OVERSCAN.STATS
   my($fitname) = pslib::psMetadataLookup($recipe, "BIAS.OVERSCAN.FIT")->get_STR();  # string defining overscan stats
   
   my($fitType, $fitSpec, $nBin, $nPts, $order);
   if ($fitname eq "NONE") {
      $fitType = "PM_FIT_NONE";
      $fitSpec = undef;
      $nBin = 0;
   } elsif ($fitname eq "SPLINE") {
      my($Npix) = "XXX";

      $fitType = "PM_FIT_SPLINE";
      $nBin = pslib::psMetadataLookup($recipe, "BIAS.OVERSCAN.FIT.NBIN");
      $nPts = pslib::psMetadataLookup($recipe, "BIAS.OVERSCAN.FIT.NPTS");
      $order = pslib::psMetadataLookup($recipe, "BIAS.OVERSCAN.FIT.ORDER");
      $fitSpec = psSpline1DAlloc($nPts, 3, 0, $Npix);                  # Npix is either NAXIS1 or NAXIS2 (Nx or Ny)
   } elsif ($fitname eq "POLYNOMIAL") {
      $fitType = "PM_FIT_POLYNOMIAL";
      $nBin = pslib::psMetadataLookup($recipe, "BIAS.OVERSCAN.FIT.NBIN");
      $nPts = pslib::psMetadataLookup($recipe, "BIAS.OVERSCAN.FIT.NPTS");
      $order = pslib::psMetadataLookup($recipe, "BIAS.OVERSCAN.FIT.ORDER");
      $fitSpec = psPolynomial1DAlloc($nPts, "PS_POLYNOMIAL_CHEB");
   } else {
      #raise pslib::PsUtilsError ("unknown fitname: %s" % fitname)
   }
   
   my($overscanlist) = pslib::psListAlloc($overscan);
   $readout->{image} = pmSubtractBias($readout->{image}, $fitSpec, $overscanlist,
				      DirectionFromOverscan($biassec), $stats, $nBin, $fitType, $bias);

   # add these to the readout->{metadata}? 
   # or to the chip / cell metadata?
}

sub dark_subtract_readout($$$)
{
   #"subtract dark image using the appropriate scaling factor for this exposure time
   #no mask needed yet"

   my($readout, $dark, $recipe) = @_;

   if (!defined($dark)) {
      return;
   }

   # need to get a lookup table from the database, apply exptime for readout and dark
   my($factor) = getScalingFactor($recipe, $readout, $dark);
   
   # $out = in - dark * factor
   my($tmp) = pslib::psImageCopy(undef, $dark, $dark->{type}->{type});
   pslib::psBinaryOp($tmp, $dark, "*", $factor);
   pslib::psBinaryOp($readout->{image}, $readout->{image}, "-", $tmp);
}

sub nonlinearity_correct_readout($$$$)
{
   #"this corrects the non-linear response, but probably should use data determined at the cell level
   #we need to set a mask value for pixels which have were out of range (TBD)"

   my($readout, $mask, $recipe, $camera) = @_;
   
   my($method) = pslib::psMetadataLookup($recipe, "NONLINEAR.METHOD")->get_STR();
   
   # use a polynomial; get coeffs from recipe
   if ($method eq "NONE") {
      ;
   } elsif ($method eq "POLYNOMIAL") {
      my($correction);
      $correction->{n} = pslib::psMetadataLookup($recipe, "NONLINEAR.ORDER")->{data}->{S32};
      $correction->{coeffs} = pslib::psMetadataLookup($recipe, "NONLINEAR.COEFFS");
      pslib::pmNonLinearityPolynomial($readout, $correction);
   } elsif ($method eq "LOOKUP") {
      my($time) = "XXX";
      my($lookup_table) = DetrendDatabaseLookup($time, $camera, "etc?");
      my($inFlux) = pslib::psFITSTableRead($lookup_table, "NONLINEAR.IN_FLUX");
      my($outFlux) = pslib::psFITSTableRead($lookup_table, "NONLINEAR.OUT_FLUX");
      pslib::pmNonLinearityLookup($readout, $inFlux, $outFlux);
   } else {
      #raise pslib::PsUtilsError ("unknown method: %s" % method);
   }
}

sub trim_readout($$$)
{
   #"trim the image and mask based on the requested region"
   my($readout, $mask, $recipe) = @_;
   my($datasec) = pmRecipeDefineSection($recipe, $readout->{metadata}, "TRIM.DATASEC");
   
   my($r0, $r1, $c0, $c1) = ($datasec =~ /\[([0-9]+):([0-9]+),([0-9]+):([0-9]+)\]/);
   
   pslib::psImageTrim($readout->{image}, $c0, $r0, $c1, $r1);
   $mask = pslib::psImageTrim($mask, $c0, $r0, $c1, $r1);
}

# apply the bad pixel mask to the image mask
sub mask_readout($$$$$)
{
   my($readout, $mask, $badpix, $kernel, $recipe) = @_;
}

sub load_image($$$$)
{
   #"load in the complete corresponding image of proper type (this should be done once per cell)
   #we are implying that the bias image is guaranteed to have the corresponding EXTNAME"

   my($type, $recipe, $header, $extname) = @_;
   my($source) = pslib::psMetadataLookup($recipe, $type . ".IMAGE")->get_STR();

   my($name);
   if (uc(substr($source, 0, 5)) eq "FILE:") {
      $name = substr($source, 5);
   } elsif (uc($source) eq "DB:BEST") {
      $name = DetrendDatabaseLookup("best", $header);   # these APIs need to be seriously fleshed out!!
   } elsif (uc($source) eq "DB:CLOSE") {
      $name = DetrendDatabaseLookup("close", $header);   # these APIs need to be seriously fleshed out!!
   } else {
      #raise PsUtilsError ("Unknown source for $type: $source");
   }

   my($image);
   if (defined($name)) {
      $image = pslib::psImageReadSection (undef, 0, 0, 0, 0, 0, $extname, 0, $name);
   } else {
      $image = undef;
   }

   return $image;
}

sub pmRecipeDefineSection($$$)
{
   my($recipe, $header, $name) = @_;

   my($source) = pslib::psMetadataLookup($recipe, $name)->get_STR();
   
   pslib::psTrace("ps.phase2.recipe", 10, "pmRecipeDefineSection (recipe, header, $name)\n");
   
   if (uc($source) eq "NONE") {
      return psRectangleAlloc(0, 0, 0, 0); # the None section
   } elsif (uc(substr($source, 0, 7)) eq "HEADER:") {
      my($keyword) = substr($source,7);
      my($section) = pslib::psMetadataLookup($header, $keyword)->get_STR();
      return pmSectiontoRectangle($section);
   } elsif (uc(substr($source, 0, 7)) eq "RECIPE:") {
      my($section) = substr($source, 7);
      return pmSectiontoRectangle($section);
   }
   
   #raise pslib::PsUtilsError ("invalid section: $section");
}

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

sub DirectionFromOverscan($)
{
   #"The direction is implied by noting the long dimension of the biassec"
   my($secstr) = @_;

   return 1;
}

sub pmSectiontoRectangle($)
{
   #"Parse a specification of the form [col0:col1,row0:row1]
   #For now, simply return it"
   my($secstr) = @_;

   return $secstr;
}

sub psFITSReadHeaderSet($)
{
   my($input) = @_;
   
   pslib::psTrace("ps.phase2", 2, "Reading headers: $input\n");

   my($meta) = pslib::psMetadataAlloc();
   pslib::psMetadataAddStr($meta, $pslib::PS_LIST_TAIL, "CAMERA", "Camera ID", "0");

   my($headers) = pslib::psHashAlloc(64);
   pslib::psHashAdd($headers, "PHU", $meta);
   
   return $headers;
}

sub pmCameraFromHeader($)
{
   #"Return a camera given the metadata read from a fits header"
   my($meta) = @_;

   return "0";
}

sub pmCameraValidateHeaders($$)
{
   my($headers, $camera) = @_;

   if (0) {
      #raise pslib::PsUtilsError ("Failed to validate $headers:$camera");
   }
}

sub pmFPAfromHeader($$)
{
   my($headers, $camera) = @_;
   
   my($ra) = 1; my($dec) = 2; my($hourAngle) = 3; my($zenith) = 4; my($azimuth) = 5;
   my($rotAngle) = 8;
   my($temperature) = 9; my($pressure) = 10; my($humidity) = 11;
   my($exposureTime) = 12;

   my($nchips) = pslib::psMetadataLookup($camera, "NCHIPS")->{data}->{S32};
   my($ncells) = pslib::psMetadataLookup($camera, "NCELLS")->{data}->{S32};
   
   my($obs) = pslib::psObservatoryAlloc("Haleakala", 21, 150, 3000, 0.1);
   my($wavelength) = 5500e-10;
   my($localTime) = pslib::psTimeGetTime($pslib::PS_TIME_TAI);

   my($exposure) = pslib::psExposureAlloc($ra, $dec, $hourAngle, $zenith, $azimuth, $localTime,
					  $rotAngle, $temperature, $pressure, $humidity, $exposureTime,
					  $wavelength, $obs);
   my($FPA) = pslib::psFPAAlloc($nchips, $exposure);
   my($chips) = pslibc::psFPA_chips_get($FPA);
   
   my($i, $j, $k);
   for ($i = 0; $i <$nchips; $i++) {
      my($chip) = pslib::psChipAlloc($ncells, $FPA);
      pslib::psArraySet($chips, $i, $chip);
      my($cells) = pslibc::psChip_cells_get($chip);
      for ($j = 0; $j <$ncells; $j++) {
	 my($nReadouts) = pslib::psMetadataLookup($camera, "NREADOUTS.$i.$j")->{data}->{S32};

	 my($cell) = pslib::psCellAlloc($nReadouts, $chip);
	 
	 pslib::psArraySet($cells, $j, $cell);
	 
	 my($readouts) = pslibc::psCell_readouts_get($cell);
	 for ($k = 0; $k < $nReadouts; $k++) {
	     pslib::psArraySet($readouts, $k, pslib::psReadoutAlloc());
	 }
      }
   }

   return $FPA;
}

sub make_kernel()
{
   my($nrow) = 5; my($ncol) = 5;
   return pslib::psImageAlloc($ncol, $nrow, $pslib::PS_TYPE_F64);
}

sub make_mask($$)
{
   my($nrow, $ncol) = @_;

   return pslib::psImageAlloc($ncol, $nrow, $pslib::PS_TYPE_F64);
}

sub DefineStats($)
{
   my($str) = @_;
   return $str;
}

sub pmSubtractBias($$$$$$$$)
{
   #"Subtract the bias. Don't do a very good job..."
   my($rawImage, $fitSpec, $overscanlist, $direction, $stats, $nBin, $type, $bias) = @_;
   return pslib::psImageAlloc($rawImage->{numCols}, $rawImage->{numRows}, $pslib::PS_TYPE_F64);
}

sub getScalingFactor($$$)
{
   my($recipe, $readout, $dark) = @_;
   my($source) = pslib::psMetadataLookup($recipe, "DARK.FACTOR")->get_STR();

   if (uc(substr($source, 0, length("VALUE:"))) eq "VALUE:") {
      return pslib::psScalarF32Alloc(substr($source, length("VALUE:")), $pslib::PS_TYPE_F64);
   }

   #raise pslib::PsUtilsError ("Unknown recipe for dark factor: $source");
}

sub pmFlatField($$$)
{
   my($readout, $mask, $flat) = @_;
}

sub defringe_readout($$$$)
{
   my($readout, $mask, $fringe, $recipe) = @_;
}

#(input, outroot, cameraname = None, recipefile = None, basefile = "base.config", verbose=0)
if (1) {
   phase2('data/data.fits', undef, undef, undef, "data/base.config", 4);
   
   if (pslib::psMemCheckLeaksToStderr(0,undef,0)) {
      die "You leaked memory\n";
   }
}

