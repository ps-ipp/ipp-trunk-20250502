-- change state of warpRun from goto_cleaned to cleaned or goto_purged to purged
-- when all of the consituant skyfiles are in the end state
-- arguments are new state (cleaned or purged) warp_id and new state again for 
-- the chipProcessedImfile sub query
UPDATE warpRun
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
    SET warpRun.state = '%s'
    -- set magicked  hook %s
    WHERE
    warpRun.warp_id = %lld
    AND (SELECT
        COUNT(warp_id)
        FROM warpSkyfile
        WHERE
            warpSkyfile.warp_id = warpRun.warp_id
            AND warpSkyfile.data_state != '%s'
            AND warpSkyfile.quality = 0
        ) = 0
