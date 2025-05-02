SELECT DISTINCT
    warpSkyfile.warp_id,
    warpSkyfile.skycell_id,
    warpSkyfile.uri,
    warpSkyfile.path_base
FROM magicRun
JOIN magicInputSkyfile USING(magic_id)
JOIN diffRun USING(diff_id)
JOIN diffInputSkyfile
    ON diffInputSkyfile.diff_id = diffRun.diff_id
    AND diffInputSkyfile.skycell_id = diffRun.skycell_id
    -- Want input warps only
    AND diffInputSkyfile.warp_id IS NOT NULL
    AND diffInputSkyfile.template = 0
JOIN warpSkyfile
    ON warpSkyfile.warp_id = diffInputSkyfile.warp_id
    AND warpSkyfile.skycell_id = diffRun.skycell_id
JOIN warpRun
    ON warpRun.warp_id = warpSkyfile.warp_id
WHERE
    warpRun.state = 'full'
    AND warpSkyfile.fault = 0
    AND warpSkyfile.quality = 0
--   AND magicRun.state = 'full'
--   AND magicMask.fault = 0
