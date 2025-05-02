#!/bin/csh -f

set liblist = ""
set liblist = "$liblist Nebulous"
set liblist = "$liblist Nebulous-Server"
set liblist = "$liblist DataStore"
set liblist = "$liblist DataStoreServer"
set liblist = "$liblist console "
set liblist = "$liblist PS-IPP-Config"
set liblist = "$liblist PS-IPP-Metadata-Config"
set liblist = "$liblist PS-IPP-MetaDB"
set liblist = "$liblist PS-IPP-PSFTP"
set liblist = "$liblist PS-IPP-PStamp"
foreach lib ($liblist)
  get.perlmods.sh lib ../$lib > perllibs/$lib.perllib.txt
end  

set binlist = ""
set binlist = "$binlist Nebulous/bin"
set binlist = "$binlist Nebulous-Server/bin"
set binlist = "$binlist DataStore/scripts"
set binlist = "$binlist DataStoreServer/scripts"
set binlist = "$binlist ippMonitor"
set binlist = "$binlist ippScripts"
set binlist = "$binlist pstamp/scripts"
foreach bin ($binlist) 
  set base = `echo $bin | awk -F/ '{print $1}'`
  get.perlmods.sh bin ../$bin > perllibs/$base.perlbin.txt
end  

get.perlmods.sh file ../glueforge/glueforge.in > perllibs/glueforge.perlbin.txt
