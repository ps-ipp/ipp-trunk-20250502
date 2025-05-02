
list tests
 test1
end

# test that the file test function works at all
macro test1
 $PASS = 1
 exec touch foo.test
 file foo.test exists
 if ($exists != 1)
   $PASS = 0
 end
 exec rm -f foo.test
 file foo.test exists
 if ($exists != 0)
   $PASS = 0
 end
end
