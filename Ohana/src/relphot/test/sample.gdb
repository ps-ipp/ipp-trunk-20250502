
set $i = 0

while ($i < Nmosaic)
# printf "%f %f %f\n", dataset->alldata->xVector[$i], dataset->alldata->yVector[$i], dataset->alldata->dyVector[$i]
# printf "%f %f\n", results->psfData[Nsec].flxlist[$i], results->psfData[Nsec].errlist[$i]
 printf "%d : %f %f\n", $i, mlist[$i], slist[$i]
 set $i = $i + 1
end
