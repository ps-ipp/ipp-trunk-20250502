#!/bin/csh -f

 rm -rf catdir.merge catdir.grizy.output
 rsync --exclude=.svn -auv catdir.2mass/ catdir.2mass.input/
 rsync --exclude=.svn -auv catdir.grizy/ catdir.grizy.output/

 set rootdir = `pwd`
 sed "s|catdir.merge|$rootdir/catdir.2mass.input|"  < HostTable.dat > catdir.2mass.input/HostTable.dat
 sed s/catdir.merge/catdir.grizy.output/ < HostTable.dat > catdir.grizy.output/HostTable.dat

 dvodist -out catdir.2mass.input
 dvodist -out catdir.grizy.output

 rm -rf catdir.2mass.input/n???? 
 rm -rf catdir.grizy.output/n????

 dvomerge -parallel-input -parallel catdir.2mass.input into catdir.grizy.output -region 0.01 4.99 0.01 2.49 -v
 # dvomerge -parallel-input -parallel catdir.2mass.input into catdir.grizy.output -verify

 ## XXX note : this test fails because there are multiple input
 ## catdirs for a given output catdir AND the sizes and timestamps of
 ## the cpt files match (size matches because of the 2880 block size;
 ## time matches because they are all touched in ~1 sec).

