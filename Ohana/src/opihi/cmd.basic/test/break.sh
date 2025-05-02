
list tests
 test1
end

# test that break will halt operation
macro test1
 $PASS = 1
 break -auto off
 for i 0 10
   if ($i == 5)
     break
   end
 end
 if ($i != 5)
   $PASS = 0
 end
end

