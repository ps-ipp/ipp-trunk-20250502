#!/bin/csh -f

if ($#argv != 1) then
  echo "USAGE: fontdimen.sh (fontN.bdf)"
  exit 2
endif

set myFont = $1 
set kpname = (times helvetica courier symbol)
set psname = (Times-Roman Helvetica Courier Symbol)

set i = 1
while ($i <= $#kpname)
  echo $myFont | grep $kpname[$i] >& /dev/null
  if ($status) then
    @ i++
    continue
  endif
  set fontsize = `echo $myFont | sed "s/$kpname[$i]//" | sed "s/.bdf//"`
  echo $kpname[$i] $psname[$i] $fontsize
  cat fontsize.in.ps | sed "s/@FONTNAME@/$psname[$i]/" | sed "s/@FONTSIZE@/$fontsize/" > $kpname[$i].$fontsize.ps
  if ($status) exit 1

  ps2txt < $kpname[$i].$fontsize.ps | awk -F, '{printf "%3d %5.2f\n", $1, $2}{printf "%3d %5.2f\n", $3, $4}' > $kpname[$i].$fontsize.psx
  if ($status) exit 1

  (./fixfont $kpname[$i]$fontsize.bdf $kpname[$i].$fontsize.psx $kpname[$i]$fontsize > $kpname[$i]$fontsize.h) >& /dev/null
  if ($status) exit 1

  rm -f $kpname[$i].$fontsize.ps $kpname[$i].$fontsize.psx
  exit 0
end
