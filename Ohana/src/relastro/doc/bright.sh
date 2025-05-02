
macro go.example
  if ($0 != 6)
    echo "USAGE: go.example (exptime) (zp) (Mo) (scale) color"
    break
  end

  local exptime zp Mo scale

  $exptime = $1
  $zp = $2
  $Mo = $3
  $scale = $4
  $color = $5

  create mag 10 25 0.05
  
  set flux = $exptime*ten(0.4*($zp - mag))
  set dflux = sqrt(flux)
  set dmag = dflux / flux
  set mag:inst = mag - $zp - 2.5*log($exptime)

  set dPosB = 0.335 / (1.0 + exp($scale*(mag - $Mo)))
  set dPos = sqrt(0.015^2 + dmag^2 + dPosB^2)

  dev -n 0
  plot mag dPos -c $color

  dev -n 1
  plot mag:inst dPos -c $color

  echo Mo,inst = {$Mo - $zp - 2.5*log($exptime)}
end

macro go.filters

  dev -n 0 ; lim   9.5 25.5 0.0 0.35; clear; box
  dev -n 1 ; lim -20.0 -4.0 0.0 0.35; clear; box

  go.example 43.0 24.56 13.00 1.3 blue
  go.example 40.0 24.75 12.00 1.3 green
  go.example 45.0 24.61 11.75 1.3 black
  go.example 30.0 24.25 11.25 1.8 violet
  go.example 30.0 23.32 11.00 2.0 red
end
