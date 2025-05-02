;+
;
; PLX_FACTOR
;
; Given a Julian date (scalar or vector), this returns the parallax
; factor for a set of equatorial coordinates (RA,Dec).  User must have
; an appropriate Earth ephemeris savefile in the path specified at the
; top of the procedure.  The savefile should return the variables:
;
;  jd, x, y, z
;
; that specify the position of the Earth in the coordinate system of
; solar system's barycenter.  For my savefile, I computed this from
; the JPL ephemeris DE405.
;
; INPUT:
;
;  epoch (JD) -- julian date(s) for which to compute the parallax
;                factor
;
;  ra (deg) -- "catalog" RA of object (unperturbed by parallax,
;              aberration, etc.)
;
;  de (deg) -- "catalog" Dec of object (unperturbed by parallax,
;              aberration, etc.)
;
;  NOTE: Epoch input can be an array, with RA and Dec input as
;  scalars.  However, if RA and Dec are input as arrays (i.e., the
;  parallax factor is to be computed for multiple objects), then they
;  must have the same dimensions as the epoch array.
;
; OUTPUT:
;
;  The output produced by this program are two arrays of parallax
;  factors (f_ra, f_de) such that:
;
;    RA  = RA_0 + f_ra * parallax
;    Dec = Dec_0 + f_de * parallax
;
;  Note that f_ra and f_de have dimensionless units.  Also note that
;  the RA correction factor includes a factor of cos(Dec) such that:
;   f_ra * parallax = delta(RA) * cos(Dec)
;
;              Written by Trent J. Dupuy -- 2010 Jul 14
;
; REVISIONS
; ---------
;  2011 Jan 25: Added /SPITZER keyword that uses the Spitzer ephemeris
;    instead of the Earth ephemeris.
;-
pro plx_factor, epoch, ra_in, de_in, f_ra, f_de, spitzer=spitzer

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;; SPECIFY PATH FOR EPHEMERIS SAVEFILE HERE ;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
savefile = '/Users/tdupuy/data/jpl/'+$
           'JPL_DE405_EARTH-SOLARBARYCENTER.sav'
if keyword_set(spitzer) then begin
   savefile = '/Users/tdupuy/data/jpl/'+$
              'JPL_SPITZER-SOLARBARYCENTER.sav'
   message,'Using spitzer ephemeris.',/continue
endif
restore, savefile
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; check inputs
;
if n_params() lt 5 then begin
   print, 'PLX_FACTOR, epoch (JD), ra (deg), dec (deg), f_ra, f_de'
   return
endif
if total(size(ra_in,/dim) ne size(de_in,/dim)) ne 0 then $
   message, 'input RA and Dec dimensions must match each other'
if (size(ra_in,/dim))[0] ne 0 and $
   total(size(ra_in,/dim) ne size(epoch,/dim)) ne 0 then $
      message, '(RA, Dec) not specified as scalars, so their '+$
               'dimensions must match the input epoch array'

;
; convert to radians and compute trig functions
;
radeg = 180d/!dpi
ra = ra_in/radeg
de = de_in/radeg
sina = sin(ra)
cosa = cos(ra)
sind = sin(de)
cosd = cos(de)

;
; interpolate the Earth ephemeris at the input epochs
;
x_in = interpol( x, jd, double(epoch) )
y_in = interpol( y, jd, double(epoch) )
z_in = interpol( z, jd, double(epoch) )
if min(epoch) lt min(jd) or max(epoch) gt max(jd) then $
   message, /continue, 'inputs epochs extend beyond the range '+$
            'of the ephemeris being used.'

;
; could also apply an offset here due to the position of the observer
; on the earth being slightly different than the geocenter.  as
; described in Sec 6.2.3 of Kovalesky & Seidelmann this effect is
; about 4 uas for a parallax of 0.1" -- i.e., negligible
;

;
; compute parallax factor (from pg. B28 of Astronomical Almanac 2010,
; also see Sec 6.2.1 of Kovalesky & Seidelmann's Fundamentals
; of Astrometry for a derivation)
;
f_ra = x_in*sina - y_in*cosa
f_de = x_in*cosa*sind + y_in*sina*sind - z_in*cosd

end
