-- Input warps
SELECT DISTINCT
    diffSkyfile.diff_id,
    diffRun.tess_id,
    diffSkyfile.skycell_id,
    diffSkyfile.path_base,
    diffSkyfile.data_state,
    diffSkyfile.quality
FROM magicDSRun
JOIN magicRun USING(magic_id)
JOIN magicInputSkyfile USING(magic_id)
JOIN diffRun USING(diff_id)
JOIN diffSkyfile
    ON magicRun.diff_id = diffSkyfile.diff_id
    AND magicInputSkyfile.node = diffSkyfile.skycell_id
JOIN diffInputSkyfile
    ON diffInputSkyfile.diff_id = diffSkyfile.diff_id
    AND diffInputSkyfile.skycell_id = diffSkyfile.skycell_id
    -- Want input warps only
    AND diffInputSkyfile.warp1 IS NOT NULL
    AND magicRun.inverse = 0
JOIN warpSkyCellMap
    ON warpSkyCellMap.warp_id = diffInputSkyfile.warp1
    AND warpSkyCellMap.skycell_id = diffInputSkyfile.skycell_id
JOIN warpSkyfile
    ON warpSkyfile.warp_id = warpSkyCellMap.warp_id
    AND warpSkyfile.skycell_id = warpSkyCellMap.skycell_id
    AND warpSkyfile.quality = 0
WHERE
    diffSkyfile.fault = 0
    AND diffSkyfile.quality = 0
    -- WHERE hook %s
UNION
-- Reference warps
SELECT DISTINCT
    diffSkyfile.diff_id,
    diffRun.tess_id,
    diffSkyfile.skycell_id,
    diffSkyfile.path_base,
    diffSkyfile.data_state,
    diffSkyfile.quality
FROM magicDSRun
JOIN magicRun USING(magic_id)
JOIN magicInputSkyfile USING(magic_id)
JOIN diffRun USING(diff_id)
JOIN diffSkyfile
    ON magicRun.diff_id = diffSkyfile.diff_id
    AND magicInputSkyfile.node = diffSkyfile.skycell_id
JOIN diffInputSkyfile
    ON diffInputSkyfile.diff_id = diffSkyfile.diff_id
    AND diffInputSkyfile.skycell_id = diffSkyfile.skycell_id
    AND diffInputSkyfile.warp2 IS NOT NULL
    AND magicRun.inverse = 1
JOIN warpSkyCellMap
    ON warpSkyCellMap.warp_id = diffInputSkyfile.warp2
    AND warpSkyCellMap.skycell_id = diffInputSkyfile.skycell_id
JOIN warpSkyfile
    ON warpSkyfile.warp_id = warpSkyCellMap.warp_id
    AND warpSkyfile.skycell_id = warpSkyCellMap.skycell_id
    AND warpSkyfile.quality = 0
WHERE
    diffSkyfile.fault = 0
    AND diffSkyfile.quality = 0
    -- WHERE hook %s
