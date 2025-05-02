#!/bin/csh -f

 rm -rf catdir.merge catdir.merge.p?
 rsync --exclude=.svn -auv catdir.2mass/ catdir.merge/
 cp HostTable.dat catdir.merge/
 dvodist -out catdir.merge
 rm -rf catdir.merge/n????

 dvomerge -parallel catdir.grizy into catdir.merge
 dvomerge -parallel catdir.grizy into catdir.merge -verify
