SELECT *
FROM (
    -- Forward diff
    SELECT DISTINCT
        magicRun.magic_id,
        rawExp.*,
        camProcessedExp.path_base,
        camProcessedExp.zpt_obs,
        camProcessedExp.zpt_stdev,
        camProcessedExp.fwhm_major,
        camProcessedExp.fwhm_minor,
        camProcessedExp.iq_m2,
        camProcessedExp.iq_m3,
        camProcessedExp.iq_m4
    FROM
        magicRun
    JOIN diffRun USING(diff_id)
    JOIN diffInputSkyfile USING(diff_id)
    LEFT JOIN warpRun
        ON warpRun.warp_id = diffInputSkyfile.warp1
        AND magicRun.inverse = 0
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN camProcessedExp USING(cam_id)
    JOIN chipRun USING(chip_id,exp_id)
    JOIN rawExp USING(exp_id)
    -- WHERE hook %s
    UNION
    -- Backward diff
    SELECT DISTINCT
        magicRun.magic_id,
        rawExp.*,
        camProcessedExp.path_base,
        camProcessedExp.zpt_obs,
        camProcessedExp.zpt_stdev,
        camProcessedExp.fwhm_major,
        camProcessedExp.fwhm_minor,
        camProcessedExp.iq_m2,
        camProcessedExp.iq_m3,
        camProcessedExp.iq_m4
    FROM
        magicRun
    JOIN diffRun USING(diff_id)
    JOIN diffInputSkyfile USING(diff_id)
    LEFT JOIN warpRun
        ON warpRun.warp_id = diffInputSkyfile.warp2
        AND magicRun.inverse = 1
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN camProcessedExp USING(cam_id)
    JOIN chipRun USING(chip_id,exp_id)
    JOIN rawExp USING(exp_id)
    -- WHERE hook %s
) AS magicExposures
