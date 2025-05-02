SELECT DISTINCT
    chip_id,
    class_id,
    chipProcessedImfile.uri,
    chipProcessedImfile.path_base,
    camProcessedExp.path_base as cam_path_base
FROM magicRun
JOIN magicInputSkyfile USING(magic_id)
JOIN diffRun USING(diff_id)
JOIN diffInputSkyfile
    ON diffInputSkyfile.diff_id = diffRun.diff_id
    AND diffInputSkyfile.skycell_id = diffRun.skycell_id
    -- Want input warps only
    AND diffInputSkyfile.warp_id IS NOT NULL
    AND diffInputSkyfile.template = 0
JOIN warpRun USING(warp_id)
JOIN warpSkyCellMap
    ON warpSkyCellMap.warp_id = warpRun.warp_id
    AND warpSkyCellMap.skycell_id = diffRun.skycell_id
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN camProcessedExp USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN chipProcessedImfile USING(chip_id,class_id)
WHERE
    chipRun.state = 'full'
    AND chipProcessedImfile.fault = 0
    AND chipProcessedImfile.quality = 0
--   AND magicRun.state = 'full'
--   AND magicMask.fault = 0
