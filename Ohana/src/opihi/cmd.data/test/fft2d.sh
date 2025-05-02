
list tests
# test1
end

macro test1
 $PASS = 1
 break -auto off

 delete x y t f Frn Fin Fro Fio dfi dfr
 mcreate t 2048 2048
 set x = xramp(t)
 set y = yramp(t)

 # set f = dsin(3*x)*dcos(5*y)
 set f = exp(-0.5*((x-1024)^2 + (y-1024)^2)*0.01)

 date; fft2d f 0 to Frn Fin; date

 date; fft2dold f 0 to Fro Fio; date

 set dfr = Frn - Fro
 set dfi = Fin - Fio

 stats dfr
 stats dfi
end
