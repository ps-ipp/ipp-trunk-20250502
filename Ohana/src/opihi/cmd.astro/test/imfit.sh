
macro trailfit.dither.XYLT
  if ($0 != 5)
    echo "USAGE: testfit.trail dX dY dL dT"
    break
  end

  $NTEST = 10000

  # truth values for the fake object to be fitted
  $Xg_tru   = 250
  $Yg_tru   = 250
  $Tg_tru   = 45.0
  $Wg_tru   = 2.0
  $Lg_tru   = 30.0
  $Zpk_tru  = 500.0
  $Sg_tru   = 0.0

  set.input.pars tru

  # generate a trail to be fitted
  mcreate zbuf 500 500
  set tbuf = zbuf + 100

  # insert an object with the parameters above
  imfit -func trail tbuf $Xg $Yg 50 50 -insert

  # make a normalized noise image
  mgaussdev sigma 500 500 0.0 1.0

  # generate the observed image (truth + noise)
  set obuf = tbuf + sigma*sqrt(tbuf)

  delete -q Xin Yin Lin Tin Xv Yv Tv Lv Zv Sv ChiSqV

  # run a bunch of tests, modifing the centroid guess by a random amount
  for i 0 $NTEST
    if ($i % 100 == 0) echo -no-return .

    # reset the guesses
    set.input.pars tru
    $Xg = $Xg_tru + $1*(rnd(0) - 0.5)
    $Yg = $Yg_tru + $2*(rnd(0) - 0.5)
    $Lg = $Lg_tru + max(3, $3*(rnd(0) - 0.5)); # too small L guess is silly
    $Tg = $Tg_tru + $4*(rnd(0) - 0.5)

    concat $Xg  Xin
    concat $Yg 	Yin
    concat $Lg  Lin
    concat $Tg 	Tin

    imfit -func trail obuf $Xg $Yg 50 50
    # imfit -func trail obuf $Xg $Yg 50 50 -save fit 

    concat $Xg  Xv 
    concat $Yg 	Yv 
    concat $Tg 	Tv 
    concat $Lg 	Lv 
    concat $Zpk	Zv
    concat $Sg 	Sv 
    concat $ChiSq ChiSqV
  end
end

macro trailfit.dither.LT
  if ($0 != 3)
    echo "USAGE: testfit.trail dL dT"
    break
  end

  $NTEST = 1000

  # truth values for the fake object to be fitted
  $Xg_tru   = 250
  $Yg_tru   = 250
  $Tg_tru   = 45.0
  $Wg_tru   = 2.0
  $Lg_tru   = 30.0
  $Zpk_tru  = 500.0
  $Sg_tru   = 0.0

  set.input.pars tru

  # generate a trail to be fitted
  mcreate zbuf 500 500
  set tbuf = zbuf + 100

  # insert an object with the parameters above
  imfit -func trail tbuf $Xg $Yg 50 50 -insert

  # make a normalized noise image
  mgaussdev sigma 500 500 0.0 1.0

  # generate the observed image (truth + noise)
  set obuf = tbuf + sigma*sqrt(tbuf)

  delete -q Xin Yin Xv Yv Tv Lv Zv Sv ChiSqV

  # run a bunch of tests, modifing the centroid guess by a random amount
  for i 0 $NTEST
    if ($i % 100 == 0) echo -no-return .

    # reset the guesses
    set.input.pars tru
    $Lg = $Lg_tru + $1*(rnd(0) - 0.5)
    $Tg = $Tg_tru + $2*(rnd(0) - 0.5)

    concat $Lg  Lin
    concat $Tg 	Tin

    imfit -func trail obuf $Xg $Yg 50 50
    # imfit -func trail obuf $Xg $Yg 50 50 -save fit 

    concat $Xg  Xv 
    concat $Yg 	Yv 
    concat $Tg 	Tv 
    concat $Lg 	Lv 
    concat $Zpk	Zv
    concat $Sg 	Sv 
    concat $ChiSq ChiSqV
  end
end

macro trailfit.dither.pos
  if ($0 != 3)
    echo "USAGE: testfit.trail dX dY"
    break
  end

  $NTEST = 1000

  # truth values for the fake object to be fitted
  $Xg_tru   = 250
  $Yg_tru   = 250
  $Tg_tru   = 0.0
  $Wg_tru   = 2.0
  $Lg_tru   = 10.0
  $Zpk_tru  = 500.0
  $Sg_tru   = 0.0

  set.input.pars tru

  # generate a trail to be fitted
  mcreate zbuf 500 500
  set tbuf = zbuf + 100

  # insert an object with the parameters above
  imfit -func trail tbuf $Xg $Yg 50 50 -insert

  # make a normalized noise image
  mgaussdev sigma 500 500 0.0 1.0

  # generate the observed image (truth + noise)
  set obuf = tbuf + sigma*sqrt(tbuf)

  delete -q Xin Yin Xv Yv Tv Lv Zv Sv ChiSqV

  # run a bunch of tests, modifing the centroid guess by a random amount
  for i 0 $NTEST
    if ($i % 100 == 0) echo -no-return .

    # reset the guesses
    set.input.pars tru
    $Xg = $Xg_tru + $1*(rnd(0) - 0.5)
    $Yg = $Yg_tru + $2*(rnd(0) - 0.5)

    concat $Xg  Xin
    concat $Yg 	Yin

    imfit -func trail obuf $Xg $Yg 50 50
    # imfit -func trail obuf $Xg $Yg 50 50 -save fit 

    concat $Xg  Xv 
    concat $Yg 	Yv 
    concat $Tg 	Tv 
    concat $Lg 	Lv 
    concat $Zpk	Zv
    concat $Sg 	Sv 
    concat $ChiSq ChiSqV
  end
end

macro testfit.trail
  if ($0 != 3)
    echo "USAGE: testfit.trail dX dY"
    break
  end

  $myfunc = trail

  mcreate zbuf 500 500
  set tbuf = zbuf + 100

  # note the SXg, SYg are FWHM, not sigma values
  $Xg_in   = 250
  $Yg_in   = 250
  $Tg_in   = 0.0
  $Wg_in   = 2.0
  $Lg_in   = 10.0
  $Zpk_in  = 500.0
  $Sg_in   = 0.0

  $Xg      = $Xg_in   
  $Yg      = $Yg_in   
  $Tg      = $Tg_in  
  $Wg      = $Wg_in  
  $Lg      = $Lg_in 
  $Zpk     = $Zpk_in  
  $Sg      = $Sg_in   

  # insert an object with the parameters above
  imfit -func $myfunc tbuf $Xg $Yg 50 50 -insert

  # make a normalized noise image
  mgaussdev sigma 500 500 0.0 1.0

  # generate the observed image (truth + noise)
  set obuf = tbuf + sigma*sqrt(tbuf)

  # make the guess wrong by a small amount
  $Xg += $1
  $Yg += $2
  imfit -func $myfunc obuf $Xg $Yg 50 50 -save fit -v

  echo $Xg   : $Xg_in   {$Xg   - $Xg_in  }
  echo $Yg   : $Yg_in   {$Yg   - $Yg_in  }
  echo $Tg   : $Tg_in   {$Tg   - $Tg_in  }
  echo $Wg   : $Wg_in   {$Wg   - $Wg_in  }
  echo $Lg   : $Lg_in   {$Lg   - $Lg_in  }
  echo $Zpk  : $Zpk_in  {$Zpk  - $Zpk_in }
  echo $Sg   : $Sg_in   {$Sg   - $Sg_in  }
end

macro testfit.func
  if ($0 != 2)
    echo "USAGE: testfit.func (func)"
    break
  end

  $myfunc = $1 

  $NX = 101
  $NY = 101
  mcreate zbuf $NX $NY
  set tbuf = zero(zbuf)

  # note the SXg, SYg are FWHM, not sigma values
  $Xg_in   = 50
  $Yg_in   = 50
  $SXg_in  = 4.0
  $SYg_in  = 4.0
  $SXYg_in = 0.0
  $Zpk_in  = 500.0
  $Sg_in   = 0.0
  $Sr_in   = 1.0
  $Npow_in = 2.25

  $Xg      = $Xg_in   
  $Yg      = $Yg_in   
  $SXg     = $SXg_in  
  $SYg     = $SYg_in  
  $SXYg    = $SXYg_in 
  $Zpk     = $Zpk_in  
  $Sg      = $Sg_in   
  $Sr      = $Sr_in   
  $Npow    = $Npow_in

  # insert an object with the parameters above
  imfit -func $myfunc tbuf $Xg $Yg 50 50 -insert

  mgaussdev sigma $NX $NY 0.0 1.0

  set noise = sigma*sqrt(tbuf)
  set obuf = tbuf + noise  

  imfit -func $myfunc obuf $Xg $Yg 50 50 -save fit -v

  echo $Xg   : $Xg_in   {$Xg   - $Xg_in   }
  echo $Yg   : $Yg_in   {$Yg   - $Yg_in   }
  echo $SXg  : $SXg_in  {$SXg  - $SXg_in  }
  echo $SYg  : $SYg_in  {$SYg  - $SYg_in  }
  echo $SXYg : $SXYg_in {$SXYg - $SXYg_in }
  echo $Zpk  : $Zpk_in  {$Zpk  - $Zpk_in  }
  echo $Sg   : $Sg_in   {$Sg   - $Sg_in   }
  echo $Sr   : $Sr_in   {$Sr   - $Sr_in   }
  echo $Npow : $Npow_in {$Npow - $Npow_in }
end

macro testfit.fgauss.pol
  if ($0 != 1)
    echo "USAGE: testfit.fgauss.pol"
    break
  end

  $myfunc = fgauss

  mcreate zbuf 101 101
  set tbuf_o = zero(zbuf)
  set tbuf_p = zero(zbuf)

  # note the SXg, SYg are FWHM, not sigma values
  $Xg_in   = 50
  $Yg_in   = 50
  $SXg_in  = 4.0
  $SYg_in  = 4.0
  $SXYg_in = 0.0
  $Zpk_in  = 500.0
  $Sg_in   = 0.0
  $Sr_in   = 1.0
  $Npow_in = 2.25

  $Xg      = $Xg_in   
  $Yg      = $Yg_in   
  $SXg     = $SXg_in  
  $SYg     = $SYg_in  
  $SXYg    = $SXYg_in 
  $Zpk     = $Zpk_in  
  $Sg      = $Sg_in   
  $Sr      = $Sr_in   
  $Npow    = $Npow_in

  # insert an object with the parameters above
  imfit -func $myfunc     tbuf_o $Xg $Yg 50 50 -insert

  # insert an object with the parameters using the -pol version
  imfit -func $myfunc-pol tbuf_p $Xg $Yg 50 50 -insert

  set dt = tbuf_o - tbuf_p
  stat dt

  tv -ch 1 tbuf_o 0 $Zpk_in
  tv -ch 2 tbuf_p 0 $Zpk_in
  tv -ch 3 dt {-0.1*$Zpk_in} {0.2*$Zpk_in}
end

macro set.input.pars
  if ($0 != 2)
    echo "USAGE: set.input.pars (name)"
    break
  end

  # copy to the 
  $Xg      = $Xg_$1   
  $Yg      = $Yg_$1   
  $Tg      = $Tg_$1  
  $Wg      = $Wg_$1  
  $Lg      = $Lg_$1 
  $Zpk     = $Zpk_$1  
  $Sg      = $Sg_$1   
end

    # echo $Xg   : $Xg_in   {$Xg   - $Xg_in  }
    # echo $Yg   : $Yg_in   {$Yg   - $Yg_in  }
    # echo $Tg   : $Tg_in   {$Tg   - $Tg_in  }
    # echo $Wg   : $Wg_in   {$Wg   - $Wg_in  }
    # echo $Lg   : $Lg_in   {$Lg   - $Lg_in  }
    # echo $Zpk  : $Zpk_in  {$Zpk  - $Zpk_in }
    # echo $Sg   : $Sg_in   {$Sg   - $Sg_in  }
