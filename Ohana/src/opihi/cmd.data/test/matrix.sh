
list tests
 test1
 test2
end

# a very simple diagonal matrix equation
macro test1
 $PASS = 1
 break -auto off

 delete -q A B C
 mcreate B 2 2
 mcreate C 2 2

 B[0][0] = 2
 B[1][0] = -1
 B[0][1] = -1
 B[1][1] = 4

 C[0][0] = 2
 C[1][0] = 1
 C[0][1] = 1
 C[1][1] = 1

 matrix set A = B * C

 # A[0][0] = B[0][0]*C[0][0] + B[1][0]*C[0][1] =  4 - 1 = 3
 # A[1][0] = B[0][0]*C[1][0] + B[1][0]*C[1][1] =  2 - 1 = 1
 # A[0][1] = B[0][1]*C[0][0] + B[1][1]*C[0][1] = -2 + 4 = 2
 # A[1][1] = B[0][1]*C[1][0] + B[1][1]*C[1][1] = -1 + 4 = 3

 if (abs(A[0][0] - 3.0) > 0.0001)
  $PASS = 0
 end
 if (abs(A[0][1] - 2.0) > 0.0001)
  $PASS = 0
 end
 if (abs(A[1][0] - 1.0) > 0.0001)
  $PASS = 0
 end
 if (abs(A[1][1] - 3.0) > 0.0001)
  $PASS = 0
 end
end

# a very simple off-diagonal matrix equation
macro test2
 $PASS = 1
 break -auto off

 delete -q A B

 mcreate A 2 2

 A[0][0] =  2
 A[1][0] = +1
 A[0][1] = -1
 A[1][1] =  4

 matrix transpose A to B
 if (not($STATUS))
   $PASS = 0
 end

 if (abs(B[0][0] - 2.0) > 0.0001)
  $PASS = 0
 end
 if (abs(B[0][1] - 1.0) > 0.0001)
  $PASS = 0
 end
 if (abs(B[1][0] + 1.0) > 0.0001)
  $PASS = 0
 end
 if (abs(B[1][1] - 4.0) > 0.0001)
  $PASS = 0
 end
end
