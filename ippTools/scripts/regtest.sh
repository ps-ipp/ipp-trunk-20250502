#!//bin/sh

set -o verbose

inject="pxinject"
regtool="regtool"

echo -e "YES\njhipp\n\n" | pxadmin -recreate || exit 1

exp_id1=`$inject -newExp -tmp_exp_name t10 -tmp_inst gpc -tmp_telescope ps1 -workdir file::///some/path -reduction batman -simple` || exit 1

echo $exp_id1

for ID in `seq 0 3`; do
    $inject -newImfile -exp_id $exp_id1 -tmp_class_id tmp.$ID -uri file://$ID || exit 1
done;

$inject -updatenewExp -exp_id $exp_id1 -state run

exp_id2=`$inject -newExp -tmp_exp_name t11 -tmp_inst gpc -tmp_telescope ps1 -workdir file::///some/path -reduction robin -simple` || exit 1

for ID in `seq 0 3`; do
    $inject -newImfile -exp_id $exp_id2 -tmp_class_id tmp.$ID -uri file://$ID || exit 1
done;

$inject -updatenewExp -exp_id $exp_id2 -state run


$regtool -pendingimfile || exit 1

for ID in `seq 0 3`; do
    $regtool -addprocessedimfile -exp_id $exp_id1 -exp_name t10 -inst gpc -telescope ps1 -tmp_class_id tmp.$ID -class_id $ID -filter r -airmass 10 -ra 1 -decl 2 -exp_time 0 -bg 1 -bg_stdev 1 -bg_mean_stdev 10 -alt 10 -az 10 -ccd_temp 10 -posang 10 -object dog -dateobs 2006-10-20T10:10:10Z -uri file://$ID || exit 1
done;

$regtool -pendingexp|| exit 1

$regtool -addprocessedexp -exp_id $exp_id1 -exp_name t10 -inst gpc -telescope ps1 -exp_tag batman.t10 -filelevel OTA -filter r -airmass 10 -ra 1 -decl 2 -exp_type object -exp_time 0 -bg 10 -bg_stdev 1 -bg_mean_stdev 10 -alt 10 -az 10 -ccd_temp 45 -posang 10 -object dog -dateobs "2006-10-20T10:10:10Z" -label foobar -tess_id moldyshoe $* || exit 1
# use BIAS or OBJECT? $regtool -addprocessedexp -exp_id $exp_id1 -exp_name t10 -inst gpc -telescope ps1 -exp_tag batman.t10 -filelevel OTA -filter r -airmass 10 -ra 1 -decl 2 -exp_type bias -exp_time 0 -bg 10 -bg_stdev 1 -bg_mean_stdev 10 -alt 10 -az 10 -ccd_temp 45 -posang 10 -object dog -dateobs "2006-10-20T10:10:10Z" -label foobar -tess_id moldyshoe $* || exit 1

$regtool -pendingimfile || exit 1

for ID in `seq 0 3`; do
    $regtool -addprocessedimfile -exp_id $exp_id2 -exp_name t11 -inst gpc -telescope ps1  -tmp_class_id tmp.$ID -class_id $ID -filter r -airmass 10 -ra 1 -decl 2 -exp_time 0 -bg 1 -bg_stdev 1 -bg_mean_stdev 10 -alt 10 -az 10 -ccd_temp 10 -posang 10 -object dog -dateobs 2006-10-20T10:10:10Z -uri file://$ID  || exit 1
done;

$regtool -pendingexp|| exit 1

$regtool -addprocessedexp -exp_id $exp_id2 -exp_name t11 -inst gpc -telescope ps1 -exp_tag batman.t11 -filelevel OTA -filter r -airmass 11 -ra 1 -decl 2 -exp_type object -exp_time 0 -bg 11 -bg_stdev 1 -bg_mean_stdev 11 -alt 11 -az 11 -ccd_temp 45 -posang 11 -object dog -dateobs 2006-10-20T10:10:10Z -label foobar -tess_id moldyshoe $* || exit 1
# BIAS or OBJECT? $regtool -addprocessedexp -exp_id $exp_id2 -exp_name t11 -inst gpc -telescope ps1 -exp_tag batman.t11 -filelevel OTA -filter r -airmass 11 -ra 1 -decl 2 -exp_type bias -exp_time 0 -bg 11 -bg_stdev 1 -bg_mean_stdev 11 -alt 11 -az 11 -ccd_temp 45 -posang 11 -object dog -dateobs 2006-10-20T10:10:10Z -label foobar -tess_id moldyshoe $* || exit 1
