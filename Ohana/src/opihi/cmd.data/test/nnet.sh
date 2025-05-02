
macro test.io

  nnet create t1 2 3 2
  
  # biases:
  vlist v1 -0.2 0.0 0.2
  vlist v2 -0.3 0.3

  # weights
  mcreate m1 2 3
  m1[0][0] =  0.2
  m1[1][0] = -0.2
  m1[0][1] = -0.6
  m1[1][1] =  0.5
  m1[0][2] = -0.2
  m1[1][2] =  0.2

  mcreate m2 3 2
  m2[0][0] = -0.6
  m2[1][0] = -0.4
  m2[2][0] = -0.5

  m2[0][1] = -0.2
  m2[1][1] = -0.5
  m2[2][1] =  0.1

  # set weights and biases
  nnet set t1 m1 v1 m2 v2

  nnet write t1 test.nnet.dat
  nnet read t2 test.nnet.dat

  nnet get t2 M1 V1 M2 V2

  set dM1 = M1 - m1
  set dM2 = M2 - m2
  set dV1 = V1 - v1
  set dV2 = V2 - v2

  stat dM1
  stat dM2
 
  vstat dV1
  vstat dV2
end

macro test.python
  if ($0 != 4)
    echo "USAGE: test.python (Nmini) (Nepoch) (eta)"
    break
  end

  # test I/O

  create x 0 100
  set y = dsin(x/10)
  write test.dat x y
  break

  local Nmini Nepoch eta
  $Nmini  = $1
  $Nepoch = $2
  $eta    = $3

  # create a nnet with just 2 inputs and 2 outputs
  nnet create t1 2 2

  # biases:
  vlist v1 0.5 0.0

  # weights
  mcreate m1 2 2
  m1[0][0] =  0.5
  m1[1][0] = -0.2
  m1[0][1] = -0.5
  m1[1][1] =  0.2

  # set weights and biases
  nnet set t1 m1 v1

  # generate an input set spanning the range -5 to +5

  create n 0 1000
  set x0 = 10*rnd(n) - 5.0
  set x1 = 10*rnd(n) - 5.0

  # get output vectors the these input vectors
  nnet apply t1 x0 x1 y0 y1

# -large-weight-initializer : revert to original implementation
# nnet create tn 2 2 
  nnet create tn 2 2 -large-weight-initializer

# -quadratic-cost : revert to original implementation
  nnet train tn x0 x1 y0 y1 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.0 -quadratic-cost -resid resid -result result
# nnet train tn x0 x1 y0 y1 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.0
# nnet train tn x0 x1 y0 y1 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 2.0

  nnet apply tn x0 x1 Y0 Y1

  set dy0 = y0 - Y0
  set dy1 = y1 - Y1

  vstat dy0
  $yp = $MAX
  $ym = $MIN

  vstat dy1
  $yp = max ($MAX , $yp)
  $ym = min ($MIN , $ym)
  $yp = 0.02; $ym = -0.02

  style -pt circle -sz 2; lim x0 $ym $yp; clear; box; plot -c blue x0 dy0; plot -c red x0 dy1
end

macro test

  nnet create t0 5 10 15 3
  nnet list

  nnet create t1 6 4 5
  nnet list

  nnet create t1 5 8
  nnet list

  # vlist v0 1 2 3 4 5
  vlist v1 1 2 3 4 5 6 7 8

  mcreate m1 5 8
  
  # generate a weight vector of length v1[]
  vlist my 9 8 7 6 5 4 3 2

  for i 0 5
    mset m1 my -y $i
    set my = my + $i
  end
  
  # set weights and biases
  nnet set t1 m1 v1

  # get weights and biases
  nnet get t1 M1 V1

  # compare
  set dv = v1 - V1
  vstat dv

  set dm = m1 - M1
  stat dm
  
end

macro test2

  nnet create t1 2 2
  nnet list

  # output = sigmoid (sum(weight[i][j] * input[i]) + bias[j])

  # biases of 
  vlist v1 0.8 0.2

  mcreate m1 2 2
  m1[0][0] =  0.5
  m1[1][0] =  0.1
  m1[0][1] = -0.2
  m1[1][1] =  0.8

  # set weights and biases
  nnet set t1 m1 v1

  vlist vin0 1 2
  vlist vin1 2 1

  # get weights and biases
  nnet apply t1 vin0 vin1 vout0 vout1
  
  vlist z0 {m1[0][0]*vin0[0] + m1[1][0]*vin1[0] + v1[0]} {m1[0][0]*vin0[1] + m1[1][0]*vin1[1] + v1[0]} 
  vlist z1 {m1[0][1]*vin0[0] + m1[1][1]*vin1[0] + v1[1]} {m1[0][1]*vin0[1] + m1[1][1]*vin1[1] + v1[1]}
  set t0 = 1 / (1 + exp(-1*z0))
  set t1 = 1 / (1 + exp(-1*z1))

  # compare
  set dt0 = t0 - vout0
  vstat dt0

  set dt1 = t1 - vout1
  vstat dt1
end

macro test3

  memory all

  nnet create t0 5 10 15 3
  nnet list

  nnet create t1 5 8
  nnet list

  # vlist v0 1 2 3 4 5
  vlist v1 1 2 3 4 5 6 7 8

  mcreate m1 5 8
  
  # generate a weight vector of length v1[]
  vlist my 9 8 7 6 5 4 3 2

  for i 0 5
    mset m1 my -y $i
    set my = my + $i
  end
  
  # set weights and biases
  nnet set t1 m1 v1

  # get weights and biases
  nnet get t1 M1 V1

  # compare
  set dv = v1 - V1
  vstat dv

  set dm = m1 - M1
  stat dm
  
  nnet delete t0
  nnet delete t1

  delete m1 M1 dm
  delete v1 V1 dv my

  vectors
  buffers

  memory all
end

macro test.simple
  if ($0 != 4)
    echo "USAGE: test.simple (Nmini) (Nepoch) (eta)"
    break
  end

  local Nmini Nepoch eta
  $Nmini  = $1
  $Nepoch = $2
  $eta    = $3

  # create a nnet with just 2 inputs and 2 outputs
  nnet create t1 2 2

  # biases:
  vlist v1 0.5 0.0

  # weights
  mcreate m1 2 2
  m1[0][0] =  0.5
  m1[1][0] = -0.2
  m1[0][1] = -0.5
  m1[1][1] =  0.2

  # set weights and biases
  nnet set t1 m1 v1

  # generate an input set spanning the range -5 to +5

  create n 0 1000
  set x0 = 10*rnd(n) - 5.0
  set x1 = 10*rnd(n) - 5.0

  # get output vectors the these input vectors
  nnet apply t1 x0 x1 y0 y1

# -large-weight-initializer : revert to original implementation
# nnet create tn 2 2 
  nnet create tn 2 2 -large-weight-initializer

# -quadratic-cost : revert to original implementation
  nnet train tn x0 x1 y0 y1 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.0 -quadratic-cost -resid resid -result result
# nnet train tn x0 x1 y0 y1 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.0
# nnet train tn x0 x1 y0 y1 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 2.0

  nnet apply tn x0 x1 Y0 Y1

  set dy0 = y0 - Y0
  set dy1 = y1 - Y1

  vstat dy0
  $yp = $MAX
  $ym = $MIN

  vstat dy1
  $yp = max ($MAX , $yp)
  $ym = min ($MIN , $ym)
  $yp = 0.02; $ym = -0.02

  style -pt circle -sz 2; lim x0 $ym $yp; clear; box; plot -c blue x0 dy0; plot -c red x0 dy1
end

macro test.bilevel
  if ($0 != 4)
    echo "USAGE: test.bilevel (Nmini) (Nepoch) (eta)"
    break
  end

  local Nmini Nepoch eta
  $Nmini  = $1
  $Nepoch = $2
  $eta    = $3

  # create a nnet with just 2 inputs and 2 outputs
  nnet create t1 3 4 3

  # biases:
  vlist v1 -0.2 0.0 0.2 0.4
  vlist v2 -0.5 0.0 0.5

  # weights
  mcreate m1 3 4
  m1[0][0] =  0.2
  m1[1][0] = -0.2
  m1[2][0] = -0.3

  m1[0][1] = -0.6
  m1[1][1] =  0.5
  m1[2][1] = -0.1

  m1[0][2] = -0.2
  m1[1][2] =  0.2
  m1[2][2] = -0.5

  m1[0][3] =  0.4
  m1[1][3] = -0.5
  m1[2][3] = -0.7

  mcreate m2 4 3
  m2[0][0] =  0.8
  m2[1][0] = -0.4
  m2[2][0] = -0.5
  m2[3][0] =  0.3

  m2[0][1] = -0.2
  m2[1][1] = -0.5
  m2[2][1] =  0.1
  m2[3][1] =  0.1

  m2[0][2] =  0.2
  m2[1][2] = -0.2
  m2[2][2] =  0.5
  m2[3][2] = -0.1

  # set weights and biases
  nnet set t1 m1 v1 m2 v2

  # generate an input set spanning the range -5 to +5

  create n 0 10000
  set x0 = 10*rnd(n) - 5.0
  set x1 = 10*rnd(n) - 5.0
  set x2 = 10*rnd(n) - 5.0

  # get output vectors the these input vectors
  nnet apply t1 x0 x1 x2 y0 y1 y2

# nnet create tn 3 4 3 -large-weight-initializer
  nnet create tn 3 4 3

# nnet train tn x0 x1 x2 y0 y1 y2 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.1
  nnet train tn x0 x1 x2 y0 y1 y2 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.0 -quadratic-cost -resid dS -result result

  nnet apply tn x0 x1 x2 Y0 Y1 Y2

  set dy0 = y0 - Y0
  set dy1 = y1 - Y1
  set dy2 = y2 - Y2

  vstat dy0
  $yp = $MAX
  $ym = $MIN

  vstat dy1
  $yp = max ($MAX , $yp)
  $ym = min ($MIN , $ym)

  vstat dy2
  $yp = max ($MAX , $yp)
  $ym = min ($MIN , $ym)
# $yp = 0.02; $ym = -0.02

  style -pt circle -sz 2; 
  lim x0 $ym $yp; clear; box; 
  plot -c blue  x0 dy0; 
  plot -c red   x0 dy1
  plot -c black x0 dy2
end

macro test.bilevel.small
  if ($0 != 4)
    echo "USAGE: test.bilevel (Nmini) (Nepoch) (eta)"
    break
  end

  local Nmini Nepoch eta
  $Nmini  = $1
  $Nepoch = $2
  $eta    = $3

  # create a nnet with just 2 inputs and 2 outputs
  nnet create t1 2 3 2

  # biases:
  vlist v1 -0.2 0.0 0.2
  vlist v2 -0.3 0.3

  # weights
  mcreate m1 2 3
  m1[0][0] =  0.2
  m1[1][0] = -0.2
  m1[0][1] = -0.6
  m1[1][1] =  0.5
  m1[0][2] = -0.2
  m1[1][2] =  0.2

  mcreate m2 3 2
  m2[0][0] = -0.6
  m2[1][0] = -0.4
  m2[2][0] = -0.5

  m2[0][1] = -0.2
  m2[1][1] = -0.5
  m2[2][1] =  0.1

  # set weights and biases
  nnet set t1 m1 v1 m2 v2

  # generate an input set spanning the range -5 to +5

  create n 0 1000
  set x0 = 10*rnd(n) - 5.0
  set x1 = 10*rnd(n) - 5.0

  # get output vectors the these input vectors
  nnet apply t1 x0 x1 y0 y1

# nnet create tn 3 4 3 -large-weight-initializer
  nnet create tn 2 3 2

# make the starting point close to the solution
  m2[0][0] = -0.58
# v2[1] = 0.25
  nnet set tn m1 v1 m2 v2

# nnet train tn x0 x1 x2 y0 y1 y2 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.1
# nnet train tn x0 x1 y0 y1 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.0 -quadratic-cost -resid dS -result result
  nnet train tn x0 x1 y0 y1 -Nmini $Nmini -Nepoch $Nepoch -eta $eta -lambda 0.0 -resid dS -result result

  nnet apply tn x0 x1 Y0 Y1

  set dy0 = y0 - Y0
  set dy1 = y1 - Y1

  vstat dy0
  $yp = $MAX
  $ym = $MIN

  vstat dy1
  $yp = max ($MAX , $yp)
  $ym = min ($MIN , $ym)

  dev -n 0 
  style -pt circle -sz 2; 
  lim x0 $ym $yp; clear; box; 
  plot -c blue  x0 dy0; 
  plot -c red   x0 dy1

  set n = ramp(dS)
  lim -n 1 n dS; clear; box; plot n dS
end

macro test.bilevel.grid.wt
  if ($0 != 4)
    echo "USAGE: test.bilevel.grid.wt (level) (x) (y)"
    break
  end

  local Level ix iy
  $Level  = $1
  $ix     = $2
  $iy     = $3

  # create a nnet with just 2 inputs and 2 outputs
  nnet create t1 2 3 2

  # biases:
  vlist v1 -0.2 0.0 0.2
  vlist v2 -0.3 0.3

  # weights
  mcreate m1 2 3
  m1[0][0] =  0.2
  m1[1][0] = -0.2
  m1[0][1] = -0.6
  m1[1][1] =  0.5
  m1[0][2] = -0.2
  m1[1][2] =  0.2

  mcreate m2 3 2
  m2[0][0] = -0.6
  m2[1][0] = -0.4
  m2[2][0] = -0.5

  m2[0][1] = -0.2
  m2[1][1] = -0.5
  m2[2][1] =  0.1

  # set weights and biases
  nnet set t1 m1 v1 m2 v2

  # generate an input set spanning the range -5 to +5

  create n 0 1000
  set x0 = 10*rnd(n) - 5.0
  set x1 = 10*rnd(n) - 5.0

  # get output vectors from these input vectors
  nnet apply t1 x0 x1 y0 y1

  # create a test nnet
  nnet create tn 2 3 2

  # 1D chi-square grid about truth
  
  set M1 = m1
  set M2 = m2
  set V1 = v1
  set V2 = v2

  # disturb one element to see cross terms
  M2[0][0] = -0.58

  $To = M$Level[$ix][$iy]

  delete -q value svec0 svec1 sigvec
  for frac 0.90 1.10 0.005
    M$Level[$ix][$iy] = $frac * $To

    nnet set tn M1 V1 M2 V2
    nnet apply tn x0 x1 Y0 Y1

    set dy0 = y0 - Y0
    set dy1 = y1 - Y1

    vstat -q dy0
    $S0 = $SIGMA  

    vstat -q dy1
    $S1 = $SIGMA

    concat {$frac * $To} value
    concat $S0 svec0
    concat $S1 svec1

    concat {sqrt($S0^2 + $S1^2)} sigvec
  end

  vstat -q sigvec

  lim -n 1 value -0.0001 $MAX; clear; box; 
  plot -c black value sigvec; 
  plot -c blue value svec0 -pt ocir -sz 2 ; 
  plot -c red value svec1 -pt ocir -sz 2
end

macro test.bilevel.grid.wt.2d
  if ($0 != 7)
    echo "USAGE: test.bilevel.grid.wt.2d (level) (x) (y) (level) (x) (y)"
    break
  end

  local LevelA ixA iyA LevelB ixB iyB
  $LevelA  = $1
  $ixA     = $2
  $iyA     = $3
  $LevelB  = $4
  $ixB     = $5
  $iyB     = $6

  # create a nnet with just 2 inputs and 2 outputs
  nnet create t1 2 3 2

  # biases:
  vlist v1 -0.2 0.0 0.2
  vlist v2 -0.3 0.3

  # weights
  mcreate m1 2 3
  m1[0][0] =  0.2
  m1[1][0] = -0.2
  m1[0][1] = -0.6
  m1[1][1] =  0.5
  m1[0][2] = -0.2
  m1[1][2] =  0.2

  mcreate m2 3 2
  m2[0][0] = -0.6
  m2[1][0] = -0.4
  m2[2][0] = -0.5

  m2[0][1] = -0.2
  m2[1][1] = -0.5
  m2[2][1] =  0.1

  # set weights and biases
  nnet set t1 m1 v1 m2 v2

  # generate an input set spanning the range -5 to +5

  create n 0 1000
  set x0 = 10*rnd(n) - 5.0
  set x1 = 10*rnd(n) - 5.0

  # get output vectors from these input vectors
  nnet apply t1 x0 x1 y0 y1

  # create a test nnet
  nnet create tn 2 3 2

  # 1D chi-square grid about truth
  
  set M1 = m1
  set M2 = m2
  set V1 = v1
  set V2 = v2

  # disturb one element to see cross terms
  M2[0][0] = -0.58

  $ToA = M$LevelA[$ixA][$iyA]
  $ToB = M$LevelB[$ixB][$iyB]

  delete -q valueA valueB sigvec

  mcreate sigbuf 41 41

  $ix = 0
  for fracA 0.90 1.10 0.005
    M$LevelA[$ixA][$iyA] = $fracA * $ToA

    $iy = 0   
    for fracB 0.90 1.10 0.005
   
      M$LevelB[$ixB][$iyB] = $fracB * $ToB

      nnet set tn M1 V1 M2 V2
      nnet apply tn x0 x1 Y0 Y1
      
      set dy0 = y0 - Y0
      set dy1 = y1 - Y1
      
      vstat -q dy0
      $S0 = $SIGMA  
      
      vstat -q dy1
      $S1 = $SIGMA
      
      $sigval = sqrt($S0^2 + $S1^2)
      concat {$fracA * $ToA} valueA
      concat {$fracB * $ToB} valueB
      concat $sigval sigvec

      sigbuf[$ix][$iy] = $sigval
      $iy ++
    end
    $ix ++
  end

  stat -q sigbuf
  tv -n tv sigbuf $MIN {$MAX - $MIN}
end
