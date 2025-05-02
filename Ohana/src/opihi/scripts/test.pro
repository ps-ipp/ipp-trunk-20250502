
macro testgauss
 create x -20 20 0.1
 set dy = zero(x) + 0.03
 set X = x - $1
 set y = $3*exp(-0.5*(X^2/$2^2)) + $4 + 0.1*(rnd(X) - 0.5)
# set y = $3*exp(-0.5*(X^2/$2^2)) + $4
 lim x y; clear; box; plot x y -dy dy -x 2 -pt 2

 $C0 = $1+5
 $C1 = $2-10
 $C2 = $3
 $C3 = $4

 vgauss x y dy yf
end

macro testpoor
 create x -20 20 0.1
 set dy = zero(x) + 0.03
 set X = x - $1
 set y = 3 + zero(x) + 0.1*(rnd(X) - 0.5)
# set y = (x + 20)/10 + $4 + 0.1*(rnd(X) - 0.5)
# set y = $3*exp(-0.5*(X^2/$2^2)) + $4
 lim x y; clear; box; plot x y -dy dy -x 2 -pt 2

 $C0 = $1
 $C1 = $2
 $C2 = $3
 $C3 = $4

 vgauss x y dy yf
end

macro testfit
 create x 0 1000
 set y = 3 + 5*x
 set dy = 10*(rnd(x) - 0.5)
 set yr = y + dy
 fit x yr 1
 echo $Cn, $Cnv
 echo $dC

 yr[100] = yr[100] + 50
 yr[200] = yr[200] - 50
 yr[300] = yr[300] + 100
 yr[400] = yr[400] - 500
 yr[500] = yr[500] + 30

 fit x yr 1
 echo $Cn, $Cnv
 echo $dC

 fit x yr 1 -clip 3 3
 echo $Cn, $Cnv
 echo $dC
end
# expected output:
# y = 3.013781 x^0 5.000005 x^1
#     0.063198     0.000110
# 1, 1000
# 2.91071543189
# y = 2.495040 x^0 5.000303 x^1
#     0.063198     0.000110
# 1, 1000
# 16.5928919554
# y = 3.021974 x^0 4.999991 x^1
#     0.063419     0.000110
# 1, 995
# 2.90820510001

macro testloops
 for i 0 100
  if ($i < 20)
   continue
  end
  if ($i > 40)
   break
  end
  echo $i
 end
end  

# test math parsing
macro testvars
 echo testing variable assignment
 $a = 1
 echo $a is 1
 $b = 5*3 + $a
 echo $b is 16

 $a = `ls`
 echo $a
 exec ls

 echo testing in-line math
 $a = 1
 echo {1 + 1} is 2
 echo {2^3} is 8
 echo {$a + 2} is 3

 echo testing vector assignment
 create x 0 10
 set y = x^2
 echo x[2] is 2
 echo y[2] is 4
 lim x y; clear; box; plot x y
 label -x "x axis" -y "&s y axis"
end

macro testspeed
 exec date
 create x 0 100000
 for i 0 5000
  set y = 3*x + $i
 end
 exec date
end

macro testmemory.buffers
 mcreate a 2048 2048
 exec date
 for i 0 500
  set b = 3*a + 6
 end
 exec date
end

macro testmemory.macros
 exec date
 for i 0 1000
   for j 0 1000
    $N = $N + 1
   end
 end
 exec date
end

macro sample
 $N = 0
 for j 0 1000000
  $N = 100
 end
end
