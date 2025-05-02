
macro memtest1
  # make a delta-function profile
  create x 0 11
  set yg = exp(-0.25*(x - 5)^2)

  # flux vs wavelength is flat & short 
  create w 0 100
  set f = zero(w) + 1

  delete -q Xn Yn
  vlist Xn   0 100
  vlist Yn   0   2

  spline create trace Xn Yn
  spline apply  trace w  dX
  $i = 0

  # run once to ensure the output is create
  deimos mkslit yg trace trace outbuff -flux f
  mgaussdev noise outbuff[][0] outbuff[0][] 0.0 0.2

  set obs = 2*outbuff + noise
  deimos fitslit obs outbuff outf outs

  memory check
  for i 0 1000  
    deimos mkslit yg trace trace outbuff -flux f
  end
  memory check
end
