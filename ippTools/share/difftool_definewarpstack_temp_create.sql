CREATE TEMPORARY TABLE skycellsToDiff (
diff_id BIGINT,
skycell_id VARCHAR(64),
warp1 BIGINT,
stack1 BIGINT,
warp2 BIGINT,
stack2 BIGINT,
tess_id VARCHAR(64),
diff_skycell_id BIGINT
) ENGINE=MEMORY;
