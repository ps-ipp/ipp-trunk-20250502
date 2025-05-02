# include "lightcurve.h"

void
help (name)
char name[];
{
  
  fprintf (stderr, "%s%s%s%s%s%s%s%s%s%s%s%s%s",
	   "USAGE: ", name, " (-P / -A) radius  \n",
	   "\n",
           "  Mandatory Flags:\n",
           "  -P / -A      one of these must be used: \n",
           "    -P  pixs     radius of search in pixels -- uses relative astrometry (rastro)\n",
           "    -A  asec     radius of search in arcsec -- uses absolute astrometry  (astro)\n",
           "  -im images   file with the list of images to use\n",
           "\n",
           "  Optional Flags:\n",
           "  -o  file  output file for star data\n",
           "  -midas    use MIDAS format time info from headers\n"
           "  -h        print this list\n\n",
	   "  ", name, " expects a series of (Ra,Dec) or (X,Y) pairs\n\n");
  
  exit (0);
  
}
