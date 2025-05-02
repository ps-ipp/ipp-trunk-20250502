
# make a plot with labels, ticks, and textlines 
macro testplot

  section a 0.0 0.0 0.5 0.5
  box
  label -fn times 18 -x xbar
  textline -frac 0.3 0.3 foobar -fn times 12
  textline -frac 0.1 0.1 "" -fn times 24 

  section b 0.0 0.5 0.5 0.5
  box
  label -fn times 18 -x xbar
  textline -frac 0.3 0.3 foobar -fn times 12
  textline -frac 0.1 0.1 "" -fn times 24 

  section c 0.5 0.0 0.5 0.5
  box
  label -fn times 18 -x xbar
  textline -frac 0.3 0.3 foobar -fn times 12
  textline -frac 0.1 0.1 "" -fn times 24 
end

# make a plot with labels, ticks, and textlines 
macro testplot2

  label -fn times 24
  box
  label -fn helvetica 18 -x xbar
  textline -frac 0.3 0.3 foobar -fn courier 12
  textline -frac 0.1 0.1 "" -fn times 24 
end

