#!//bin/sh

set -o verbose

faketest.sh || exit 1

#warp tool -definerun -fake_id 1 -mode warp -workdir file:///foo 

#warptool -addinputexp -warp_id 1 -cam_id 1 || exit 1
#warptool -addinputexp -warp_id 1 -cam_id 2 || exit 1
#warptool -updaterun -warp_id 1 -state run || exit 1

warptool -exp -warp_id 1 || exit 1

warptool -imfile -warp_id 1 || exit 1

warptool -tooverlap -warp_id 1 || exit 1

warptool -addoverlap -mapfile mapfile.txt || exit 1

warptool -scmap || exit 1

warptool -towarped || exit 1

warptool -addwarped -warp_id 1 -skycell_id foo1 -tess_id bar -uri file:///tmp/foo -path_base file://wonderland -bg 1 -bg_stdev 2 || exit 1
warptool -addwarped -warp_id 1 -skycell_id foo2 -tess_id bar -uri file:///tmp/foo -path_base file://wonderland -bg 1 -bg_stdev 2 || exit 1

warptool -warped -warp_id 1 || exit 1

#warptool -updaterun -warp_id 1 -state stop || exit 1
