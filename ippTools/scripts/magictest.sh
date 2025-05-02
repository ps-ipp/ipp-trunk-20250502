#!//bin/sh

set -o verbose

difftest.sh || exit 1


magictool -definerun -workdir file:///foo/bar/ || exit 1
magictool -addinputskyfile -magic_id 1 -diff_id 1 -node a || exit 1
magictool -addinputskyfile -magic_id 1 -diff_id 2 -node b || exit 1
magictool -inputtree -magic_id 1 -dep_file magic_dep.md 
magictool -updaterun -state run -magic_id 1 || exit 1

magictool -addresult -magic_id 1 -node a -uri file:///foo/a
magictool -addresult -magic_id 1 -node b -uri file:///foo/b
magictool -addresult -magic_id 1 -node root -uri file:///foo/root
magictool -tomask
magictool -addmask -magic_id 1 -uri file:///foo/mask
magictool -toskyfilemask
magictool -addskyfilemask -magic_id 1 -diff_id 1 -uri file:///foo/mask_1
magictool -addskyfilemask -magic_id 1 -diff_id 2 -uri file:///foo/mask_2
magictool -updaterun -state stop -magic_id 1 || exit 1

#difftool -todiffskyfile || exit 1
#difftool -inputskyfile || exit 1
#difftool -adddiffskyfile -diff_id 1 -uri file:///tmp/diff/skyfile -path_base file://lalaland -bg 1 -bg_stdev 2 || exit 1
#difftool -diffskyfile -diff_id 1 || exit 1
#difftool -updaterun -state stop -diff_id 1 || exit 1
