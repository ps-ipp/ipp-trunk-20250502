
# generate a natural cubic spline for a set of knots
macro test_01
 
  vlist xKnots  0 5 10 15 20
  vlist yKnots -3 6  5  8  2

  spline create t01 xKnots yKnots

  create x xKnots[0] xKnots[-1] 0.1
  spline apply t01 x y

  lim x y; clear; box; plot -x line -c black x y; plot -pt cir -sz 2 -c red xKnots yKnots
end  

# generate a natural cubic spline for a set of knots
macro test_02
 
  vlist xKnots  0 5 10 15 20
  vlist yKnots -3 6  5  8  2

  # force lower-bound slope = 0.0
  spline create t02 xKnots yKnots -dyLower 0.0

  create x xKnots[0] xKnots[-1] 0.1
  spline apply t02 x y

  lim x y; clear; box; plot -x line -c black x y; plot -pt cir -sz 2 -c red xKnots yKnots
end  

# generate a natural cubic spline for a set of knots
macro test_03
 
  vlist xKnots  0 5 10 15 20
  vlist yKnots -3 6  5  8  2

  # force lower-bound slope = 0.0
  spline create t03 xKnots yKnots -dyUpper 0.0

  create x xKnots[0] xKnots[-1] 0.1
  spline apply t03 x y

  lim x y; clear; box; plot -x line -c black x y; plot -pt cir -sz 2 -c red xKnots yKnots
end  

# generate a natural cubic spline for a set of knots
macro test_04
 
  vlist xKnots  0 5 10 15 20
  vlist yKnots -3 6  5  8  2

  # force lower-bound & upper-bound slopes = 0.0
  spline create t04 xKnots yKnots -dyLower 0.0 -dyUpper 0.0

  create x xKnots[0] xKnots[-1] 0.1
  spline apply t04 x y

  lim x y; clear; box; plot -x line -c black x y; plot -pt cir -sz 2 -c red xKnots yKnots
end  

