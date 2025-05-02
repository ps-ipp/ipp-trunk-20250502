-- the subselect is so where criteria can be specified without knowing
-- which table the field came from
SELECT DISTINCT
    fakeRun.*,
    chipProcessedImfile.exp_id,
    chipProcessedImfile.class_id,
    chipProcessedImfile.uri,
    chipProcessedImfile.bg,
    chipProcessedImfile.bg_stdev,
    chipProcessedImfile.bg_mean_stdev,
    chipProcessedImfile.fringe_0,
    chipProcessedImfile.fringe_1,
    chipProcessedImfile.fringe_2,
    chipProcessedImfile.ap_resid,
    chipProcessedImfile.ap_resid_stdev,
    chipProcessedImfile.n_stars,
    chipProcessedImfile.n_extended,
    chipProcessedImfile.n_cr,
    chipProcessedImfile.path_base as chip_path_base,
    camProcessedExp.path_base as cam_path_base,
    rawExp.exp_name,
    rawExp.exp_tag,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel
FROM fakeRun
JOIN camRun USING(cam_id)
JOIN camProcessedExp USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN chipProcessedImfile USING(chip_id,exp_id)
JOIN rawExp USING(exp_id)
--    ON chipProcessedImfile.exp_id = rawExp.exp_id
LEFT JOIN fakeProcessedImfile
    ON fakeRun.fake_id = fakeProcessedImfile.fake_id
    AND chipProcessedImfile.class_id = fakeProcessedImfile.class_id
LEFT JOIN fakeMask
    ON fakeRun.label = fakeMask.label
WHERE
    ((fakeRun.state = 'new'
        AND fakeProcessedImfile.fake_id IS NULL
        AND fakeProcessedImfile.class_id IS NULL
    )
    OR (fakeRun.state = 'update'
        AND fakeProcessedImfile.data_state = 'cleaned')
    )
    AND fakeMask.label IS NULL
