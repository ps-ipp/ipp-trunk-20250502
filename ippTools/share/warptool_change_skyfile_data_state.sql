-- handle changes in warpSkyfile.data_state.
-- Used for the modes tofullskyfile, tocleanedskyfile and topurgedskyfile
-- arguments are are new data_state, string for magic hook, warp_id, skycell_id
UPDATE warpSkyfile
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
    SET 
    data_state = '%s'
    -- set magicked hook %s
WHERE
    warp_id = %lld
    AND skycell_id = '%s'
