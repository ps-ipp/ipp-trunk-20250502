#!//bin/sh

set -o verbose

regtest.sh || exit 1

chiptool -pendingimfile || exit 1

for ID in `seq 0 3`; do
    chiptool -addprocessedimfile -chip_id 1 -exp_id 1 -class_id $ID -uri file://chipp-t10.$ID -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -path_base file:///foo || exit 1
done;

for ID in `seq 0 3`; do
    chiptool -addprocessedimfile -chip_id 2 -exp_id 2 -class_id $ID -uri file://chipp-t11.$ID -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -path_base file:///foo || exit 1
done;

chiptool -pendingimfile || exit 1
