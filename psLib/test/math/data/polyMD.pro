
list times
0.0
10.0
30.0
100.0
300.0
end

macro mkfake

 delete -q exptime

 create n 0 1000
 for i 0 $times:n
   set tmp = $times:$i + zero(n)
   concat tmp exptime
 end

 set temp = -85 + 15*rnd(exptime); 

 $D0 = 1000.0
 $D1 = 5.0
 $D2 = 0.025
 $D3 = 0.005

 set f = $D0 + $D1*exptime + $D2*exptime*temp + $D3*exptime*temp^2; 

 set f0 = $D0 + $D2*temp + $D3*temp^2; 
 set f1 = $D0 + $D1*exptime + $D2*exptime*temp
 set f2 = $D0 + $D1*exptime + $D3*exptime*temp^2; 

 set ord1 = exptime
 set ord2 = exptime * temp
 set ord3 = exptime * temp^2

 write polyMD.dat exptime temp ord1 ord2 ord3 f
end

macro test.fit2d 
  fit2d exptime temp f 3
end

macro test.fit3d 
  fit3d ord1 ord2 ord3 f 1
end

macro testfit
 $N = 100
 create x 0 $N
 set y = $N*rnd(x)
 set z = $N*rnd(x)
 set f = 10 + 2*x - 3*y +5*z
 fit3d x y z f 1 -v
end

macro testfit2
 $N = 100
 create x 0 $N
 set y = $N*rnd(x)
 set z = $N*rnd(x)
 set f = 10 + 2*x - 0.1*x^2 + 0.3*x*y - 3*y + 5*z - 20*z^2
 fit3d x y z f 2 -v
end
