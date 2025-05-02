
macro test.mean

 $Nsample = 16
 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 50 50 0.0 1.0
  medimage add t1 t
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 medimage calc t1 T -mean

 imhist -q T x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 echo "expect {1/sqrt($Nsample)} : $C1"
 plot -c red -x line x yf
end

macro test.median

 $Nsample = 16
 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 50 50 0.0 1.0
  medimage add t1 t
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
 end

 # note that median of a gaussian distributed variable is not distributed with sigma' = sigma / sqrt(N)
 # (somewhat higher scatter)
 medimage calc t1 T

 imhist -q T x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 echo "expect {1/sqrt($Nsample)} : $C1 (actually should be a bit higher)"
 plot -c red -x line x yf
end
 
macro test.wtmean

 $Nsample = 8
 $sig1 = 1.0
 $sig2 = 3.0

 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 50 50 0.0 $sig1
  set v = $sig1^2 + zero(t)      
  medimage add t1 t -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 for i 0 $Nsample
  mgaussdev t 50 50 0.0 $sig2
  set v = $sig2^2 + zero(t)      
  medimage add t1 t -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 # note that median of a gaussian distributed variable is not distributed with sigma' = sigma / sqrt(N)
 # (somewhat higher scatter)
 medimage calc t1 T -wtmean

 imhist -q T x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 $S1 = $Nsample / $sig1^2 + $Nsample / $sig2^2
 echo "expect {1/sqrt($S1)} : $C1"
 plot -c red -x line x yf
end

macro test.irls
 medimage delete -q t1
 $Nsample = 16
 $sig = 1.0
 for i 0 $Nsample
  mgaussdev t 50 50 0.0 $sig
  set v = $sig^2 + zero(t)      

  set bad = (rnd(t) < 0.05) ? 10*rnd(t) + 5 : zero(t)
  set ts = t + bad

  medimage add t1 ts -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 # get stats for straight mean:
 medimage calc t1 Tm -mean

 imhist -q Tm x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 echo "sigma from straight stdev: $C1"
 # stats Tm
 
 plot -c red -x line x yf

 # get stats for irls
 medimage calc t1 Ti -irls

 imhist -q Ti x y -range -10 10 -delta 0.1
 lim -n 2 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 echo "sigma from irls: $C1 (ideal is {$sig/sqrt($Nsample)})"
 # stats Ti
 
 plot -c red -x line x yf
end


###################33


macro test.mean.var

 $Nsample = 64
 $sig = 2.0
 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 100 100 0.0 $sig
  medimage add t1 t
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 medimage calc t1 T -mean -variance Tv

 imhist -q T x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 plot -c red -x line x yf

 imhist Tv xv yv -range -1 4 -delta 0.1
 lim -n 2 xv yv; clear; box; plot xv yv -x hist

 stat Tv
 echo "$C1 vs {sqrt($MEDIAN)} : expect {$sig/sqrt($Nsample)}"
end

macro test.median.var

 $Nsample = 64
 $sig = 2.0
 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 50 50 0.0 $sig
  medimage add t1 t
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 # note that median of a gaussian distributed variable is not distributed with sigma' = sigma / sqrt(N)
 # (somewhat higher scatter)
 medimage calc t1 T -variance Tv

 imhist -q T x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 plot -c red -x line x yf

 imhist Tv xv yv -range -1 4 -delta 0.1
 lim -n 2 xv yv; clear; box; plot xv yv -x hist

 stat Tv
 echo "$C1 vs {sqrt($MEDIAN)} : expect {$sig/sqrt($Nsample)}"
end
 
macro test.wtmean.var

 $Nsample = 32
 $sig1 = 1.0
 $sig2 = 1.0

 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 50 50 0.0 $sig1
  set v = $sig1^2 + zero(t)      
  medimage add t1 t -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 for i 0 $Nsample
  mgaussdev t 50 50 0.0 $sig2
  set v = $sig2^2 + zero(t)      
  medimage add t1 t -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 # note that median of a gaussian distributed variable is not distributed with sigma' = sigma / sqrt(N)
 # (somewhat higher scatter)
 medimage calc t1 T -wtmean -variance Tv

 imhist -q T x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 plot -c red -x line x yf

 stat -q Tv
 $S1 = $Nsample / $sig1^2 + $Nsample / $sig2^2
 echo $C1 vs {sqrt($MEDIAN)} : expect {1/sqrt($S1)}
end

macro test.irls.var

 $Nsample = 16
 $sig = 1.0

 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 50 50 0.0 $sig
  set v = $sig^2 + zero(t)      

  set bad = (rnd(t) < 0.05) ? 10*rnd(t) + 5 : zero(t)
  set ts = t + bad

  medimage add t1 ts -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 # get stats for straight mean:
 medimage calc t1 Tm -mean -variance Tv

 imhist -q Tm x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 echo "sigma from straight stdev: $C1"
 # stats Tm
 
 plot -c red -x line x yf

 # get stats for irls
 medimage calc t1 Ti -irls -variance Tv

 imhist -q Ti x y -range -10 10 -delta 0.1
 lim -n 2 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 echo "sigma from irls: $C1 (ideal is {$sig/sqrt($Nsample)})"
 # stats Ti
 
 plot -c red -x line x yf

 set dTv = sqrt(Tv)
 imhist dTv xv yv -range -1 4 -delta 0.02; lim -n 3 xv yv; clear; box; plot xv yv -x hist

 stat -q Tv
 echo $C1 vs {sqrt($MEDIAN)} (ideal is {$sig/sqrt($Nsample)})"
end

macro test.irls.boot.var

 $Nsample = 64
 $sig = 1.0

 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 200 200 0.0 $sig
  set v = $sig^2 + zero(t)      

  set bad = (rnd(t) < 0.05) ? 10*rnd(t) + 5 : zero(t)
  set ts = t + bad

  mgaussdev noise 200 200 0.0 0.5
  set ts = ts + noise

  medimage add t1 ts -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  # echo $C1
 end

 # get stats for straight mean:
 medimage calc t1 Tm -mean -variance Tv

 imhist -q Tm x y -range -10 10 -delta 0.02
 lim -n 1 x y; clear; box; plot -x hist x y
 peak -q x y
 $C0 = $peakpos
 $C1 = 1.5*$sig / sqrt($Nsample)
 $C2 = $peakval
 $C3 = 0
 vgauss -q x y con yf
 echo "sigma from straight stdev: $C1"
 # stats Tm
 
 plot -c red -x line x yf

 # get stats for irls
 date
 medimage calc t1 Ti -irls -variance Tv -bootstrap-iter 100
 date

 imhist -q Ti x y -range -10 10 -delta 0.02
 lim -n 2 x y; clear; box; plot -x hist x y
 peak -q x y
 $C0 = $peakpos
 $C1 = 1.5*$sig / sqrt($Nsample)
 $C2 = $peakval
 $C3 = 0
 vgauss -q x y con yf
 echo "sigma from irls: $C1 (ideal is {$sig/sqrt($Nsample)})"
 # stats Ti
 
 plot -c red -x line x yf

 set dTv = sqrt(Tv)
 imhist dTv xv yv -range 0 {5*$sig/sqrt($Nsample)} -delta 0.02; lim -n 3 xv yv; clear; box; plot xv yv -x hist

 stat -q Tv
 echo $C1 vs {sqrt($MEDIAN)} (ideal is {$sig/sqrt($Nsample)})"
end

##############################
macro test.irls.boot.test

 $Nsample = 100
 $sig1 = 1.0

 medimage delete -q t1
 for i 0 $Nsample
  mgaussdev t 100 100 0.0 $sig1
  set v = $sig1^2 + zero(t)      

  medimage add t1 t -variance v
 end

 # get stats for irls
 medimage calc t1 Ti -irls -variance Tv -bootstrap

 imhist -q Ti x y -range {-10*$sig1/sqrt($Nsample)} {10*$sig1/sqrt($Nsample)} -delta 0.01
 lim -n 2 x y; clear; box; plot -x hist x y
 peak -q x y
 $C0 = $peakpos
 $C1 = 1.5*$sig1/sqrt($Nsample)
 $C2 = $peakval
 $C3 = 0
 vgauss x y con yf
 echo "sigma from irls: $C1 (ideal is {$sig1/sqrt($Nsample)})"
 # stats Ti
 
 plot -c red -x line x yf

 set dTv = sqrt(Tv)
 imhist dTv xv yv -range 0 {5*$sig1/sqrt($Nsample)} -delta 0.02; lim -n 3 xv yv; clear; box; plot xv yv -x hist

 stat -q irls_npt
 $Npix = $MEAN

 stat -q Tv
 echo "sigma of irls average: $C1, sqrt(mean) of irls variance: {sqrt($MEAN)}, (ideal is {$sig1/sqrt($Npix)})"
end

macro test.irls.range.var
 medimage delete -q t1
 for i 0 8
  mgaussdev t 50 50 0.0 1.0
  set v = 1.0 + zero(t)      

  set bad = (rnd(t) < 0.05) ? 10*rnd(t) + 5 : zero(t)
  set ts = t + bad

  medimage add t1 ts -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  echo $C1
 end

 for i 0 8
  mgaussdev t 50 50 0.0 3.0
  set v = 3.0 + zero(t)      

  set bad = (rnd(t) < 0.05) ? 10*rnd(t) + 5 : zero(t)
  set ts = t + bad

  medimage add t1 ts -variance v
  imhist -q t x y -range -10 10 -delta 0.1

  $C0 = 0
  $C1 = 1.5
  $C2 = 400
  $C3 = 0
  vgauss -q x y con yf
  echo $C1
 end

 # get stats for straight mean:
 medimage calc t1 Tm -mean -variance Tv

 imhist -q Tm x y -range -10 10 -delta 0.1
 lim -n 1 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 echo $C1
 stats Tm
 
 plot -c red -x line x yf

 # get stats for irls
 medimage calc t1 Ti -irls -variance Tv

 imhist -q Ti x y -range -10 10 -delta 0.1
 lim -n 2 x y; clear; box; plot -x hist x y
 $C0 = 0
 $C1 = 1.5
 $C2 = 400
 $C3 = 0
 vgauss -q x y con yf
 echo $C1
 stats Ti
 
 plot -c red -x line x yf

 stat -q Tv
 echo $C1 vs {sqrt($MEDIAN)}

 set dTv = sqrt(Tv)
 imhist dTv xv yv -range -1 4 -delta 0.02; lim -n 3 xv yv; clear; box; plot xv yv -x hist
end
