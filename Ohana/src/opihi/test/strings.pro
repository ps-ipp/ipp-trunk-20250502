
macro test1
 $test1 = 01234567890.01234567890.1234567890
 $test2 = $test1 + $test1
 $test3 = $test2 and $test2
 $test4 = $test3 or $test3
 $test5 = $test4 with $test4

 echo strlen test1; strlen "$test1" 
 echo strlen test2; strlen "$test2"
 echo strlen test3; strlen "$test3"
 echo strlen test4; strlen "$test4"
 echo strlen test5; strlen "$test5"
end

macro test2
 $test1 = 01234567890.01234567890.1234567890
 appendup test1 test2
 appendup test2 test3
 appendup test3 test4
 appendup test4 test5

 echo strlen test1; strlen "$test1" 
 echo strlen test2; strlen "$test2"
 echo strlen test3; strlen "$test3"
 echo strlen test4; strlen "$test4"
 echo strlen test5; strlen "$test5"
end
 
macro appendup
  $$2 = $$1 and $$1
end
