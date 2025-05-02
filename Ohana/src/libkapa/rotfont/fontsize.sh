#!/bin/csh -f

set kpname = (times helvetica courier symbol)
set psname = (Times-Roman Helvetica Courier Symbol)

set i = 1
while ($i <= $#kpname)
  foreach fontsize (8 12 14 18 24)
    echo $kpname[$i] $psname[$i] $fontsize
    cat fontsize.in.ps | sed "s/@FONTNAME@/$psname[$i]/" | sed "s/@FONTSIZE@/$fontsize/" > fontsize.ps
    ps2txt < fontsize.ps | awk -F, '{printf "%3d %5.2f\n", $1, $2}{printf "%3d %5.2f\n", $3, $4}' > $kpname[$i].$fontsize.psx
    ./fixfont $kpname[$i]$fontsize.bdf $kpname[$i].$fontsize.psx $kpname[$i]$fontsize > $kpname[$i]$fontsize.h
  end
  @ i++
end
