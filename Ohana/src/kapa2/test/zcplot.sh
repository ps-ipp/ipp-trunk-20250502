
macro test.zcplot
  create x 0 10
  lim x x; clear; box; 
  for i 0 11
    zcplot x {x + 0.25*$i} x 0 10 -pt $i -sz 3
  end
  tvcolors rainbow

  png -name test.png  
  ps -name test.ps
end
