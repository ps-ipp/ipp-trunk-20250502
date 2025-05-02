CREATE TEMPORARY TABLE warpComplete
 (warp_id BIGINT, skycell_id VARCHAR(64), tess_id VARCHAR(64),
 PRIMARY KEY(warp_id, skycell_id, tess_id)) ENGINE=MEMORY;
