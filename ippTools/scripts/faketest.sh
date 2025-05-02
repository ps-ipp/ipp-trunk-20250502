#!//bin/sh

set -o verbose

camtest.sh || exit 1

#faketool -pendingexp || exit 1
faketool -pendingimfile || exit 1

for ID in `seq 0 3`; do
    faketool -addprocessedimfile -fake_id 1 -exp_id 1 -class_id $ID -uri file://chipp-t10.$ID -path_base file:///foo || exit 1
done;

for ID in `seq 0 3`; do
    faketool -addprocessedimfile -fake_id 2 -exp_id 2 -class_id $ID -uri file://chipp-t11.$ID -path_base file:///foo || exit 1
done;

faketool -pendingimfile || exit 1
faketool -processedimfile || exit 1
