
if (not($?PLOT)) set PLOT = 0

list tests
 test1
 test2
 test3
 memtest1
end

# test using even samples
macro test1
 $PASS = 1
 break -auto off

 local P PI
 $PI = 3.14159265359
 $P  = 15.0

 delete -q t f period power

 create t 0 100
 set f = sin(2*$PI*t/$P)
 set df = 0.01 + zero(f)

 periodogram_fm t f df 5 50 period power

 peak -q period power

 if (abs ($peakpos - $P) > 0.5)
   $PASS = 0
   echo "OFFSET: {$peakpos - $P}"
 end

 if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using even samples
macro test1a
 $PASS = 1
 break -auto off

 local P PI
 $PI = 3.14159265359
 $P  = 15.0

 delete -q t f period power

 create t 0 100
 set f = sin(2*$PI*t/$P)
 set df = 0.01 + zero(f)

 periodogram_fm t f df 5 50 period power -Nperiods 512

 peak -q period power

 if (abs ($peakpos - $P) > 0.5)
   $PASS = 0
   echo "OFFSET: {$peakpos - $P}"
 end

 if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using random samples
macro test2
 $PASS = 1
 break -auto off

 local P PI
 $PI = 3.14159265359
 $P  = 15.0

 delete -q x t f period power

 create x 0 100
 set t = 100 * rnd(x) 
 set f = sin(2*$PI*t/$P)
 set df = 0.01 + zero(f)

 periodogram_fm t f df 5 50 period power

 # lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
 # lim -n 1 period power; clear; box; plot period power

 peak -q period power

 if (abs ($peakpos - $P) > 0.5)
   $PASS = 0
 end

 if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using random samples, higher frequency
macro test3
 $PASS = 1
 break -auto off

 local P PI
 $PI = 3.14159265359
 $P  = 2.0

 delete -q x t f period power

 create x 0 100
 set t = 100 * rnd(x) 
 set f = sin(2*$PI*t/$P)
 set df = 0.01 + zero(f)

 periodogram_fm t f df 1 10 period power

 # lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
 # lim -n 1 period power; clear; box; plot period power

 peak -q period power

 if (abs ($peakpos - $P) > 0.05)
   $PASS = 0
 end

  if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using random samples, offset start
macro test4
 $PASS = 1
 break -auto off

 local P PI
 $PI = 3.14159265359
 $P  = 15.0

 delete -q x t f period power

 create x 500 800
 set t = 300 * rnd(x) + 500
 set f = sin(2*$PI*t/$P)
 set df = 0.01 + zero(f)

 periodogram_fm t f df 2 30 period power

#  lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
#  lim -n 1 period power; clear; box; plot period power

 peak -q period power

 if (abs ($peakpos - $P) > 0.05)
   $PASS = 0
 end

 if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using random samples, offset start, non-zero DC
macro test5
 $PASS = 1
 break -auto off

 local P PI
 $PI = 3.14159265359
 $P  = 15.0

 delete -q x t f period power

 create x 500 800
 set t = 300 * rnd(x) + 500
 set f = sin(2*$PI*t/$P) + 0.5
 set df = 0.01 + zero(f)

 periodogram_fm t f df 2 30 period power

#  lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
#  lim -n 1 period power; clear; box; plot period power

 peak -q period power

 if (abs ($peakpos - $P) > 0.05)
   $PASS = 0
 end

 if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using 300 random samples, offset start, non-zero DC, some noise
macro test6
 $PASS = 1
 break -auto off

 local P PI
 $PI = 3.14159265359
 $P  = 15.0

 delete -q x t f period power

 create x 500 800
 set t = 300 * rnd(x) + 500
 set fraw = sin(2*$PI*t/$P) + 0.5

 # 0.05 : peakpos = 14.95
 # 0.10 : peakpos = 15.04 (
 gaussdev df t[] 0.0 0.25
 set f = fraw + df
 set df = 0.01 + zero(f)

 periodogram_fm t f df 2 30 period power

#  lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
#  lim -n 1 period power; clear; box; plot period power

 peak -q period power

 if (abs ($peakpos - $P) > 0.05)
   $PASS = 0
 end

 if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using 100 fewer random samples, offset start, non-zero DC, some noise
macro test7
 $PASS = 1
 break -auto off

 local P PI
 $PI = 3.14159265359
 $P  = 15.0

 delete -q x t f period power

 create x 0 100
 set t = 100 * rnd(x)
 set fraw = sin(2*$PI*t/$P) + 0.5

 # 0.05 : peakpos = 14.95
 # 0.10 : peakpos = 15.04 (
 gaussdev df t[] 0.0 0.25
 set f = fraw + df
 set df = 0.01 + zero(f)

 periodogram_fm t f df 2 30 period power

#  lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
#  lim -n 1 period power; clear; box; plot period power

 peak -q period power

 if (abs ($peakpos - $P) > 0.05)
   $PASS = 0
 end

 if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using Ndays random samples, RR Lyrae-sized light curves (0.7 mag),
# optional noise level 
macro test8
 if ($0 != 4)
   echo "USAGE: test8: Period Ndays (df)"
   break
 end
 
 local Ndays 
 $P = $1
 $Ndays = $2
 $dM = $3

 $PASS = 1
 break -auto off

 local PI
 $PI = 3.14159265359
 $trueP = $P

 delete -q x t f period power

 create x 0 $Ndays

 # t is a time in days, but we always have 4 within 1 hour:
 set tday = int(100 * rnd(x)); # choose Ndays random days between 0 and 100
 set dtx = (3/24) * rnd(x);  # choose a starting time within that night
 set t0 = tday + dtx

 set dt1 = (15.0 / 1440) * rnd(x) + ( 0 + 7.5) / 1440
 set dt2 = (15.0 / 1440) * rnd(x) + (15 + 7.5) / 1440
 set dt3 = (15.0 / 1440) * rnd(x) + (30 + 7.5) / 1440

 delete -q t
 concat t0 t
 set tmp = t0 + dt1; concat tmp t
 set tmp = t0 + dt2; concat tmp t
 set tmp = t0 + dt3; concat tmp t

 set fraw = 0.75*sin(2*$PI*t/$P)

 # 0.05 : peakpos = 14.95
 # 0.10 : peakpos = 15.04 (
 gaussdev df t[] 0.0 $dM
 set f = fraw + df
 set df = $dM + zero(f)

 periodogram_fm t f df 0.1 20.0 period power

#  lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
#  lim -n 1 period power; clear; box; plot period power

 peak -q period power

 if (abs ($peakpos - $P) > 0.05)
   $PASS = 0
 end
 if ($PLOT)
  lim period power; clear; box; line -c red70 -lw 3 $P 0 to $P $peakval; plot period power -x line
 end
end

# test using Ndays random samples, RR Lyrae-sized light curves (0.7 mag),
# optional noise level 
# compare periodogram and periodogram_fm
macro test9
 if ($0 != 4)
   echo "USAGE: test8: Period Ndays (df)"
   break
 end
 
 local Ndays 
 $P = $1
 $Ndays = $2
 $dM = $3

 $PASS = 1
 break -auto off

 local PI
 $PI = 3.14159265359
 $trueP = $P

 delete -q x t f period power

 create x 0 $Ndays

 # t is a time in days, but we always have 4 within 1 hour:
 set tday = int(100 * rnd(x)); # choose Ndays random days between 0 and 100
 set dtx = (3/24) * rnd(x);  # choose a starting time within that night
 set t0 = tday + dtx

 set dt1 = (15.0 / 1440) * rnd(x) + ( 0 + 7.5) / 1440
 set dt2 = (15.0 / 1440) * rnd(x) + (15 + 7.5) / 1440
 set dt3 = (15.0 / 1440) * rnd(x) + (30 + 7.5) / 1440

 delete -q t
 concat t0 t
 set tmp = t0 + dt1; concat tmp t
 set tmp = t0 + dt2; concat tmp t
 set tmp = t0 + dt3; concat tmp t

 set fraw = 0.75*sin(2*$PI*t/$P)

 # 0.05 : peakpos = 14.95
 # 0.10 : peakpos = 15.04 (
 gaussdev df t[] 0.0 $dM
 set f = fraw + df
 set df = $dM + zero(f)

 periodogram_fm t f df 0.1 20.0 period_fm power_fm
 periodogram t f 0.1 20.0 period power

#  lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
#  lim -n 1 period power; clear; box; plot period power

 peak -q period_fm power_fm
 $peakval_fm = $peakval

 peak -q period power
 vstat -q power
 set power = power / $MAX

# if (abs ($peakpos - $P) > 0.05)
#   $PASS = 0
# end

 set freq = 1 / period
 set freq_fm = 1 / period_fm
 $Freq = 1 / $P

 if ($PLOT)
  if (1)
    lim period power; clear; box
    line -c red70 -lw 3 $P 0 to $P $peakval_fm;
    plot period power -x line -c grey70 -lw 2
    plot period_fm power_fm -x line -c black
  else
    lim freq power; clear; box
    line -c red70 -lw 3 $Freq 0 to $Freq 1.0
    plot freq power -x line -c grey70 -lw 2
    plot freq_fm power_fm -x line -c black
  end
 end
end


# test using Ndays random samples, RR Lyrae-sized light curves (0.7 mag),
# optional noise level 
# compare periodogram and periodogram_fm
macro test10
 if ($0 != 4)
   echo "USAGE: test8: Period Ndays (df)"
   break
 end
 
 local Ndays 
 $P = $1
 $Ndays = $2
 $dM = $3

 $PASS = 1
 break -auto off

 local PI
 $PI = 3.14159265359
 $trueP = $P

 delete -q x t f period power

 create x 0 $Ndays

 # t is a time in days, but we always have 4 within 1 hour:
 set tday = int(100 * rnd(x)); # choose Ndays random days between 0 and 100
 set dtx = (3/24) * rnd(x);  # choose a starting time within that night
 set t0 = tday + dtx

 set dt1 = (15.0 / 1440) * rnd(x) + ( 0 + 7.5) / 1440
 set dt2 = (15.0 / 1440) * rnd(x) + (15 + 7.5) / 1440
 set dt3 = (15.0 / 1440) * rnd(x) + (30 + 7.5) / 1440

 delete -q t
 concat t0 t
 set tmp = t0 + dt1; concat tmp t
 set tmp = t0 + dt2; concat tmp t
 set tmp = t0 + dt3; concat tmp t

 set fraw = 0.75*sin(2*$PI*t/$P)

 # 0.05 : peakpos = 14.95
 # 0.10 : peakpos = 15.04 (
 gaussdev df t[] 0.0 $dM
 set f = fraw + df
 set df = $dM + zero(f)

 periodogram_fm t f df 0.05 20.0 period_fm power_fm

 gaussdev df t[] 0.0 $dM
 set Fo = df
 periodogram_fm t Fo df 0.05 20.0 period power

#  lim -n 0 t f; clear; box; plot -x 2 -pt 2 t f
#  lim -n 1 period power; clear; box; plot period power

 peak -q period_fm power_fm
 $peakval_fm = $peakval

 peak -q period power

# if (abs ($peakpos - $P) > 0.05)
#   $PASS = 0
# end

 set freq = 1 / period
 set freq_fm = 1 / period_fm
 $Freq = 1 / $P

 if ($PLOT)
  if (1)
    lim period_fm power_fm; clear; box
    line -c red70 -lw 3 $P 0 to $P $peakval_fm;
    plot period power -x line -c grey70 -lw 2
    plot period_fm power_fm -x line -c black
  else
    lim freq power; clear; box
    line -c red70 -lw 3 $Freq 0 to $Freq 1.0
    plot freq power -x line -c grey70 -lw 2
    plot freq_fm power_fm -x line -c black
  end
 end
end

# we have time (MJD) and mag
# we generate the folded lightcure and measure sigma relative to the smoothed version (bins of 0.1 period)
macro fold.one.period
  if ($0 != 5)
    echo "USAGE: fold.one.period (time) (mag) (magErr) (period)"
    break
  end

  local myTime myMag myMagErr myPeriod
  $myTime = $1
  $myMag  = $2
  $myMagErr  = $3
  $myPeriod = $4

  set phi = $myTime / $myPeriod - int($myTime / $myPeriod)

  if ($PLOT_FOLD)
    lim -n phi phi $myMag; clear; box; 
  end

  delete -q magResid

  $dPhi = 0.05; # half of bin size
  create nphi $dPhi {1 + $dPhi} {2*$dPhi}
  set magR = zero(nphi)
  set magS = zero(nphi)
  for i 0 nphi[]
    subset tmp_mag_sub = $myMag where (phi >= nphi[$i] - $dPhi) && (phi < nphi[$i] + $dPhi)
    vstat -q tmp_mag_sub
    magR[$i] = $MEDIAN
    magS[$i] = $SIGMA

    set magDelta = tmp_mag_sub - $MEDIAN
    concat magDelta magResid 

    if ($PLOT_FOLD)
      subset tmp_phi_sub = phi where (phi >= nphi[$i] - $dPhi) && (phi < nphi[$i] + $dPhi)
      if ($i % 2)
        plot tmp_phi_sub tmp_mag_sub -pt 7 -sz 3 -c blue -lw 2
      else
        plot tmp_phi_sub tmp_mag_sub -pt 7 -sz 3 -c red -lw 2
      end
    end  
  end

  if ($PLOT_FOLD)  
    plot -pt 10 -sz 1.5 phi $myMag -dy $myMagErr
    plot -pt 2 -sz 2.0 -c red nphi magR -dy magS
  end

  vstat -q magResid
end



# Memory test
macro memtest1

 local i
 local P PI
 $PI = 3.14159265359
 $P  = 15.0

 delete -q x t f period power

 create x 500 800
 set t = 300 * rnd(x) + 500
 set f = sin(2*$PI*t/$P)

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 100
  periodogram_fm t f df 2 30 period power
 end
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 180)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/100}"
 end
end
