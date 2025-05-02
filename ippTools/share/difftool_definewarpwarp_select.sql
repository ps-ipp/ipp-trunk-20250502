SELECT
    inputWarpRun.warp_id AS input_warp_id,
    inputWarpRun.tess_id AS tess_id,
    inputRawExp.exp_id AS input_exp_id,
    inputWarpRun.data_group AS input_data_group,
    -- The following trick pulls out the warp_id that has the smallest distance
    SUBSTRING_INDEX(GROUP_CONCAT(templateWarpRun.warp_id ORDER BY ABS(ASIN(SQRT(POW(SIN(0.5*(inputRawExp.decl - templateRawExp.decl)),2) + COS(inputRawExp.decl) * COS(templateRawExp.decl) * POW(SIN(0.5*(inputRawExp.ra - templateRawExp.ra)),2))))), ',', 1) AS template_warp_id
FROM warpRun AS inputWarpRun
JOIN fakeRun AS inputFakeRun USING(fake_id)
JOIN camRun AS inputCamRun USING(cam_id)
JOIN chipRun AS inputChipRun USING(chip_id)
JOIN rawExp AS inputRawExp USING(exp_id)
-- To find exposures that haven't been diffed, insert newline here:%s LEFT JOIN diffs USING(exp_id)
JOIN warpRun AS templateWarpRun
    ON templateWarpRun.warp_id != inputWarpRun.warp_id -- Don't use self as template!
    AND templateWarpRun.tess_id = inputWarpRun.tess_id -- Ensure using same tessellation
JOIN fakeRun AS templateFakeRun
    ON templateFakeRun.fake_id = templateWarpRun.fake_id
    AND templateFakeRun.fake_id != inputFakeRun.fake_id -- Don't use self as template!
JOIN camRun AS templateCamRun
    ON templateCamRun.cam_id = templateFakeRun.cam_id
    AND templateCamRun.cam_id != inputCamRun.cam_id -- Don't use self as template!
JOIN chipRun AS templateChipRun
    ON templateChipRun.chip_id = templateCamRun.chip_id
    AND templateChipRun.chip_id != inputChipRun.chip_id -- Don't use self as template!
JOIN rawExp AS templateRawExp
    ON templateRawExp.exp_id = templateChipRun.exp_id
    AND templateRawExp.filter = inputRawExp.filter
    AND templateRawExp.exp_id != inputRawExp.exp_id -- Don't use self as template!
WHERE inputWarpRun.state = 'full'
    AND templateWarpRun.state = 'full'
-- WHERE hook %s
GROUP BY input_warp_id
