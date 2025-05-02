UPDATE magicRun 
    JOIN magicDSRun USING(magic_id)
    JOIN warpRun ON stage = 'warp' AND stage_id = warp_id
    JOIN warpSkyfile using(warp_id)
SET warpRun.magicked = 0,
    warpSkyfile.magicked = 0,
    magicDSRun.state = '@NEW_STATE@'
WHERE re_place
