#!//bin/sh

set -o verbose

chiptest.sh || exit 1

camtool -pendingexp || exit 1
camtool -pendingimfile || exit 1

camtool -addprocessedexp -cam_id 1 -uri file:///cam -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -sigma_ra 1 -sigma_dec 2 -zp_mean 10 -zp_stdev 2 -n_stars 2 -n_extended 0 -n_astrom 42 -n_cr 10000000 -path_base file:///foo || exit 1
camtool -addprocessedexp -cam_id 2 -uri file:///cam -bg 1 -bg_stdev 2 -bg_mean_stdev 3 -sigma_ra 1 -sigma_dec 2 -zp_mean 10 -zp_stdev 2 -n_stars 2 -n_extended 0 -n_astrom 42 -n_cr 10000000 -path_base file:///foo || exit 1
