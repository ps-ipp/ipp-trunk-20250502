
define t0
  set $i = 0
  while ($i < Nnomask)
    printf "%d : %f %f %f %f %f\n", $i, fitStats->sample[$i].X, fitStats->sample[$i].Y, fitStats->sample[$i].R, fitStats->sample[$i].D, fitStats->sample[$i].T
    set $i = $i + 1
  end
end
