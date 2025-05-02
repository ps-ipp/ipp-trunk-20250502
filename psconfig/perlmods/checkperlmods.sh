#!/bin/csh -f

# extract a complete list
# cat *.perllib.txt *.perlbin.txt | awk '{print $2}' | sort | uniq > all.mods.txt

# now identify non-modules (e.g., strict), IPP modules, and external modules
# e.g. NOT | constant, IPP | DataStore, EXT | IPC-Run

# find the total number of refs to each EXT module

rm -f ext.mods.cnt
foreach mod (`grep "^EXT |" all.mods.txt | awk '{print $3}'`)
  set Nmod = `grep $mod *.perllib.txt *.perlbin.txt | awk '{n+=$2}END{print n}'`
  echo $mod $Nmod >> ext.mods.cnt
end

# find where uncommon modules are used (do we really need them?)
rm -f uncommon.mods.txt
foreach mod (`awk '($2 < 3){print $1}' ext.mods.cnt`)
  grep $mod *.perllib.txt *.perlbin.txt >> uncommon.mods.txt
end

