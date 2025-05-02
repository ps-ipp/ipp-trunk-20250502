
macro mkimage2
  if ($0 != 2)
    echo "USAGE: mkimage2 (angle)"
    break
  end

  # create an image and set the WCS

  mcreate im 500 500

  keyword im CTYPE1 -w "RA---TAN"
  keyword im CTYPE2 -w "DEC--TAN"

  keyword im CRVAL1 -wf 10.00
  keyword im CRVAL2 -wf 45.00
 
  keyword im CRPIX1 -wf 250
  keyword im CRPIX2 -wf 250

  keyword im CDELT1 -wf {0.25/3600}
  keyword im CDELT2 -wf {0.25/3600}

  keyword im PC001001 -wf {dcos($1)}
  keyword im PC002001 -wf {dsin($1)}
  keyword im PC001002 -wf {-1.0*dsin($1)}
  keyword im PC002002 -wf {dcos($1)}

  for ix 50 500 100
    for iy 50 500 100
      im[$ix][$iy] = 1
      concat $ix Xc
      concat $iy Yc
    end
  end

  set Rc = Xc
  set Dc = Yc

  coords im -p Rc Dc
  
  clear -s 

  tv im -0.2 1.5
  region -image -no-clear
  cplot Rc Dc -pt 7 -sz 3.0 -x 2 -c red

  create r1 9.0 11.0 0.001
  create d2 44.0 46.0 0.001
  set d1 = 45.0 + zero(r1)
  set r2 = 10.0 + zero(d2)
  cplot r1 d1 -c blue -x 0 ; 
  cplot r2 d2 -c red -x 0

  set d1 = Dc[0] + zero(r1)
  set r2 = Rc[0] + zero(d2)
  cplot r1 d1 -c blue -x 0 ; 
  cplot r2 d2 -c red -x 0
end

macro mkimage

  # create an image and set the WCS

  mcreate im 500 500

  keyword im CTYPE1 -w "RA---TAN"
  keyword im CTYPE2 -w "DEC--TAN"

  keyword im CRVAL1 -wf 10.00
  keyword im CRVAL2 -wf 45.00
 
  keyword im CRPIX1 -wf 250
  keyword im CRPIX2 -wf 250

  keyword im CDELT1 -wf {0.25/3600}
  keyword im CDELT2 -wf {0.25/3600}

  keyword im PC001001 -wf 1.0
  keyword im PC002001 -wf 0.0
  keyword im PC001002 -wf 0.0
  keyword im PC002002 -wf 1.0

  for ix 50 500 100
    for iy 50 500 100
      im[$ix][$iy] = 1
      concat $ix Xc
      concat $iy Yc
    end
  end

  tv im -0.2 1.5

  set Rc = Xc
  set Dc = Yc

  coords im -p Rc Dc
  
  region -image 
  cplot Rc Dc -pt 7 -sz 3.0 -x 2 -c red
end

macro raline
  create rx 0 360
  set dx = zero(rx) + $1
  coords im -c rx dx
  break -auto off
  for i 0 rx[]
    $ix = rx[$i]
    $iy = dx[$i]
    im[$ix][$iy] = 1.0
  end
  break -auto on
end

macro decline
  create dx -90 90
  set rx = zero(dx) + $1
  coords im -c rx dx
  break -auto off
  for i 0 rx[]
    $ix = rx[$i]
    $iy = dx[$i]
    im[$ix][$iy] = 1.0
  end
  break -auto on
end

macro mkallsky

  # create an image and set the WCS

  mcreate im 400 200

  keyword im CTYPE1 -w "RA---AIT"
  keyword im CTYPE2 -w "DEC--AIT"

  keyword im CRVAL1 -wf 0.00
  keyword im CRVAL2 -wf 0.00
 
  keyword im CRPIX1 -wf 200
  keyword im CRPIX2 -wf 100

  keyword im CDELT1 -wf 1.0
  keyword im CDELT2 -wf 1.0

  keyword im PC001001 -wf 1.0
  keyword im PC002001 -wf 0.0
  keyword im PC001002 -wf 0.0
  keyword im PC002002 -wf 1.0

  raline -45.0
  raline 0.0
  raline 45.0

  decline -90.0
  decline 0.0
  decline 90.0

  clear -s
  tv im -0.2 1.5
  box

  region -image -no-clear

  skylines
end

macro skylines
  create rx 0 360
  set dx1 = zero(rx) - 45.0
  cplot rx dx1 -x 2 -pt 0 -sz 0.5 -c blue
  set dx2 = zero(rx) - 0.0
  cplot rx dx2 -x 2 -pt 0 -sz 0.5 -c blue
  set dx3 = zero(rx) + 45.0
  cplot rx dx3 -x 2 -pt 0 -sz 0.5 -c blue

  create dy -90 90
  set ry1 = zero(dy) - 90.0
  cplot ry1 dy -x 2 -pt 0 -sz 0.5 -c blue
  set ry2 = zero(dy) - 0.0
  cplot ry2 dy -x 2 -pt 0 -sz 0.5 -c blue
  set ry3 = zero(dy) + 90.0
  cplot ry3 dy -x 2 -pt 0 -sz 0.5 -c blue
end
