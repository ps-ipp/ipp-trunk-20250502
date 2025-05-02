DELETE warpBackgroundSkyfile
FROM warpBackgroundRun
JOIN warpBackgroundSkyfile USING(warp_bg_id)
WHERE warpBackgroundSkyfile.fault != 0
