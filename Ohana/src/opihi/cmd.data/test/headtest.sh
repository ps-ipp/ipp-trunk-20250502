
macro init.test
 create x 0 10
 set y  = x^2
 mcreate blank 0 0 
 keyword blank TE0 -wd 56
 keyword blank TE1 -w "TEST LINE"
 keyword blank TE2 -wf 5.12342341
 keyword blank TE3 -wb T
 keyword blank TE4 -wc "not sure what do say"
 keyword blank TE4 -wf 4.5
 keyword blank TE5 -ws "this is a long comment"
 keyword blank TE2 -wc "this is a short comment"
 write -fits DATA test2.fits x y -header blank
end
