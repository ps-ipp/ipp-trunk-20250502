INSERT INTO diffInputSkyfile
SELECT
    %s,                         -- diff_id
    skycell_id,
    %s,                         -- input warp_id
    NULL,                       -- input stack_id
    %s,                         -- template warp_id
    NULL,                       -- template stack_id
    tess_id,
    0                           -- diff_skyfile_id
FROM warpSkyfile AS inputWarpSkyfile
JOIN warpSkyfile AS templateWarpSkyfile USING(skycell_id, tess_id)
WHERE inputWarpSkyfile.quality = 0
    AND templateWarpSkyfile.quality = 0
    AND inputWarpSkyfile.warp_id = %s
    AND templateWarpSkyfile.warp_id = %s
