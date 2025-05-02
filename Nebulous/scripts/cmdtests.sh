#!/bin/tcsh -f

alias OK '\!*; \
 if ($status == 0) then \
   echo -n "OK : " \
 else \
   echo -n "NOT OK : " \
 endif'

alias IS 'if (\!*) then \
   echo -n "OK : " \
 else \
   echo -n "NOT OK : " \
 endif'

# here is a set of tests of the user-level neb-XXX commands

set myFile = foobar/test01
set myAltF = foobar/test02

neb-stat $myFile > /dev/null
if ($status == 0) then
   neb-rm $myFile
   echo "# reseting test environment"
endif

neb-stat $myAltF > /dev/null
if ($status == 0) then
   neb-rm $myAltF
   echo "# reseting test environment"
endif

OK neb-touch $myFile
echo "neb-touch file"

set nebname = `neb-ls $myFile`
IS ($status == 0)
echo "found nebfile"

IS ("$nebname" == "$myFile")
echo "name of nebfile"

set diskname = `neb-ls -p $myFile`
IS ($status == 0)
echo "found diskname"

IS (-e $diskname)
echo "found diskfile $diskname"

echo "lorem ipsum" > $diskname

set result = `neb-cat $myFile`
IS ("$result" == "lorem ipsum")
echo "neb-cat"

OK neb-stat $myFile > /dev/null
echo neb-stat

neb-stat $myFile | grep "object id" > /dev/null
IS ($status == 0)
echo neb-stat object id

neb-stat $myFile | grep "read lock" > /dev/null
IS ($status == 0)
echo neb-stat read lock

neb-stat $myFile | grep "file:" > /dev/null
IS ($status == 0)
echo neb-stat file:

OK neb-replicate $myFile
echo neb-replicate

set Nfile = `neb-stat $myFile | grep "file:" | wc -l`
IS ($Nfile == 2)
echo "two copies"

neb-copies $myFile 2
IS ($status == 0)
echo neb-copies

set Nc = `neb-copies $myFile`
IS ("$Nc" == "user.copies:2")
echo neb-copies

OK neb-cp $myFile $myAltF

set nebname = `neb-ls $myAltF`
IS ($status == 0)
echo "found nebfile"

IS ("$nebname" == "$myAltF")
echo "name of copy"

set result = `neb-cat $myFile`
IS ("$result" == "lorem ipsum")
echo "neb-cat"

neb-cull $myFile
IS ($status)
echo "neb-cull will not remove instance if only 2 copies"

OK neb-replicate $myFile
echo "neb-replicate makes a 3rd copy"

set Nfile = `neb-stat $myFile | grep "file:" | wc -l`
IS ($Nfile == 3)
echo "three copies"

OK neb-cull $myFile
echo "neb-cull will remove 3rd copy"

OK neb-rm $myFile
echo neb-rm

OK neb-rm $myAltF
echo neb-rm

touch foobar
OK neb-insert $myFile foobar
echo neb-insert

OK neb-stat $myFile > /dev/null
echo neb-stat

neb-stat $myFile | grep "object id" > /dev/null
IS ($status == 0)
echo neb-stat object id

OK neb-mv $myFile $myAltF
echo neb-mv

OK neb-stat $myAltF > /dev/null
echo neb-stat

neb-stat $myAltF | grep "object id" > /dev/null
IS ($status == 0)
echo neb-stat object id

