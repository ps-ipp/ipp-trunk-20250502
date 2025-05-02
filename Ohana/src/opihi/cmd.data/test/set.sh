
macro memtest1

 create x -10 10 0.1
 set y = zero(x)
 $i = 0

 memory check
 for i 0 1000
   set y = exp(x)
 end
 memory check
   
end

macro memtest2

 create x -10 10 0.1
 set y = zero(x)
 $i = 0

 memory check
 for i 0 1000
   set y = (x < 0) ? x : exp(x)
 end
 memory check
   
end

macro memtest3

 create x -10 10 0.1
 set y = zero(x)
 $i = 0

 memory check
 for i 0 1000
   set y = (x < 5 + 2*$i) ? x : exp(x)
 end
 memory check
   
end

