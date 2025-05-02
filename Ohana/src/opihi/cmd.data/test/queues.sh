
list tests
 test1
 test2
 test3
 test4
 test4.1
 test5
 test5.1
 testmem1
 testmem2
 testmem3
 testmem4.0
 testmem4.1
 testmem4.2
 testmem4.3
 testmem5.0
 testmem5.1
end

# test queueinit
macro test1
 $PASS = 1
 queueinit dummy
 queuesize dummy -var N

 if ($N != 0)
   $PASS = 0
 end
end

# test queueinit memory
macro testmem1
 $PASS = 1
 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   queueinit dummy
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# test queuesize
macro test2
 $PASS = 1
 queueinit dummy
 queuepush dummy foobar
 queuesize dummy -var N

 if ($N != 1)
   $PASS = 0
 end
end

# test queuesize memory
macro testmem2
 $PASS = 1
 queueinit dummy
 queuepush dummy foobar

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   queuesize dummy -var N
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# test queuedelete
macro test3
 $PASS = 1
 queueinit dummy
 queuepush dummy foobar
 queuepush dummy foobar
 queuepush dummy foobar

 queuedelete dummy
 queuepush dummy foobar
 queuesize dummy -var N

 if ($N != 1)
   $PASS = 0
 end
end

# test queuedelete memory
macro testmem3
 $PASS = 1
 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   queueinit dummy
   queuepush dummy foobar
   queuedelete dummy
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# test queuepush / queuepop
macro test4
 $PASS = 1
 queueinit dummy
 queuepush dummy foobar
 queuepop  dummy -var N

 if ("$N" != "foobar")
   $PASS = 0
 end
end

# test queuepush / queuepop
macro test4.1
 $PASS = 1
 queueinit dummy
 queuepush dummy foo1
 queuepush dummy foo2
 queuepush dummy foo3

 queuepop  dummy -var N
 if ("$N" != "foo1")
   $PASS = 0
 end
 queuepop  dummy -var N
 if ("$N" != "foo2")
   $PASS = 0
 end
 queuepop  dummy -var N
 if ("$N" != "foo3")
   $PASS = 0
 end
 queuesize dummy -var N
 if ($N != 0)
   $PASS = 0
 end
end

# test queuepush / queuepop memory
macro testmem4.0
 $PASS = 1
 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   queueinit dummy
   queuepush dummy foobar
   queuepop dummy -var N
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# test queuepush / queuepop memory
macro testmem4.1
 $PASS = 1
 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   queueinit dummy
   queuepush dummy foobar
   queuepop dummy
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# test queuepush / queuepop memory
macro testmem4.2
 $PASS = 1
 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   queueinit dummy
   queuepush dummy foobar
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# test queuepush / queuepop memory
macro testmem4.3
 $PASS = 1
 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 queueinit dummy

 output /dev/null
 for i 0 10000
   queuepush dummy foobar
   queuepop dummy
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# test queuepush / queuepop with keys
macro test5
 $PASS = 1
 queueinit dummy
 queuepush dummy "test 1 word"
 queuepush dummy "test 2 word"
 queuepush dummy "test 3 word"
 queuepop  dummy -var N -key 1 2

 if ("$N" != "test 2 word")
   $PASS = 0
 end
end

# test queuepush / queuepop with keys
macro test5.1
 $PASS = 1
 queueinit dummy
 queuepush dummy "test 1 word"
 queuepush dummy "test 2 word"
 queuepush dummy "bird 2 word"
 queuepop  dummy -var N -key 0:1 test:2

 if ("$N" != "test 2 word")
   $PASS = 0
 end
end

# memory test for queuepush / queuepop with keys
macro testmem5.0
 $PASS = 1
 queueinit dummy

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   queuepush dummy "test 1 word"
   queuepush dummy "test 2 word"
   queuepush dummy "test 3 word"
   queuepop dummy -var N -key 1 2
   queuepop dummy -var N -key 1 1
   queuepop dummy -var N -key 1 3
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
   queuelist
 end
end

# memory test for queuepush / queuepop with keys
macro testmem5.1
 $PASS = 1
 queueinit dummy

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   queuepush dummy "test 1 word"
   queuepop dummy -var N -key 1 1
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end
