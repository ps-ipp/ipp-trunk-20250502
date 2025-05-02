
macro memtest1

 # preset variables so memory usage does not change
 $PMIN = -10
 $PMAX = 9.9
 $MIN = -10
 $MAX = 9.9
 $MEDIAN = -0.1097
 $MEAN = -0.05
 $MODE = -10
 $TOTAL = -10
 $NPIX = 200
 $NPTS = 200
 $NUSED = 200
 $SIGMA = 5.7879184514

 create x -10 10 0.1
 $i = 0

 memory check
 for i 0 1000
   vstat -q x
 end
 memory check
   
end

