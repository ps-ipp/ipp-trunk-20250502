#!/bin/csh -f

 rm -rf catdir.merge.save
 rm -rf catdir.merge
 rsync -auv catdir.2mass/ catdir.merge/
 dvomerge catdir.grizy into catdir.merge
 rsync -auv catdir.merge/ catdir.merge.save/
 dvomerge -replace catdir.2mass into catdir.merge
