#!/bin/csh -f

 rm -rf catdir.merge
 rm -rf catdir.merge.p?
 rsync -auv catdir.2mass/ catdir.merge/
 cp HostTable.dat catdir.merge/
 dvodist -out catdir.merge
 dvomerge -parallel catdir.grizy into catdir.merge
 dvosecfilt -parallel catdir.merge 10
