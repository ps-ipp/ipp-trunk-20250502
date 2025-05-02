#!//bin/sh

set -o verbose

warptest.sh || exit 1


difftool -definerun -workdir file:///tmp/diff -skycell_id foo1 -tess_id bar || exit 1
difftool -addinputskyfile -diff_id 1 -warp_id 1 -kind warped -template || exit 1
difftool -addinputskyfile -diff_id 1 -warp_id 1 -kind warped || exit 1
#difftool -updaterun -state run -diff_id 1 || exit 1
difftool -todiffskyfile || exit 1
difftool -inputskyfile || exit 1
difftool -adddiffskyfile -diff_id 1 -uri file:///tmp/diff/skyfile -path_base file://lalaland -bg 1 -bg_stdev 2 || exit 1
difftool -diffskyfile -diff_id 1 || exit 1
difftool -updaterun -state stop -diff_id 1 || exit 1

#
# diff_id 2
#
difftool -definerun -workdir file:///tmp/diff -skycell_id foo1 -tess_id bar || exit 1
difftool -addinputskyfile -diff_id 2 -warp_id 1 -kind warped -template || exit 1
difftool -addinputskyfile -diff_id 2 -warp_id 1 -kind warped || exit 1
#difftool -updaterun -state run -diff_id 1 || exit 1
difftool -todiffskyfile || exit 1
difftool -inputskyfile || exit 1
difftool -adddiffskyfile -diff_id 2 -uri file:///tmp/diff/skyfile -path_base file://lalaland -bg 1 -bg_stdev 2 || exit 1
difftool -diffskyfile -diff_id 2 || exit 1
difftool -updaterun -state stop -diff_id 2 || exit 1
