# -*- perl -*-

macro navigate
  style -n 0
  limits
  $DSTARS = -1
  $ZOOM = 180 / ($YMAX - $YMIN)
  for i 1 100
    $REDRAW = 0
    cursor -g 1
    # zoom controls
    if ("$KEY" == "1")
      $ZOOM = $ZOOM * 2
      $REDRAW = 1
      $Rnum = $R$KEY		      
      $Dnum = $D$KEY
      $KEY = num
    end
    if ("$KEY" == "2")
      $ZOOM = $ZOOM * 1.2
      $REDRAW = 1
      $Rnum = $R$KEY		      
      $Dnum = $D$KEY
      $KEY = num
    end
    if ("$KEY" == "3")
      $REDRAW = 1
      $Rnum = $R$KEY		      
      $Dnum = $D$KEY
      $KEY = num
    end
    if ("$KEY" == "4")
      $ZOOM = $ZOOM / 1.2
      $REDRAW = 1
      $Rnum = $R$KEY		      
      $Dnum = $D$KEY
      $KEY = num
    end
    if ("$KEY" == "5")
      $ZOOM = $ZOOM / 2
      $REDRAW = 1
      $Rnum = $R$KEY		      
      $Dnum = $D$KEY
      $KEY = num
    end
    if ("$KEY" == "6")
      $ZOOM = $ZOOM / 20
      $REDRAW = 1
      $Rnum = $R$KEY		      
      $Dnum = $D$KEY
      $KEY = num
    end

    # help list
    if ("$KEY" == "h")
     echo "1 - zoom in factor of 2"
     echo "2 - zoom in factor of 1.2"
     echo "3 - recenter at cursor"
     echo "4 - zoom out factor of 1.2"
     echo "5 - zoom out factor of 2"
     echo "6 - zoom out factor of 10"
     echo "z - zoom to radius (requires 2nd keystroke)"
     echo "f - show full sky"
     echo ""
     echo "q - quit"
     echo "g - toggle skygrid on/off"
     echo "r - plot detected asteroids (rocks)"
     echo "l - plot HST GSC"
     echo "L - plot Landolt stars"
     echo "m - list measurements for stars within 1 pixel of cursor"
     echo "M - list measurements for stars within 1.8 arcsec of cursor"
     echo "i - list info about images touching cursor location" 
     echo "I - list info about images, with pixel coords of cursor position"
     echo "x - plot stars scaled by magnitude Chisq"
     echo "X - plot stars by magnitude scatter"
     echo "S - toggle auto-plotting of stars"
     echo "t - plot light curve for star within 2 arcsec of cursor position"
     echo "T - plot 'galaxy' light curve for star within 2 arcsec of cursor position"
     echo "c - plot status catalog boundaries"
     echo "C - list catalog at cursor location"
    end

    # quit from navigate
    if ("$KEY" == "q") 
      break
    end

    # measure distance
    if ("$KEY" == "d")
      $r0 = $R$KEY
      $d0 = $D$KEY
      $ok = $KEY
      echo "type at radius"
      cursor -g 1
      $r1 = $R$KEY
      $d1 = $D$KEY
      $dr = 3600*((dcos($d0)*($r0-$r1))^2 + ($d0-$d1)^2)^0.5
      echo "$dr arcsec"
    end
    # show ra, dec
    if ("$KEY" == "w")
      $tmp = $R$KEY
      if ($tmp < 0) 
        $tmp = $R$KEY + 360.0
      end
      echo "$tmp $D$KEY" 
      exec echo $tmp $D$KEY | radec -hh
    end
    # zoom to radius
    if ("$KEY" == "z")
      $r0 = $R$KEY
      $d0 = $D$KEY
      $ok = $KEY
      echo "type at radius"
      cursor -g 1
      $r1 = $R$KEY
      $d1 = $D$KEY
      $dr = (($r0-$r1)^2 + ($d0-$d1)^2)^0.5
      $ZOOM = $RAD / $dr
      $REDRAW = 1
      $KEY = $ok
      $R$KEY = $r0
      $D$KEY = $d0
    end

    # adjust mag scaling
    if ("$KEY" == "J")
      $MAG = $MAG - 0.5
      $REDRAW = 2
    end
    if ("$KEY" == "K")
      $MAG = $MAG + 0.5
      $REDRAW = 2
    end
    if ("$KEY" == "j")
      $dMAG = $dMAG * 0.8
      $REDRAW = 2
    end
    if ("$KEY" == "k")
      $dMAG = $dMAG * 1.25
      $REDRAW = 2
    end
    echo "mag, dmag: $MAG, $dMAG"

    # redraw region 
    if ($REDRAW == 1)
      region $R$KEY $D$KEY {$RAD/$ZOOM} sin
      $RMIN = $R$KEY + $XMIN
      $RMAX = $R$KEY + $XMAX
      $DMIN = $D$KEY + $YMIN
      $DMAX = $D$KEY + $YMAX
      if (($ZOOM < 20) && ($GRID == 1)) 
        style -c red; cgrid
      end
      if (($ZOOM > 20) && ($DSTARS == 1))
        style -pt 7
	pmeasure -all -m $MAG {$MAG + $dMAG}
      end    
      style -c black
       images
#       showtile
    end

    # redraw region 
    if ($REDRAW == 2)
      clear
      if (($ZOOM < 20) && ($GRID == 1)) 
        style -c red; cgrid
      end
      if (($ZOOM > 20) && ($DSTARS == 1))
        style -pt 2
	pmeasure -all -m $MAG {$MAG + $dMAG}
      end    
      style -c black
      images
    end

    # turn grid on / off
    if ("$KEY" == "g")
      if ($GRID == 1) 
        $GRID = 0
        style -c white; cgrid
        style -c black
      else
        $GRID = 1 
        style -c black; cgrid
      end
    end

    # plot full sky
    if ("$KEY" == "f") 
      echo "full"
      $ZOOM = 1
      resize 1150 600		      
      region 0 0 90 ait
      $RMIN = 0
      $RMAX = 360
      $DMIN = -90
      $DMAX = +90
      style -c red; cgrid
      style -c black
      images
    end

    # plot rocks
    if ("$KEY" == "r") 
#      plot.rocks
      style -c blue   -pt 1; procks -speed 0.0041 1
      style -c red    -pt 1; procks -speed 0.00041 0.0041
      style -c indigo -pt 1; procks -speed 0 0.00041
      style -c black -lw 0;
    end
    # plot HST-GSC
    if ("$KEY" == "l") 
      style -c blue -pt 7; cat -all -g -m 9 16
      style -c black
    end
    # plot Landolt
    if ("$KEY" == "L") 
#      style -c red  -lw 2 -pt 3; cat -a 1 2 3 /data/elixir/srcdir/refs/stetson/stetsonBn.txt -m 9 18
#      style -c blue -lw 2 -pt 3; cat -a 25 26 8 /data/elixir/srcdir/refs/landolt/new/Landolt92.fix -m 9 18
      style -c red -lw 2 -pt 7; cat -a 1 2 3 /data/elixir/srcdir/refs/sdss/g_SDSS.dat -m 9 14
#      style -c red -lw 2 -pt 3; cat -a 25 26 8 /data/elixir/srcdir/refs/landolt/new/Landolt92.hq -m 9 18
#      style -c red -lw 2 -pt 3; cat -a 22 23 8 /data/elixir/srcdir/refs/landolt/new/Landolt92.unfix -m 9 18
#      style -c blue -lw 2 -pt 7; cat -a 1 2 4 /data/elixir/srcdir/refs/landolt/extreme/extreme.match -m 0 20
#      style -x 2 -c red -pt 7 ; cplot RA DEC
      style -c black -lw 0
    end

    # list star measurements
    if ("$KEY" == "m") 
        $dR = $RAD/$ZOOM/300
        if ($dR < 0.0005)
	 $dR = 0.0005
        end
	gstar $R$KEY $D$KEY $dR -m
    end

    # plot mag residuals
    if ("$KEY" == "R") 
      echo "filter: "
      cursor 1
      clear -n 1 -s; lim 10 22 -0.2 0.2; clear; box
      dmags $KEY\:rel - $KEY : $KEY -type 0
      plot -x 2 -pt 0 -sz 0.3 -c red yv xv
      dmags $KEY\:rel - $KEY : $KEY -type 0 -flag 0 -nphot +3 -chisq 2.0
      plot -x 2 -pt 2 -sz 0.5 -c black yv xv
      $KEY = R
      style -n 0
    end

    if ("$KEY" == "M") 
	gstar $R$KEY $D$KEY 0.0005 -m
    end
    # list images
    if ("$KEY" == "i") 
	gimages $R$KEY $D$KEY
    end
    if ("$KEY" == "I") 
	gimages $R$KEY $D$KEY -pix
    end
    # turn stars on / off
    if ("$KEY" == "S")
      $DSTARS = $DSTARS * -1
      if (($ZOOM > 20) && ($DSTARS == 1))
       style -pt 7
       pmeasure -all -m $MAG {$MAG + $dMAG}
      end
    end
    # plot light-curve interactive
    if ("$KEY" == "t")
      style -n 1 -pt 2 -x 2
      clear
      if ($R$KEY < 0) 
       $R$KEY = $R$KEY + 360
      end
      lcurve -l $R$KEY $D$KEY {30/3600} -d -v time mag
      box
      lcv
      style -n 0
    end
    # plot light-curve 
    if ("$KEY" == "T")
      style -n 1 -pt 1 -c red -x 2
      lcurve $R$KEY $D$KEY {30/3600} -d
      style -c black
      style -n 0 
    end
    # plot catalogs
    if ("$KEY" == "c")
      style -c blue; pcat; style -c black
    end
    # list catalogs
    if ("$KEY" == "C")
      gcat $R$KEY $D$KEY
    end

    # plot image chisqs
    if ("$KEY" == "x") 
       gcat $R$KEY $D$KEY
       extract $CATNAME Xm -photcode R
       extract $CATNAME ra
       extract $CATNAME dec
       style -x 2 -pt 7 -c blue
       czplot ra dec Xm 3 30
       style -c black -pt 1
    end
    # plot meas errors
    if ("$KEY" == "X") 
       gcat $R$KEY $D$KEY
       extract $CATNAME dM -photcode R
       extract $CATNAME ra
       extract $CATNAME dec
       style -x 2 -pt 7 -c red
       czplot ra dec dM 0 30
       style -c black -pt 1
    end
    # temp plot for skyprobe
    if ("$KEY" == "u") 
      imextract -region time
      imextract -region mcal
      imextract -region airmass
      imextract -region nstar
      vstat time
      clear -n 1;
      section a 0 0.00 1 0.33
      lim {$MEDIAN-0.3} {$MEDIAN+0.3} -0.8 -0.5; box; plot time mcal
      section b 0 0.33 1 0.33
      lim {$MEDIAN-0.3} {$MEDIAN+0.3}  0.95 3.0; box; plot time airmass
      section c 0 0.66 1 0.33
      lim {$MEDIAN-0.3} {$MEDIAN+0.3} 0 3000; box; plot time nstar
      style -n 0
    end
    if ("$KEY" == "s")
      $tmp = $R$KEY
      if ($tmp < 0) 
        $tmp = $R$KEY + 360.0
      end
      $line = `echo $tmp $D$KEY | radec -hh`
      imextract -region photcode
      imextract -region time
     
      subset t = time if (int(photcode/100) == 1)
      uniq t T
      $Bn = t[]
      $BN = T[]
      
      subset t = time if (int(photcode/100) == 2)
      uniq t T
      $Vn = t[]
      $VN = T[]
      
      subset t = time if (int(photcode/100) == 3)
      uniq t T
      $Rn = t[]
      $RN = T[]
      
      subset t = time if (int(photcode/100) == 4)
      uniq t T
      $In = t[]
      $IN = T[]
     
      echo "$line  $Bn $BN  $Vn $VN  $Rn $RN  $In $IN"
    end

    if ("$KEY" == "p") 
      echo "P - new coords; p - old coords"
      cursor -g 1
      exec echo $Rp $Dp $RP $DP >> fix.coords
    end

    if ("$KEY" == "y")
      ccd I - 2MASS_J : 2MASS_J - 2MASS_K
      lim -n 1 -1 10 -1 3; clear; box; plot -x 2 -pt 2 -sz 0.5 xv yv
      dev -n 0 -g
    end
  end
end

