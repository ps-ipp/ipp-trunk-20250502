SELECT warp_bg_id,
    skycell_id,
    warpSkyfile.path_base
FROM warpBackgroundRun
    JOIN warpBackgroundSkyfile USING(warp_bg_id)
    JOIN warpSkyfile USING(warp_id, skycell_id)
