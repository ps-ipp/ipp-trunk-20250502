#!/bin/csh -f

 rm -rf catdir.merge catdir.grizy.output
 rsync --exclude=.svn -auv catdir.2mass/ catdir.merge/
 rsync --exclude=.svn -auv catdir.grizy/ catdir.grizy.output/
 cp HostTable.dat catdir.merge/
 dvodist -out catdir.merge
 rm -rf catdir.merge/n0000 catdir.merge/n0730
 dvomerge -parallel-input catdir.merge into catdir.grizy.output/
 dvomerge -verify -parallel-input catdir.merge into catdir.grizy.output/
