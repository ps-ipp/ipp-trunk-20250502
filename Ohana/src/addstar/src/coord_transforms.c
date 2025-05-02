# include "addstar.h"
# include "loadstarpar.h"

// values used by libdvo/coord_systems (from Liu et al 2011, A&A 526, A16):
// Liu etal and Reid etal disagree at the 0.3 arcsec level
// transform->phi =  62.8717488056*RAD_DEG;
// transform->Xo  =  32.9319185700*RAD_DEG;
// transform->xo  = 282.8594812080;

int galactic_to_celestial (double *R, double *D, double l, double b) {

  fprintf (stderr, "define me...");
  abort();
}

int celestial_to_galactic (double *l, double *b, double R, double D) {

_angp = np.radians(192.859508333) #  12h 51m 26.282s (J2000)
_dngp = np.radians(27.128336111)  # +27d 07' 42.01" (J2000) 
_l0   = np.radians(32.932)
_ce   = np.cos(_dngp)
_se   = np.sin(_dngp)



}




/*** python version from LSD (Mario Juric)

# Appendix of Reid et al. (http://adsabs.harvard.edu/cgi-bin/bib_query?2004ApJ...616..872R)
# This convention is also used by LAMBDA/WMAP (http://lambda.gsfc.nasa.gov/toolbox/tb_coordconv.cfm)
_angp = np.radians(192.859508333) #  12h 51m 26.282s (J2000)
_dngp = np.radians(27.128336111)  # +27d 07' 42.01" (J2000) 
_l0   = np.radians(32.932)
_ce   = np.cos(_dngp)
_se   = np.sin(_dngp)

def equgal(ra, dec):
	ra = np.radians(ra)
	dec = np.radians(dec)

	cd, sd = np.cos(dec), np.sin(dec)
	ca, sa = np.cos(ra - _angp), np.sin(ra - _angp)

	sb = cd*_ce*ca + sd*_se
	l = np.arctan2(sd - sb*_se, cd*sa*_ce) + _l0
	b = np.arcsin(sb)

	l = np.where(l < 0, l + 2.*np.pi, l)

	l = np.degrees(l)
	b = np.degrees(b)

	return NamedList(('l', l), ('b', b))

def galequ(l, b):
	l = np.radians(l)
	b = np.radians(b)

	cb, sb = np.cos(b), np.sin(b)
	cl, sl = np.cos(l-_l0), np.sin(l-_l0)

	ra  = np.arctan2(cb*cl, sb*_ce-cb*_se*sl) + _angp
	dec = np.arcsin(cb*_ce*sl + sb*_se)

	ra = np.where(ra < 0, ra + 2.*np.pi, ra)

	ra = np.degrees(ra)
	dec = np.degrees(dec)

	return NamedList(('ra', ra), ('dec', dec))

 ***/
