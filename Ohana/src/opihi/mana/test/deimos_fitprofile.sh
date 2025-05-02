
macro memtest1
  # generate slit profile:
  create xprof 0 17
  $Xpmin = 3
  $Xpmax = 13
  set profile_left  = (xprof <= $Xpmin) ? exp(-0.5*(xprof - $Xpmin)^2/1.0^2) : zero(xprof)
  set profile_right = (xprof >= $Xpmax) ? exp(-0.5*(xprof - $Xpmax)^2/1.0^2) : zero(xprof)
  set profile_mid   = (xprof > $Xpmin) && (xprof < $Xpmax) ? zero(xprof) + 1 : zero(xprof)
  set fprofile = profile_left + profile_mid + profile_right
  set wprofile = 0.02*fprofile

  $i = 0
  deimos fitprofile -q xprof fprofile wprofile $Xpmin $Xpmax

  # tv -n 0 out 250 750
  # center 15 50 20
  # resize 850 2050
  # tvcolor rainbow
  # break

  memory check
  for i 0 1000  
    deimos fitprofile -q xprof fprofile wprofile $Xpmin $Xpmax
  end
  memory check
end

