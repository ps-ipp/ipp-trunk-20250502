
list tests
 test1
 test2
end

# test that cd will go into a new directory
macro test1
 $PASS = 1
 exec mkdir test.dir
 output /dev/null
 cd test.dir
 exec touch foo.test
 cd ..
 output stdout
 file test.dir/foo.test exists
 if ($exists != 1)
   $PASS = 0
 end
 exec rm -f test.dir/foo.test
 exec rmdir test.dir
end

# test that pwd output is correct
macro test2
 $PASS = 1
 exec touch foo.test
 pwd -var testdir
 file $testdir\/foo.test exists
 if ($exists != 1)
  $PASS = 0
 end
 exec rm foo.test
end
