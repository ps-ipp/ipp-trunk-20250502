
list tests
 test1
 test2
end

# test that a shell function works at all
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

# test that the shell status is returned
macro test2
 $PASS = 1
 exec touch foo.test
 exec ls foo.test >& /dev/null
 if ($STATUS != 1)
   $PASS = 0
 end
 exec rm -f foo.test
 exec ls foo.test >& /dev/null
 if ($STATUS != 0)
   $PASS = 0
 end
end
