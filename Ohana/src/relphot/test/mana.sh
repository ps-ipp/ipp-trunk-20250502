
macro go
  if ($0 != 2)
    echo "USAGE: go (datafile)"
    break
  end

  data $1
  read Fo 1 Fx 2 dFo 4 dFx 5 chisq 7 Nstar 9 Nbad 10 Nfit 11
  set dF = Fo - Fx
  set n = ramp(Fo)

  # predicted errorbar compared to measured errorbar
  set dfo = dFo / sqrt(Nstar)
  lim -n 1 dfo dFx; clear; box; plot dfo dFx
  line -c red 0 0 to 50 50 

  # number fitted
  set fFit = Nfit / Nstar
  lim -n 2 n fFit; clear; box; plot n fFit

  # fitted mean vs truth scaled by sigma
  set dNsigma = dF / dFx
  lim -n 0 n dNsigma; clear; box; plot n dNsigma

  histogram dNsigma NdNsigma -5.0 5.0 0.1 -range dx
  $C0 = 0
  $C1 = 1.0
  $C2 = 100
  $C3 = 0
  vgauss -q dx NdNsigma con NdNsigmaF
  vstat -q dNsigma

  lim -n 3 dx NdNsigma; clear; box; plot -x 1 dx NdNsigma; plot -x 0 dx NdNsigmaF -c red

  echo $C0 $C1 $MEAN $SIGMA
end

macro testset
  if ($0 != 2)
    echo "USAGE: testset (mode)"
    break
  end

  foreach Nout 0 1 2 4 8 16
    echo --- $Nout ---
    exec bin/test_liststats.lin64 $1 -Noutliers $Nout  > test.$1.dat; go test.$1.dat
    echo -------------
  end
end

macro testset.flat
  if ($0 != 2)
    echo "USAGE: testset (mode)"
    break
  end

  foreach Nout 0 1 2 4 8 16
    echo --- $Nout ---
    exec bin/test_liststats.lin64 $1 -Noutliers $Nout -flat-outliers > test.$1.dat; go test.$1.dat
    echo -------------
  end
end
