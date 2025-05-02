
macro jobs.rates
 clear -s

 section a 0.0 0.0 1.0 0.5
 data jobs.stop.dat
 read t 6
 set T = t*(t > 30) + (t+60)*(t<30)
 create n 0 t[]
 lim T n; box; plot T n
 subset tf = T if (T > 70)
 subset nf = n if (T > 70)
 fit tf nf 1
 
 section b 0.0 0.5 1.0 0.5
 data jobs.run.dat
 read t 6
 set T = t*(t > 30) + (t+60)*(t<30)
 create n 0 t[]
 lim T n; box; plot T n
 subset tf = T if (T > 70)
 subset nf = n if (T > 70)
 fit tf nf 1
end
