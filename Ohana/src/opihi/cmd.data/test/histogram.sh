
list tests
 test1
 memtest1
end

# Test if histogram works
macro test1

 $PASS = 1

 local i

 # set data values at the i*0.1 + 0.05 values so round-off does not causs miscounts
 create x 0.05 10.05 0.1

 for i 45 55
  x[$i] = 4.5
 end

 # histogram bins are 0-0.1, 0.1-0.2, etc
 histogram x xhis 0 10 0.1

 if ((xhis[10] != 1) || (xhis[45] != 10))
  $PASS = 0
  echo "Value mismatch: xhis[10] xhis[45] (should be 1,10)"
 end

end


# Memory test
macro memtest1

 local i

 create xhis 0 10 0.1
 $i = 1

 memory stats
 $startmem = $memory:Ntotal

 for i 0 1000
  histogram x xhis 0 10 0.1
 end
  
 memory stats
 $endmem = $memory:Ntotal

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end
