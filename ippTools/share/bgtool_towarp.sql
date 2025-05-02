SELECT DISTINCT
    warpBackgroundRun.*,
    warpRun.tess_id,
    skycell_id,
    rawExp.exp_tag,
    rawExp.camera
FROM warpBackgroundRun
JOIN warpRun USING(warp_id)
JOIN warpSkyCellMap USING(warp_id)
JOIN warpSkyfile USING(warp_id, skycell_id)
JOIN chipBackgroundRun USING(chip_bg_id)
JOIN chipBackgroundImfile USING(chip_bg_id, class_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
LEFT JOIN warpBackgroundSkyfile USING(warp_bg_id, skycell_id)
LEFT JOIN Label ON Label.label = warpBackgroundRun.label
WHERE warpBackgroundSkyfile.warp_bg_id IS NULL
    AND chipBackgroundRun.state = 'full'
    AND chipBackgroundImfile.fault = 0
    AND chipBackgroundImfile.quality = 0
    AND warpRun.state IN ('full', 'cleaned', 'goto_cleaned')
    AND warpSkyfile.fault = 0
    AND warpSkyfile.quality = 0
    AND (Label.active OR Label.active IS NULL)
-- WHERE hook %s
ORDER BY priority DESC, warp_bg_id, skycell_id
