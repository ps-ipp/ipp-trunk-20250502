
macro memtest1
  # generate LSF:
  create x 0 7
  set lsf = exp(-0.5*(x-3)^2)

  # generate PSF:
  create xp 0 9
  set psf = exp(-0.5*(xp-5)^2/1.5^2)

  # generate slit profile:
  create xprof 0 17
  $Xpmin = 3
  $Xpmax = 13
  set profile_left  = (xprof <= $Xpmin) ? exp(-0.5*(xprof - $Xpmin)^2/1.0^2) : zero(xprof)
  set profile_right = (xprof >= $Xpmax) ? exp(-0.5*(xprof - $Xpmax)^2/1.0^2) : zero(xprof)
  set profile_mid   = (xprof > $Xpmin) && (xprof < $Xpmax) ? zero(xprof) + 1 : zero(xprof)
  set profile = profile_left + profile_mid + profile_right

  # generate object flux:
  create wave 0 100
  set flux = 100 + 0.5*wave

  # generate sky flux:
  set sky = 200 + zero(wave)

  # off-slit background:
  set backgnd = 300 + zero(wave)

  # slit trace
  vlist Xn   0 100 
  vlist Yn   0   4
  spline create slit_trace_in Xn Yn

  # psf trace
  vlist Xn   0 25 50 75 100 
  vlist Yn   0  2  4  2   0
  spline create psf_trace_in Xn Yn

  $i = 0
  deimos mkalt out 31 -psf psf -object flux -sky sky -stilt 20.0 -profile profile -backgnd backgnd -trace slit_trace_in

  # tv -n 0 out 250 750
  # center 15 50 20
  # resize 850 2050
  # tvcolor rainbow
  # break

  memory check
  for i 0 1000  
    deimos mkalt out 31 -psf psf -object flux -sky sky -stilt 20.0 -profile profile -backgnd backgnd -trace slit_trace_in
  end
  memory check
end

