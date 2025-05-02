SELECT DISTINCT
    warpBackgroundRun.warp_bg_id,
    warpBackgroundSkyfile.magicked
FROM warpBackgroundRun
JOIN warpSkyfile USING(warp_id)
LEFT JOIN warpBackgroundSkyfile USING(warp_bg_id, skycell_id)
WHERE warpBackgroundRun.state = 'new'
    AND warpSkyfile.quality = 0
    AND warpSkyfile.fault = 0
-- WHERE hook %s
GROUP BY warp_bg_id
HAVING
    COUNT(warpBackgroundSkyfile.skycell_id) = COUNT(warpSkyfile.skycell_id)
    AND SUM(IF(warpBackgroundSkyfile.fault > 0, 1, 0)) = 0
    AND SUM(IF(warpBackgroundSkyfile.quality > 0, 1, 0)) != COUNT(*)
