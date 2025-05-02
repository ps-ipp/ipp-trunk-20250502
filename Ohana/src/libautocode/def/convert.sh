#!/bin/csh -f

foreach f ( *-ps1-v5.d )
  set g = `echo $f | sed s/ps1-v5/ps1-v6/`
  cat $f | sed s/PS1_V5/PS1_V6/ > $g
end

