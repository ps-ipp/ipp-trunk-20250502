
list tests
end

macro test1
 $PASS = 1
 break -auto off

 create t 0 4096 1.0
 set f = dsin(20*t)

 fft1d f 0 to Frn Fin

 clear
 section a 0.0 0.0 1.0 0.5
 lim t Fro; box; plot -pt 7 -c blue t Fro; plot -pt 2 -c red t Frn
end
