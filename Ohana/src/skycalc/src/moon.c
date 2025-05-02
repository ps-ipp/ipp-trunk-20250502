# include <skycalc_internal.h>

void SC_lpmoon(double jd, double lat, double sid,
       double* ra, double* dec, double* dist) {


/* implements "low precision" moon algorithms from
   Astronomical Almanac (p. D46 in 1992 version).  Does
   apply the topocentric correction. 
   Units are as follows
   jd,lat, sid;   decimal hours 
   *ra, *dec,   decimal hours, degrees 
   *dist;      earth radii */


    double T, lambda, beta, pie, l, m, n, x, y, z, alpha, delta,
        rad_lat, rad_lst, distance, topo_dist;
    char dummy[40];  /* to fix compiler bug on IBM system */

    T = (jd - J2000) / 36525.;  /* jul cent. since J2000.0 */

    lambda = 218.32 + 481267.883 * T 
        + 6.29 * sin((134.9 + 477198.85 * T) / DEG_IN_RADIAN)
        - 1.27 * sin((259.2 - 413335.38 * T) / DEG_IN_RADIAN)
        + 0.66 * sin((235.7 + 890534.23 * T) / DEG_IN_RADIAN)
        + 0.21 * sin((269.9 + 954397.70 * T) / DEG_IN_RADIAN)
        - 0.19 * sin((357.5 + 35999.05 * T) / DEG_IN_RADIAN)
        - 0.11 * sin((186.6 + 966404.05 * T) / DEG_IN_RADIAN);
    lambda = lambda / DEG_IN_RADIAN;
    beta = 5.13 * sin((93.3 + 483202.03 * T) / DEG_IN_RADIAN)
        + 0.28 * sin((228.2 + 960400.87 * T) / DEG_IN_RADIAN)
        - 0.28 * sin((318.3 + 6003.18 * T) / DEG_IN_RADIAN)
        - 0.17 * sin((217.6 - 407332.20 * T) / DEG_IN_RADIAN);
    beta = beta / DEG_IN_RADIAN;
    pie = 0.9508 
        + 0.0518 * cos((134.9 + 477198.85 * T) / DEG_IN_RADIAN)
        + 0.0095 * cos((259.2 - 413335.38 * T) / DEG_IN_RADIAN)
        + 0.0078 * cos((235.7 + 890534.23 * T) / DEG_IN_RADIAN)
        + 0.0028 * cos((269.9 + 954397.70 * T) / DEG_IN_RADIAN);
    pie = pie / DEG_IN_RADIAN;
    distance = 1 / sin(pie);

    l = cos(beta) * cos(lambda);
    m = 0.9175 * cos(beta) * sin(lambda) - 0.3978 * sin(beta);
    n = 0.3978 * cos(beta) * sin(lambda) + 0.9175 * sin(beta);

    x = l * distance; 
    y = m * distance; 
    z = n * distance;  /* for topocentric correction */


    /* lat isn't passed right on some IBM systems unless you do this
       or something like it! */
    sprintf(dummy,"%f",lat);

    rad_lat = lat / DEG_IN_RADIAN;
    rad_lst = sid / HRS_IN_RADIAN;

    x = x - cos(rad_lat) * cos(rad_lst);
    y = y - cos(rad_lat) * sin(rad_lst);
    z = z - sin(rad_lat);


    topo_dist = sqrt(x * x + y * y + z * z);

    l = x / topo_dist; 
    m = y / topo_dist; 
    n = z / topo_dist;

    alpha = SC_atan_circ(l,m);
    delta = asin(n);

    *ra = alpha * HRS_IN_RADIAN;

    *dec = delta * DEG_IN_RADIAN;
    
    *dist = topo_dist;

}

/* More accurate (but more elaborate and slower) lunar
   ephemeris, from Jean Meeus' *Astronomical Formulae For Calculators*,
   pub. Willman-Bell.  Includes all the terms given there. */

void SC_accumoon (jd,geolat,lst,elevsea,geora,geodec,geodist,topora,topodec,topodist)
     double jd,geolat,lst,elevsea;
     double *geora,*geodec,*geodist,*topora,*topodec,*topodist;
{
  double pie, dist;  /* horiz parallax */
  double Lpr,M,Mpr,D,F,Om,T,Tsq,Tcb;
  double e,lambda,B,beta,om1,om2;
  double sinx, x, y, z, l, m, n;
  double x_geo, y_geo, z_geo;  /* geocentric position of *observer* */

  jd = jd + SC_etcorr(jd)/SEC_IN_DAY;   /* approximate correction to ephemeris time */
  T = (jd - 2415020.) / 36525.;   /* this based around 1900 ... */
  Tsq = T * T;
  Tcb = Tsq * T;

  Lpr = 270.434164 + 481267.8831 * T - 0.001133 * Tsq
    + 0.0000019 * Tcb;
  M = 358.475833 + 35999.0498*T - 0.000150*Tsq
    - 0.0000033*Tcb;
  Mpr = 296.104608 + 477198.8491*T + 0.009192*Tsq
    + 0.0000144*Tcb;
  D = 350.737486 + 445267.1142*T - 0.001436 * Tsq
    + 0.0000019*Tcb;
  F = 11.250889 + 483202.0251*T -0.003211 * Tsq
    - 0.0000003*Tcb;
  Om = 259.183275 - 1934.1420*T + 0.002078*Tsq
    + 0.0000022*Tcb;

  Lpr = SC_circulo(Lpr);
  Mpr = SC_circulo(Mpr);
  M = SC_circulo(M);
  D = SC_circulo(D);
  F = SC_circulo(F);
  Om = SC_circulo(Om);


  sinx =  sin((51.2 + 20.2 * T)/DEG_IN_RADIAN);
  Lpr = Lpr + 0.000233 * sinx;
  M = M - 0.001778 * sinx;
  Mpr = Mpr + 0.000817 * sinx;
  D = D + 0.002011 * sinx;

  sinx = 0.003964 * sin((346.560+132.870*T -0.0091731*Tsq)/DEG_IN_RADIAN);

  Lpr = Lpr + sinx;
  Mpr = Mpr + sinx;
  D = D + sinx;
  F = F + sinx;

  sinx = sin(Om/DEG_IN_RADIAN);
  Lpr = Lpr + 0.001964 * sinx;
  Mpr = Mpr + 0.002541 * sinx;
  D = D + 0.001964 * sinx;
  F = F - 0.024691 * sinx;
  F = F - 0.004328 * sin((Om + 275.05 -2.30*T)/DEG_IN_RADIAN);

  e = 1 - 0.002495 * T - 0.00000752 * Tsq;

  M = M / DEG_IN_RADIAN;   /* these will all be arguments ... */
  Mpr = Mpr / DEG_IN_RADIAN;
  D = D / DEG_IN_RADIAN;
  F = F / DEG_IN_RADIAN;

  lambda = Lpr + 6.288750 * sin(Mpr)
    + 1.274018 * sin(2*D - Mpr)
    + 0.658309 * sin(2*D)
    + 0.213616 * sin(2*Mpr)
    - e * 0.185596 * sin(M)
    - 0.114336 * sin(2*F)
    + 0.058793 * sin(2*D - 2*Mpr)
    + e * 0.057212 * sin(2*D - M - Mpr)
    + 0.053320 * sin(2*D + Mpr)
    + e * 0.045874 * sin(2*D - M)
    + e * 0.041024 * sin(Mpr - M)
    - 0.034718 * sin(D)
    - e * 0.030465 * sin(M+Mpr)
    + 0.015326 * sin(2*D - 2*F)
    - 0.012528 * sin(2*F + Mpr)
    - 0.010980 * sin(2*F - Mpr)
    + 0.010674 * sin(4*D - Mpr)
    + 0.010034 * sin(3*Mpr)
    + 0.008548 * sin(4*D - 2*Mpr)
    - e * 0.007910 * sin(M - Mpr + 2*D)
    - e * 0.006783 * sin(2*D + M)
    + 0.005162 * sin(Mpr - D);

		/* And furthermore.....*/

  lambda = lambda + e * 0.005000 * sin(M + D)
    + e * 0.004049 * sin(Mpr - M + 2*D)
    + 0.003996 * sin(2*Mpr + 2*D)
    + 0.003862 * sin(4*D)
    + 0.003665 * sin(2*D - 3*Mpr)
    + e * 0.002695 * sin(2*Mpr - M)
    + 0.002602 * sin(Mpr - 2*F - 2*D)
    + e * 0.002396 * sin(2*D - M - 2*Mpr)
    - 0.002349 * sin(Mpr + D)
    + e * e * 0.002249 * sin(2*D - 2*M)
    - e * 0.002125 * sin(2*Mpr + M)
    - e * e * 0.002079 * sin(2*M)
    + e * e * 0.002059 * sin(2*D - Mpr - 2*M)
    - 0.001773 * sin(Mpr + 2*D - 2*F)
    - 0.001595 * sin(2*F + 2*D)
    + e * 0.001220 * sin(4*D - M - Mpr)
    - 0.001110 * sin(2*Mpr + 2*F)
    + 0.000892 * sin(Mpr - 3*D)
    - e * 0.000811 * sin(M + Mpr + 2*D)
    + e * 0.000761 * sin(4*D - M - 2*Mpr)
    + e * e * 0.000717 * sin(Mpr - 2*M)
    + e * e * 0.000704 * sin(Mpr - 2 * M - 2*D)
    + e * 0.000693 * sin(M - 2*Mpr + 2*D)
    + e * 0.000598 * sin(2*D - M - 2*F)
    + 0.000550 * sin(Mpr + 4*D)
    + 0.000538 * sin(4*Mpr)
    + e * 0.000521 * sin(4*D - M)
    + 0.000486 * sin(2*Mpr - D);

/*              *eclongit = lambda;  */

  B = 5.128189 * sin(F)
    + 0.280606 * sin(Mpr + F)
    + 0.277693 * sin(Mpr - F)
    + 0.173238 * sin(2*D - F)
    + 0.055413 * sin(2*D + F - Mpr)
    + 0.046272 * sin(2*D - F - Mpr)
    + 0.032573 * sin(2*D + F)
    + 0.017198 * sin(2*Mpr + F)
    + 0.009267 * sin(2*D + Mpr - F)
    + 0.008823 * sin(2*Mpr - F)
    + e * 0.008247 * sin(2*D - M - F)
    + 0.004323 * sin(2*D - F - 2*Mpr)
    + 0.004200 * sin(2*D + F + Mpr)
    + e * 0.003372 * sin(F - M - 2*D)
    + 0.002472 * sin(2*D + F - M - Mpr)
    + e * 0.002222 * sin(2*D + F - M)
    + e * 0.002072 * sin(2*D - F - M - Mpr)
    + e * 0.001877 * sin(F - M + Mpr)
    + 0.001828 * sin(4*D - F - Mpr)
    - e * 0.001803 * sin(F + M)
    - 0.001750 * sin(3*F)
    + e * 0.001570 * sin(Mpr - M - F)
    - 0.001487 * sin(F + D)
    - e * 0.001481 * sin(F + M + Mpr)
    + e * 0.001417 * sin(F - M - Mpr)
    + e * 0.001350 * sin(F - M)
    + 0.001330 * sin(F - D)
    + 0.001106 * sin(F + 3*Mpr)
    + 0.001020 * sin(4*D - F)
    + 0.000833 * sin(F + 4*D - Mpr);
  /* not only that, but */
  B = B + 0.000781 * sin(Mpr - 3*F)
    + 0.000670 * sin(F + 4*D - 2*Mpr)
    + 0.000606 * sin(2*D - 3*F)
    + 0.000597 * sin(2*D + 2*Mpr - F)
    + e * 0.000492 * sin(2*D + Mpr - M - F)
    + 0.000450 * sin(2*Mpr - F - 2*D)
    + 0.000439 * sin(3*Mpr - F)
    + 0.000423 * sin(F + 2*D + 2*Mpr)
    + 0.000422 * sin(2*D - F - 3*Mpr)
    - e * 0.000367 * sin(M + F + 2*D - Mpr)
    - e * 0.000353 * sin(M + F + 2*D)
    + 0.000331 * sin(F + 4*D)
    + e * 0.000317 * sin(2*D + F - M + Mpr)
    + e * e * 0.000306 * sin(2*D - 2*M - F)
    - 0.000283 * sin(Mpr + 3*F);

  om1 = 0.0004664 * cos(Om/DEG_IN_RADIAN);
  om2 = 0.0000754 * cos((Om + 275.05 - 2.30*T)/DEG_IN_RADIAN);

  beta = B * (1. - om1 - om2);
  /*      *eclatit = beta; */

  pie = 0.950724
    + 0.051818 * cos(Mpr)
    + 0.009531 * cos(2*D - Mpr)
    + 0.007843 * cos(2*D)
    + 0.002824 * cos(2*Mpr)
    + 0.000857 * cos(2*D + Mpr)
    + e * 0.000533 * cos(2*D - M)
    + e * 0.000401 * cos(2*D - M - Mpr)
    + e * 0.000320 * cos(Mpr - M)
    - 0.000271 * cos(D)
    - e * 0.000264 * cos(M + Mpr)
    - 0.000198 * cos(2*F - Mpr)
    + 0.000173 * cos(3*Mpr)
    + 0.000167 * cos(4*D - Mpr)
    - e * 0.000111 * cos(M)
    + 0.000103 * cos(4*D - 2*Mpr)
    - 0.000084 * cos(2*Mpr - 2*D)
    - e * 0.000083 * cos(2*D + M)
    + 0.000079 * cos(2*D + 2*Mpr)
    + 0.000072 * cos(4*D)
    + e * 0.000064 * cos(2*D - M + Mpr)
    - e * 0.000063 * cos(2*D + M - Mpr)
    + e * 0.000041 * cos(M + D)
    + e * 0.000035 * cos(2*Mpr - M)
    - 0.000033 * cos(3*Mpr - 2*D)
    - 0.000030 * cos(Mpr + D)
    - 0.000029 * cos(2*F - 2*D)
    - e * 0.000029 * cos(2*Mpr + M)
    + e * e * 0.000026 * cos(2*D - 2*M)
    - 0.000023 * cos(2*F - 2*D + Mpr)
    + e * 0.000019 * cos(4*D - M - Mpr);

  beta = beta/DEG_IN_RADIAN;
  lambda = lambda/DEG_IN_RADIAN;
  l = cos(lambda) * cos(beta);
  m = sin(lambda) * cos(beta);
  n = sin(beta);
  SC_eclrot(jd,&l,&m,&n);

  dist = 1/sin((pie)/DEG_IN_RADIAN);
  x = l * dist;
  y = m * dist;
  z = n * dist;

  *geora = SC_atan_circ(l,m) * HRS_IN_RADIAN;
  *geodec = asin(n) * DEG_IN_RADIAN;
  *geodist = dist;

  SC_geocent (lst, geolat, elevsea, &x_geo, &y_geo, &z_geo);

  x = x - x_geo;  /* topocentric correction using elliptical earth fig. */
  y = y - y_geo;
  z = z - z_geo;

  *topodist = sqrt(x*x + y*y + z*z);

  l = x / (*topodist);
  m = y / (*topodist);
  n = z / (*topodist);

  *topora = SC_atan_circ(l,m) * HRS_IN_RADIAN;
  *topodec = asin(n) * DEG_IN_RADIAN;

}

/* returns jd at which moon is at a given
   altitude, given jdguess as a starting point. In current version
   uses high-precision moon -- execution time does not seem to be
   excessive on modern hardware.  If it's a problem on your machine,
   you can replace calls to 'accumoon' with 'lpmoon' and remove
   the 'elevsea' argument. */
double SC_jd_moon_alt (double alt, double jdguess, double lat, double longit, double elevsea) {

  double jdout;
  double deriv, err, del = 0.002;
  double ra,dec,dist,geora,geodec,geodist,sid,ha,alt2,alt3,az;
  short i = 0;

  /* first guess */

  sid=SC_lst(jdguess,longit);
  SC_accumoon(jdguess,lat,sid,elevsea,&geora,&geodec,&geodist,
	   &ra,&dec,&dist);
  ha = SC_lst(jdguess,longit) - ra;
  alt2 = SC_altit(dec,ha,lat,&az);
  jdguess = jdguess + del;
  sid = SC_lst(jdguess,longit);
  SC_accumoon(jdguess,lat,sid,elevsea,&geora,&geodec,&geodist,
	   &ra,&dec,&dist);
  alt3 = SC_altit(dec,(sid - ra),lat,&az);
  err = alt3 - alt;
  deriv = (alt3 - alt2) / del;
  while((fabs(err) > 0.1) && (i < 10)) {
    jdguess = jdguess - err/deriv;
    sid=SC_lst(jdguess,longit);
    SC_accumoon(jdguess,lat,sid,elevsea,&geora,&geodec,&geodist,
	     &ra,&dec,&dist);
    alt3 = SC_altit(dec,(sid - ra),lat,&az);
    err = alt3 - alt;
    i++;
  }
  if(i >= 9) jdguess = -1000.;
  jdout = jdguess;
  return(jdout);
}

/* Given site position, return Moonset for closest midnight */
double SC_moonset_tonight (struct SC_date_time date, double lat, double longit, double elevsea, double elev) {

  double jd, jdmid, stmid;
  double min_alt, max_alt;
  double geora, geodec, geodist;  /* geocent for moon, not used here.*/
  double ramoon, decmoon, distmoon;
  double hamoonset, tmoonset, jdmoonset;
  struct SC_date_time date_midnight;
  double dt, lst0, lst1, djd, horiz;

  horiz = sqrt (2. * elev / 6378140.) * DEG_IN_RADIAN;

  /* find offset in hours from longit to greenwich */
  jd = SC_date_to_jd (date);  /* true jd now */
  lst0 = SC_lst (jd, 0.0);    /* lst at long = 0 */
  lst1 = SC_lst (jd, longit); /* local lst now */
  dt = lst0 - lst1;
  if (dt < 0) dt += 24;
	
  /* midnight at greenwich */
  date_midnight = date;
  date_midnight.h = 0;
  date_midnight.mn = 0;
  date_midnight.s = 0;
	
  /* find jd for local midnight, select the *closest* midnight */
  jdmid = SC_date_to_jd (date_midnight) - dt / 24.0;
  djd = jd - jdmid;
  if (djd < -0.5) jdmid -= 1.0;
  if (djd >  0.5) jdmid += 1.0;
  stmid = SC_lst (jdmid,longit); 

  SC_accumoon (jdmid,lat,stmid,elevsea,&geora,&geodec,&geodist,&ramoon,&decmoon,&distmoon);

  SC_min_max_alt (lat,decmoon,&min_alt,&max_alt);  /* rough check -- occurs? */
  if (max_alt < -(0.83+horiz)) return (-1);
  if (min_alt > -(0.83+horiz)) return (-1);

  /* compute moonrise and set if they're likely to occur */

  hamoonset = SC_ha_alt(decmoon,lat,-(0.83+horiz)); /* rough approx. */

  tmoonset = SC_adj_time(ramoon+hamoonset-stmid);
  jdmoonset = jdmid + tmoonset / 24.;
  jdmoonset = SC_jd_moon_alt(-(0.83+horiz),jdmoonset,lat,longit,elevsea);

  return (jdmoonset);

}

/* Given site position, return Moonrise for closest midnight */
double SC_moonrise_tonight (struct SC_date_time date, double lat, double longit, double elevsea, double elev) {

  double jd, jdmid, stmid;
  double min_alt, max_alt;
  double geora, geodec, geodist;  /* geocent for moon, not used here.*/
  double ramoon, decmoon, distmoon;
  double hamoonset, tmoonrise, jdmoonrise;
  struct SC_date_time date_midnight;
  double dt, lst0, lst1, djd, horiz;

  horiz = sqrt (2. * elev / 6378140.) * DEG_IN_RADIAN;

  /* find offset in hours from longit to greenwich */
  jd = SC_date_to_jd (date);  /* true jd now */
  lst0 = SC_lst (jd, 0.0);    /* lst at long = 0 */
  lst1 = SC_lst (jd, longit); /* local lst now */
  dt = lst0 - lst1;
  if (dt < 0) dt += 24;
	
  /* midnight at greenwich */
  date_midnight = date;
  date_midnight.h = 0;
  date_midnight.mn = 0;
  date_midnight.s = 0;
	
  /* find jd for local midnight, select the *closest* midnight */
  jdmid = SC_date_to_jd (date_midnight) - dt / 24.0;
  djd = jd - jdmid;
  if (djd < -0.5) jdmid -= 1.0;
  if (djd >  0.5) jdmid += 1.0;
  stmid = SC_lst (jdmid,longit); 

  SC_accumoon (jdmid,lat,stmid,elevsea,&geora,&geodec,&geodist,&ramoon,&decmoon,&distmoon);

  SC_min_max_alt (lat,decmoon,&min_alt,&max_alt);  /* rough check -- occurs? */
  if (max_alt < -(0.83+horiz)) return (-1);
  if (min_alt > -(0.83+horiz)) return (-1);

  /* compute moonrise and set if they're likely to occur */

  hamoonset = SC_ha_alt(decmoon,lat,-(0.83+horiz)); /* rough approx. */

  tmoonrise = SC_adj_time(ramoon-hamoonset-stmid);
  jdmoonrise = jdmid + tmoonrise / 24.;
  jdmoonrise = SC_jd_moon_alt(-(0.83+horiz),jdmoonrise,lat,longit,elevsea);

  return (jdmoonrise);

}
