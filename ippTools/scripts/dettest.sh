#!//bin/sh

set -o verbose

det_id=1

./regtest.sh -end_stage reg || exit 1

det_id=`dettool -definebyquery -det_type bias -inst gpc -filelevel fpa -select_exp_type bias -airmass_min 1 -airmass_max 10 -exp_time_min 10 -exp_time_max 30.0 -workdir file::///some/path -simple | cut -f1 -d" "` || exit 1

dettool -raw || exit 1

dettool -toprocessedimfile || exit 1

for ID in `seq 0 3` ; do
    dettool -addprocessedimfile -det_id 1 -exp_id 1 -class_id $ID -uri file://proc-$ID -recip myrecip -bg 2 -bg_stdev 3 -bg_mean_stdev 4 || exit 1
done;

for ID in `seq 0 3` ; do
    dettool -addprocessedimfile -det_id 1 -exp_id 2 -class_id $ID -uri file://proc-$ID -recip myrecip -bg 2 -bg_stdev 3 -bg_mean_stdev 4 || exit 1
done;


dettool -tostacked || exit 1

for ID in `seq 0 3` ; do
    dettool -addstacked -det_id $det_id -uri file://stacked-$ID -class_id $ID -recip myrecipe -bg 1 -bg_stdev 2 -bg_mean_stdev 3 || exit 1
done;

dettool -tonormalizedstat || exit 1

for ID in `seq 0 3` ; do
    dettool -addnormalizedstat -det_id $det_id -class_id $ID -norm 0.12345 || exit 1
done;

dettool -tonormalize || exit 1
for ID in `seq 0 3` ; do
    dettool -addnormalizedimfile -det_id $det_id -class_id $ID -uri file://normalized-$ID -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -path_base banana1 || exit 1
done;

dettool -tonormalizedexp || exit 1
dettool -addnormalizedexp -det_id $det_id -recip myrecipe -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -path_base file://normalizedexp || exit 1

dettool -toresidimfile || exit 1

for ID in `seq 0 3` ; do
    dettool -addresidimfile -det_id $det_id -exp_id 1 -class_id $ID -recip myrecip -bg 1 -bg_stdev 1 -bg_mean_stdev 1 -uri file://resid-$ID || exit 1
done;

for ID in `seq 0 3` ; do
    dettool -addresidimfile -det_id $det_id -exp_id 2 -class_id $ID -recip myrecip -bg 1 -bg_stdev 1 -bg_mean_stdev 1 -uri file://resid-$ID || exit 1
done;

dettool -toresidexp || exit 1
dettool -addresidexp -det_id $det_id -exp_id 1 -recip myrecipe -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -path_base jpeg1 || exit 1
dettool -addresidexp -det_id $det_id -exp_id 2 -recip myrecipe -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -path_base jpeg1 -reject || exit 1

dettool -todetrunsummary || exit 1
dettool -residexp || exit 1
dettool -updateresidexp -det_id $det_id -iteration 0 -recip yourrecipe || exit 1
dettool -updateresidexp -det_id $det_id -iteration 0 -exp_id 2 -reject || exit 1
dettool -updateresidexp -det_id $det_id -iteration 0 -exp_id 2 || exit 1

dettool -adddetrunsummary -det_id $det_id -iteration 0 -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -accept || exit 1

dettool -todetrunsummary || exit 1

dettool -residexp || exit 1

dettool -updatedetrun -det_id $det_id -state stop || exit 1

dettool -register_detrend -det_type bias -mode master -filelevel fpa -workdir file:/// -label foo || exit 1
dettool -register_detrend_imfile -det_id 2 -class_id 1 -uri file:///foo || exit 1
dettool -register_detrend_imfile -det_id 2 -class_id 2 -uri file:///foo || exit 1
dettool -register_detrend_imfile -det_id 2 -class_id 3 -uri file:///foo || exit 1
dettool -register_detrend_imfile -det_id 2 -class_id 4 -uri file:///foo || exit 1
dettool -updatedetrun -det_id 2 -state stop || exit 1

# correct test
dettool -makecorrection -det_id 1 || exit 1
dettool -tocorrectexp || exit 1
dettool -tocorrectimfile -det_id 3 || exit 1
dettool -addcorrectimfile -det_id 3 -class_id 0 -uri file:///correct/0 || exit 1
dettool -addcorrectimfile -det_id 3 -class_id 1 -uri file:///correct/1 || exit 1
dettool -addcorrectimfile -det_id 3 -class_id 2 -uri file:///correct/2 || exit 1
dettool -addcorrectimfile -det_id 3 -class_id 3 -uri file:///correct/3 || exit 1
# detRun 3 should be automatically set to stop by this point 
dettool -tocorrectexp || exit 1
