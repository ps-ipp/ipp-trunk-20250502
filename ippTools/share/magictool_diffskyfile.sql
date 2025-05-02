SELECT DISTINCT
    diffSkyfile.diff_id,
    diffRun.skycell_id,
    diffSkyfile.uri,
    diffSkyfile.path_base
FROM magicRun
JOIN magicInputSkyfile USING(magic_id)
JOIN diffRun USING(diff_id)
JOIN diffSkyfile USING(diff_id)
JOIN diffInputSkyfile
    ON diffInputSkyfile.diff_id = diffRun.diff_id
    AND diffInputSkyfile.skycell_id = diffRun.skycell_id
    -- Want input warps only
    AND diffInputSkyfile.warp_id IS NOT NULL
    AND diffInputSkyfile.template = 0
JOIN warpSkyCellMap
    ON warpSkyCellMap.warp_id = diffInputSkyfile.warp_id
    AND warpSkyCellMap.skycell_id = diffInputSkyfile.skycell_id
JOIN warpSkyfile
    ON warpSkyfile.warp_id = warpSkyCellMap.warp_id
    AND warpSkyfile.skycell_id = warpSkyCellMap.skycell_id
    AND warpSkyfile.quality = 0
WHERE
    diffRun.state = 'full'
    AND diffSkyfile.fault = 0
    AND diffSkyfile.quality = 0
--   AND magicRun.state = 'full'
--   AND magicMask.fault = 0
