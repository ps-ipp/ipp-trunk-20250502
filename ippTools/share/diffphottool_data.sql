SELECT DISTINCT
    diffPhotSkyfile.*,
    diffRun.tess_id,
    diffPhotRun.state,
    diffPhotRun.workdir,
    diffPhotRun.label,
    diffPhotRun.diff_id,
    diffRun.bothways,
    diffRun.diff_mode,
    -- The following are only valid for warps
    -- XXX This needs to be more clever to handle diffs between stacks
    -- Zero points are appropriate for both forward and backward diffs
    camProcessedInput.zpt_obs,
    camProcessedInput.zpt_stdev,
    camProcessedInput.zpt_lq,
    camProcessedInput.zpt_uq,
    rawInput.comment,
    rawInput.exp_time,
    rawInput.camera,
    rawInput.exp_name AS exp_name_1,
    rawInput.exp_id AS exp_id_1,
    chipInput.chip_id AS chip_id_1,
    camInput.cam_id AS cam_id_1,
    fakeInput.fake_id AS fake_id_1,
    warp1,
    camProcessedInput.sigma_ra AS sigma_ra_1,
    camProcessedInput.sigma_dec AS sigma_dec_1,
    rawTemplate.exp_name AS exp_name_2,
    rawTemplate.exp_id AS exp_id_2,
    chipTemplate.chip_id AS chip_id_2,
    camTemplate.cam_id AS cam_id_2,
    fakeTemplate.fake_id AS fake_id_2,
    warp2,
    camProcessedTemplate.sigma_ra AS sigma_ra_2,
    camProcessedTemplate.sigma_dec AS sigma_dec_2
FROM diffPhotRun
JOIN diffPhotSkyfile USING(diff_phot_id)
JOIN diffRun USING(diff_id)
JOIN diffInputSkyfile USING(diff_id, skycell_id)
LEFT JOIN warpRun AS warpInput
    ON warpInput.warp_id = diffInputSkyfile.warp1
LEFT JOIN fakeRun AS fakeInput
    ON fakeInput.fake_id = warpInput.fake_id
LEFT JOIN camRun AS camInput
    ON camInput.cam_id = fakeInput.cam_id
LEFT JOIN camProcessedExp AS camProcessedInput
    ON camProcessedInput.cam_id = camInput.cam_id
LEFT JOIN chipRun AS chipInput
    ON chipInput.chip_id = camInput.chip_id
LEFT JOIN rawExp AS rawInput
    ON rawInput.exp_id = chipInput.exp_id
LEFT JOIN warpRun AS warpTemplate
    ON warpTemplate.warp_id = diffInputSkyfile.warp2
LEFT JOIN fakeRun AS fakeTemplate
    ON fakeTemplate.fake_id = warpTemplate.fake_id
LEFT JOIN camRun AS camTemplate
    ON camTemplate.cam_id = fakeTemplate.cam_id
LEFT JOIN camProcessedExp AS camProcessedTemplate
    ON camProcessedTemplate.cam_id = camTemplate.cam_id
LEFT JOIN chipRun AS chipTemplate
    ON chipTemplate.chip_id = camTemplate.chip_id
LEFT JOIN rawExp AS rawTemplate
    ON rawTemplate.exp_id = chipTemplate.exp_id
