
$Z_GAL = 500.0
$R_GAL = 2000.0
$N_GAL = 10000000

macro mkgal
 create f 0 $N_GAL
 set r = $R_GAL*sqrt(rnd(f))
 set L = rnd(f)*360.0
 gaussdev z r[] 0.0 $Z_GAL
 set B = atan2 (z , r)

 set RA = L
 set DEC = B

 csystem G C RA DEC
end

macro showgal
  region 0 0 90 ait
  box
  cgrid -c red
  section default -imtool none ; parity 0 1
  cdensify img RA DEC 
  set limg = log(img)
  tv limg -0.1 2.0
  tvcolor rainbow
end

macro check.proper.motion

  # create a bunch of test stars:
  create n 0 10000
  set L = 360.0*rnd(n)
  set B = dasin(2.0*rnd(n) - 1.0)

  # choose a hardwired proper motion
  set uL = 1.0*(rnd(n) - 0.5)
  set uB = 1.0*(rnd(n) - 0.5)

  set Bo = B + uB/3600.0
  set Lo = L + uL/3600.0/dcos(B)

  set R = L
  set D = B
  set Ro = Lo
  set Do = Bo

  csystem G C R D
  csystem G C Ro Do

  set uR1 = (Ro - R)*3600.0*dcos(D)
  set uD1 = (Do - D)*3600.0

  $Xo  = 282.8594812080
  $xo  =  32.9319185700
  $phi = -62.8717488056

  $delta_G = 90 + $phi
  $alpha_G = $Xo - 90

  # I got this wrong:
  # set C1 = dcos(D)*dcos($phi) + dsin(D)*dcos(R)*dsin($phi)*dsin($Xo) + dsin(D)*dsin(R)*dsin($phi)*dcos($Xo)
  # set C2 =                              dcos(R)*dsin($phi)*dcos($Xo) +         dsin(R)*dsin($phi)*dsin($Xo)

  # corrected version
    set C1 = dcos(D)*dcos($phi) + dsin(D)*dcos(R)*dsin($phi)*dsin($Xo) - dsin(D)*dsin(R)*dsin($phi)*dcos($Xo)
    set C2 =                           -1*dcos(R)*dsin($phi)*dcos($Xo) -         dsin(R)*dsin($phi)*dsin($Xo)

  # set C1 = dcos(D)*dsin($delta_G) - dsin(D)*dcos($delta_G)*dcos(R - $alpha_G)
  # set C2 =                                  dcos($delta_G)*dsin(R - $alpha_G)

  set cB = 1.0 / sqrt(C1^2 + C2^2)

  set uR2 = cB * (C1 * uL - C2 * uB)
  set uD2 = cB * (C1 * uB + C2 * uL)

  set duR = uR1 - uR2
  set duD = uD1 - uD2
end
