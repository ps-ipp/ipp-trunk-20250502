
# global constants:
$iFkap    = 0.001 / 4.74047; # (km/sec/kpc) / (arcsec/ year)

## FEAST-HIP
if (1)
  $A_oort = +14.82; # km/sec/kpc
  $B_oort = -12.37; # km/sec/kpc
  $U_sol  =   9.32; # km/sec
  $V_sol  =  11.18; # km/sec
  $W_sol  =   7.61; # km/sec
end

## ROESSER:
if (0)
  $A_oort = +14.50; # km/sec/kpc
  $B_oort = -13.00; # km/sec/kpc
  $U_sol  =   9.44; # km/sec
  $V_sol  =  11.90; # km/sec
  $W_sol  =   7.20; # km/sec
end

# galactic to celestial
$trans_phi = -62.8717488056;
$trans_Xo  = 282.8594812080;
$trans_xo  =  32.9319185700;

$trans_sin_phi_cos_Xo = dsin($trans_phi)*dcos($trans_Xo);
$trans_sin_phi_sin_Xo = dsin($trans_phi)*dsin($trans_Xo);
$trans_cos_phi        = dcos($trans_phi);

$trans_cos_phi_cos_Xo = dcos($trans_phi)*dcos($trans_Xo);
$trans_cos_phi_sin_Xo = dcos($trans_phi)*dsin($trans_Xo);
$trans_sin_phi        = dsin($trans_phi);

$trans_cos_Xo 	      = dcos($trans_Xo);
$trans_sin_Xo 	      = dsin($trans_Xo);

$disk_radius = 10; # 10 kpc radius disk
$disk_scale = 0.5; # 200 pc scale height

$halo_scale = 2.0; # 2000 pc scale height

# generate a collection of stars in R,D [L,B] with a range of distances
# generate the predicted proper motions

macro mkstars_halo
  if ($0 != 4)
    echo "USAGE: mkstars_disk (Nstars) (Dm_err) (Nsample)"
    break
  end

  local Nstars dDm_o Nsample

  $Nstars = $1
  $dDm_o = $2
  $Nsample = $3

  # halo stars are uniformly sampled in a gaussian halo
  gaussdev Dhalo $Nstars 0.0 $halo_scale
  set Dm = 5*log(Dhalo * 100)
  
  create seq 0 $Nstars
  set phi   = 2*rnd(seq) - 1

  set L = 360*rnd(seq)
  set B = dasin(phi)
end

macro mkstars_disk
  if ($0 != 5)
    echo "USAGE: mkstars_disk (Nstars) (Dm_err) (Nsample) (minDm)"
    break
  end

  local Nstars dDm_o Nsample minDm

  $Nstars = $1
  $dDm_o = $2
  $Nsample = $3
  $minDm = $4

  gaussdev Zgal $Nstars 0.0 $disk_scale
  set rgal = $disk_radius * rnd(Zgal)

  set B = atan2(Zgal , rgal)
  set L = 360*rnd(Zgal)

  set d_kpc_o = sqrt(rgal^2 + Zgal^2)

  set Dm = 5*log(d_kpc_o * 100)

  subset Bs  = B  if (Dm > $minDm)
  subset Ls  = L  if (Dm > $minDm)
  subset Dms = Dm if (Dm > $minDm)

  mkmotion_dmerr Ls Bs Dms $dDm_o $Nsample
end

macro mkstars_uniform
  if ($0 != 5)
    echo "USAGE: mkstars_uniform (Nstars) (Rmax) (Dm_err) (Nsample)"
    break
  end

  $Nstars = $1
  $Rmax = $2
  $dDm_o = $3

  create seq 0 $Nstars
  set phi   = 2*rnd(seq) - 1

  # uniform density
  # R ~ R_max^3/2
  set d_kpc_o = $Rmax*rnd(seq)^(1/3)
  set Dm = 5*log(d_kpc_o * 100)

  set L = 360*rnd(seq)
  set B = dasin(phi)

  mkmotion_dmerr L B Dm $3 $4
end

macro mkstars_single_dm
  if ($0 != 5)
    echo "USAGE: mkstars (Nstars) (Dm) (Dm_err) (Nsample)"
    break
  end

  create seq 0 $1
  set phi   = 2*rnd(seq) - 1

  set d_kpc = ten(0.2*$2 - 2) + zero(seq)
  set Dm = 5*log(d_kpc * 100)

  set L = 360*rnd(seq)
  set B = dasin(phi)

  mkmotion_dmerr L B Dm $3 $4

  # C1, C2 are from http://arxiv.org/pdf/1306.2945v2.pdf
  set C1 =  dcos(D)*$trans_cos_phi + dsin(D)*(dcos(R)*$trans_sin_phi_sin_Xo - dsin(R)*$trans_sin_phi_cos_Xo);
  set C2 =                                -1*(dcos(R)*$trans_sin_phi_cos_Xo + dsin(R)*$trans_sin_phi_sin_Xo);

  set cosBinv = 1.0 / sqrt(C1*C1 + C2*C2);

  set uR = cosBinv * (C1 * uL_o - C2 * uB_o);
  set uD = cosBinv * (C1 * uB_o + C2 * uL_o);
end

# given L,B,Dm and a Dm_err value, find duL, duB
macro mkmotion_dmerr
  if ($0 != 6)
    echo "USAGE: mkmotion (L) (B) (Dm) (Dm_err) (Nsample)"
    break
  end

  local i Dm_err Nstars 

  set  _L = $1
  set  _B = $2
  set _Dm = $3
  $Dm_err = $4
  $Nsample = $5

  $Nstars = _L[]

  set d_kpc_o = ten(0.2*_Dm - 2)

  set R = _L
  set D = _B
  csystem G C R D

  set uL_gal =     ($A_oort * dcos(2.0*_L) + $B_oort) * dcos(_B*1.0) * $iFkap;
  set uB_gal = -0.5*$A_oort * dsin(2.0*_L) *            dsin(_B*2.0) * $iFkap;

  set uL_sol =  ($U_sol * dsin(_L) - $V_sol * dcos(_L))                             * $iFkap / d_kpc_o;
  set uB_sol = (($U_sol * dcos(_L) + $V_sol * dsin(_L))*dsin(_B) - $W_sol*dcos(_B)) * $iFkap / d_kpc_o;

  set uL_o = uL_gal + uL_sol
  set uB_o = uB_gal + uB_sol

  set uL = uL_o
  set uB = uB_o

  set uL_sol_o = uL_sol
  set uB_sol_o = uB_sol

  # create a buffer to store the results : results for each star are in the y-dir, 
  mcreate resultsL $Nstars $Nsample
  mcreate resultsB $Nstars $Nsample

  # now MC a number of distances
  for i 0 $Nsample
    gaussdev dDm $Nstars 0.0 $Dm_err
    set Dm_x = _Dm + dDm
    set d_kpc = ten(0.2*Dm_x - 2)
    
    set uL_sol =  ($U_sol * dsin(_L) - $V_sol * dcos(_L))                             * $iFkap / d_kpc;
    set uB_sol = (($U_sol * dcos(_L) + $V_sol * dsin(_L))*dsin(_B) - $W_sol*dcos(_B)) * $iFkap / d_kpc;

    set duL = uL_sol - uL_sol_o
    set duB = uB_sol - uB_sol_o

    mset resultsL duL -x $i    
    mset resultsB duB -x $i    
  end
    
  set duL = zero(uL_o)
  set duB = zero(uB_o)
  set uLoff = zero(uL_o)
  set uBoff = zero(uB_o)

  for i 0 $Nstars
    mget resultsL tmp -y $i
    vstat -q tmp
    
    uLoff[$i] = $MEAN
    duL[$i] = $SIGMA

    mget resultsB tmp -y $i
    vstat -q tmp
    
    uBoff[$i] = $MEAN
    duB[$i] = $SIGMA  
  end
end

macro mkstars
  if ($0 != 3)
    echo "USAGE: mkstars (Nstars) (d kpc)"
    break
  end

  create seq 0 $1
  set phi   = 2*rnd(seq) - 1

  set L = 360*rnd(seq)
  set B = dasin(phi)

  set R = L
  set D = B
  csystem G C R D

  set uL_gal =     ($A_oort * dcos(2.0*L) + $B_oort) * dcos(B*1.0) * $iFkap;
  set uB_gal = -0.5*$A_oort * dsin(2.0*L) *            dsin(B*2.0) * $iFkap;

  set uL_sol =  ($U_sol * dsin(L) - $V_sol * dcos(L))                           * $iFkap / $2;
  set uB_sol = (($U_sol * dcos(L) + $V_sol * dsin(L))*dsin(B) - $W_sol*dcos(B)) * $iFkap / $2;

  set uL = uL_gal + uL_sol
  set uB = uB_gal + uB_sol

  # C1, C2 are from http://arxiv.org/pdf/1306.2945v2.pdf
  set C1 =  dcos(D)*$trans_cos_phi + dsin(D)*(dcos(R)*$trans_sin_phi_sin_Xo - dsin(R)*$trans_sin_phi_cos_Xo);
  set C2 =                                -1*(dcos(R)*$trans_sin_phi_cos_Xo + dsin(R)*$trans_sin_phi_sin_Xo);

  set cosBinv = 1.0 / sqrt(C1*C1 + C2*C2);

  set uR = cosBinv * (C1 * uL - C2 * uB);
  set uD = cosBinv * (C1 * uB + C2 * uL);
end

macro pltstars
  if ($0 != 2)
    echo "USAGE: pltstars (scale)"
    break
  end

  resize -n galactic 1600 800
  region 0 0 90 ait; cgrid -c red

  cneedles L B uL uB -scale $1 -c black -lw 0
  cplot L B -pt 0 -sz 0.3 -c black
  label +x Galactic -fn courier 24

  resize -n celestial 1600 800
  region 0 0 90 ait; cgrid -c red

  style -c red -lw 0 -pt 0 -sz 0.3; cggrid; style -c blue; galactic
  cplot R D -pt 0 -sz 0.3 -c black
  cneedles R D uR uD -scale $1 -c black -lw 0
  label +x Celestial -fn courier 24
end

macro cggrid

 local delta off
 $delta = 2.0

 delete _lgrid _bgrid

 create _tgrid 0 360 $delta
 set _n = ramp(_tgrid) 
 set _lgrid_t = _tgrid * (_n % 2) + (_tgrid - $delta) * (_n % 2 == 0)

 for off -70 90 20
   set _bgrid_t = zero(_lgrid_t) + $off
   concat _lgrid_t _lgrid
   concat _bgrid_t _bgrid
 end

 create _tgrid -90 90 $delta
 set _n = ramp(_tgrid) 
 set _bgrid_t = _tgrid * (_n % 2) + (_tgrid - $delta) * (_n % 2 == 0)

 for off 0 360 30
   set _lgrid_t = zero(_bgrid_t) + $off
   concat _lgrid_t _lgrid
   concat _bgrid_t _bgrid
 end

 csystem G C _lgrid _bgrid
 cplot _lgrid _bgrid -pt 100
end


macro check.region
  if ($0 != 6)
    echo "USAGE: check.region (Rmin) (Rmax) (Dmin) (Dmax) (dist)"
    break
  end
  
  mkstars 10000 $5

  set keep = (R > $1) && (R < $2) && (D > $3) && (D < $4)
  subset  Rs =  R if keep
  subset  Ds =  D if keep
  subset uRs = uR if keep
  subset uDs = uD if keep
  lim -0.01 0.01 -0.01 0.01; clear; box; plot uRs uDs -pt 0 -sz 0.5
  dot -0.003 -0.004 -pt 7 -c red -sz 4
end
