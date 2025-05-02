
list tests
 test1
 test2
 test3
 test4
 test5
 test6
end

# basic read
macro test1
 
 $PASS = 1

 data read.t1.dat
 read t1 1 t2 2 t3 3

 if (t1[0] != 1) set PASS = 0
 if (t1[1] != 2) set PASS = 0
 if (t1[2] != 4) set PASS = 0

 if (t2[0] != 5) set PASS = 0
 if (t2[1] != 3) set PASS = 0
 if (t2[2] != 5) set PASS = 0

 if (t3[0] != 8) set PASS = 0
 if (t3[1] != 9) set PASS = 0
 if (t3[2] != 7) set PASS = 0
end

# basic read with int types
macro test2
 
 $PASS = 1

 data read.t1.dat
 read t1:int 1 t2:int 2 t3:int 3

 vtype t1 -var type; if ("$type" != "INT") set PASS = 0
 vtype t2 -var type; if ("$type" != "INT") set PASS = 0
 vtype t3 -var type; if ("$type" != "INT") set PASS = 0

 # note: no way to verify the mode of a vector
 if ($VERBOSE >= 2) vectors

 if (t1[0] != 1) set PASS = 0
 if (t1[1] != 2) set PASS = 0
 if (t1[2] != 4) set PASS = 0

 if (t2[0] != 5) set PASS = 0
 if (t2[1] != 3) set PASS = 0
 if (t2[2] != 5) set PASS = 0

 if (t3[0] != 8) set PASS = 0
 if (t3[1] != 9) set PASS = 0
 if (t3[2] != 7) set PASS = 0
end

# basic read with comments
macro test3
 
 $PASS = 1

 data read.t2.dat
 read t1 1 t2 2 t3 3

 if (t1[0] != 1) set PASS = 0
 if (t1[1] != 2) set PASS = 0
 if (t1[2] != 4) set PASS = 0

 if (t2[0] != 5) set PASS = 0
 if (t2[1] != 3) set PASS = 0
 if (t2[2] != 5) set PASS = 0

 if (t3[0] != 8) set PASS = 0
 if (t3[1] != 9) set PASS = 0
 if (t3[2] != 7) set PASS = 0
end

# basic read with comments & int types
macro test4
 
 $PASS = 1

 data read.t2.dat
 read t1:int 1 t2:int 2 t3:int 3

 vtype t1 -var type; if ("$type" != "INT") set PASS = 0
 vtype t2 -var type; if ("$type" != "INT") set PASS = 0
 vtype t3 -var type; if ("$type" != "INT") set PASS = 0

 if ($VERBOSE >= 2) vectors

 if (t1[0] != 1) set PASS = 0
 if (t1[1] != 2) set PASS = 0
 if (t1[2] != 4) set PASS = 0

 if (t2[0] != 5) set PASS = 0
 if (t2[1] != 3) set PASS = 0
 if (t2[2] != 5) set PASS = 0

 if (t3[0] != 8) set PASS = 0
 if (t3[1] != 9) set PASS = 0
 if (t3[2] != 7) set PASS = 0
end

# read with invalid entries
macro test5
 
 $PASS = 1

 data read.t3.dat
 read t1 1 t2 2 t3 3

 if (t1[0] != 1) set PASS = 0
 if (t1[1] != 2) set PASS = 0
 if (t1[2] != 4) set PASS = 0

 if (t2[0] != 5) set PASS = 0
 if (not(isnan(t2[1]))) set PASS = 0
 if (t2[2] != 5) set PASS = 0

 if (t3[0] != 8) set PASS = 0
 if (t3[1] != 9) set PASS = 0
 if (not(isnan(t3[2]))) set PASS = 0
end

# read with invalid entries & int types
macro test6
 
 $PASS = 1

 data read.t3.dat
 read t1:int 1 t2:int 2 t3:int 3

 vtype t1 -var type; if ("$type" != "INT") set PASS = 0
 vtype t2 -var type; if ("$type" != "INT") set PASS = 0
 vtype t3 -var type; if ("$type" != "INT") set PASS = 0

 if ($VERBOSE >= 2) vectors

 if (t1[0] != 1) set PASS = 0
 if (t1[1] != 2) set PASS = 0
 if (t1[2] != 4) set PASS = 0

 if (t2[0] != 5) set PASS = 0
 if (t2[1] != 0) set PASS = 0
 if (t2[2] != 5) set PASS = 0

 if (t3[0] != 8) set PASS = 0
 if (t3[1] != 9) set PASS = 0
 if (t3[2] != 0) set PASS = 0
end

