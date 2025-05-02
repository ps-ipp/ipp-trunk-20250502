
macro go
  $SIG = 0.2
  create x 0 15
  set y = 2 + zero(x)
  gaussdev yoff y[] 0.0 $SIG
  set dy = $SIG + zero(y)
  set yobs = y + yoff

  # add in an outlier
  yobs[1] = 20
  yobs[6] = -5
  yobs[12] = 10

  fit1d x yobs 0 -dy dy
  applyfit1d x yfit1

  fit1d_irls x yobs 0 -dy dy
  applyfit1d x yfit2

  subset  xs =    x if ((x != 1) && (x != 6) && (x != 12))
  subset  ys = yobs if ((x != 1) && (x != 6) && (x != 12))
  subset dys =   dy if ((x != 1) && (x != 6) && (x != 12))
  subset ys2 = yfit2 if ((x != 1) && (x != 6) && (x != 12))

  fit1d xs ys 0 -dy dys
  applyfit1d x yfit3
  subset ys3 = yfit3 if ((x != 1) && (x != 6) && (x != 12))

  lim x yobs; clear; box; plot x yobs -dy dy; plot x yfit1 -c red -x 0; plot x yfit2 -c blue -x 0; plot x yfit3 -c green -x 0

  set dy1 = ys - ys2
  set dy2 = ys - ys3

  vstat dy1
  vstat dy2
end


macro go2
  $SIG = 0.5
  create x 0 15
  set y = 2 + 3*x
  gaussdev yoff y[] 0.0 $SIG
  set dy = $SIG + zero(y)
  set yobs = y + yoff

  # add in an outlier
  yobs[1] = 20

  fit1d x yobs 1 -dy dy
  applyfit1d x yfit1

  delete c0v c1v dc0v dc1v
  for i 0 100
    fit1d_irls -q x yobs 1 -dy dy
    concat $C0 c0v
    concat $C1 c1v
    concat $dC0 dc0v
    concat $dC1 dc1v
  end
  vstat -q c0v; set C0 = $MEAN; set C0sigma = $SIGMA
  vstat -q c1v; set C1 = $MEAN; set C1sigma = $SIGMA
  vstat -q dc0v; set dC0 = $MEAN
  vstat -q dc1v; set dC1 = $MEAN

  fprint "y = %f x^0 %f x^1" $C0 $C1
  fprint "    %f     %f " $dC0 $dC1
  fprint "    %f     %f " $C0sigma $C1sigma

  applyfit1d x yfit2

  subset  xs =    x if (x != 1)
  subset  ys = yobs if (x != 1)
  subset dys =   dy if (x != 1)

  fit1d xs ys 1 -dy dys
  applyfit1d x yfit3

  lim x yobs; clear; box; plot x yobs -dy dy; plot x yfit1 -c red -x 0; plot x yfit2 -c blue -x 0; plot x yfit3 -c green -x 0
 
  set dy1 = yobs - yfit1
  set dy2 = yobs - yfit2

  vstat dy1
  vstat dy2
end

macro go1
  $SIG = 0.1
  create x 0 15
  set y = 2 + 3*x
  gaussdev yoff y[] 0.0 $SIG
  set dy = $SIG*(2+x) + zero(y)
  set yobs = y + yoff*(2+x)

  # add in an outlier
  yobs[1] = 20

  fit1d x yobs 1 -dy dy
  applyfit1d x yfit1

  fit1d_irls x yobs 1 -dy dy
  applyfit1d x yfit2

  subset  xs =    x if (x != 1)
  subset  ys = yobs if (x != 1)
  subset dys =   dy if (x != 1)

  fit1d xs ys 1 -dy dys
  applyfit1d x yfit3

  lim x yobs; clear; box; plot x yobs -dy dy; plot x yfit1 -c red -x 0; plot x yfit2 -c blue -x 0; plot x yfit3 -c green -x 0
 
  set dy1 = yobs - yfit1
  set dy2 = yobs - yfit2

  vstat dy1
  vstat dy2
end

macro go3
  $SIG = 0.05
  create x 0 15
  set y = 2 + 3*x
  gaussdev yoff y[] 0.0 $SIG
  set dy = $SIG*y + zero(y)
  set yobs = y + yoff*y

  # add in an outlier
  yobs[1] = 20

  fit1d x yobs 1 -dy dy
  applyfit1d x yfit1

  fit1d_irls x yobs 1 -dy dy
  applyfit1d x yfit2

  subset  xs =    x if (x != 1)
  subset  ys = yobs if (x != 1)
  subset dys =   dy if (x != 1)

  fit1d xs ys 1 -dy dys
  applyfit1d x yfit3

  lim x yobs; clear; box; plot x yobs -dy dy; plot x yfit1 -c red -x 0; plot x yfit2 -c blue -x 0; plot x yfit3 -c green -x 0
 
  set dy1 = yobs - yfit1
  set dy2 = yobs - yfit2

  vstat dy1
  vstat dy2
end

