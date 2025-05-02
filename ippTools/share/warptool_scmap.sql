SELECT
    warpSkyCellMap.*,
    chipProcessedImfile.uri,
    chipProcessedImfile.path_base as chip_path_base,
    camProcessedExp.path_base as cam_path_base,
    camProcessedExp.fault as cam_fault,
    camProcessedExp.background_model AS cam_background_model,
    chipProcessedImfile.chip_id,
    chipRun.state,
    chipRun.data_group as chip_data_group,
    chipProcessedImfile.data_state,
    chipProcessedImfile.fault AS chip_fault,
    chipProcessedImfile.magicked,
    camRun.state as camState,
    rawImfile.magicked AS raw_magicked,
    IFNULL(magicDSRun.magic_ds_id, 0) AS magic_ds_id,
    IFNULL(magicDSRun.state, 0) AS dsRun_state,
    IFNULL(magicDSFile.data_state, 0) AS dsFile_data_state,
    IFNULL(magicDSFile.fault, 0) as dsFile_fault
FROM warpRun
JOIN warpSkyCellMap
    USING(warp_id)
JOIN fakeRun
    USING(fake_id)
JOIN camRun
    USING(cam_id)
JOIN camProcessedExp
    USING(cam_id)
JOIN chipRun
    USING(chip_id)
JOIN chipProcessedImfile
    ON chipRun.chip_id = chipProcessedImfile.chip_id
    AND warpSkyCellMap.class_id = chipProcessedImfile.class_id
    AND chipProcessedImfile.quality = 0
JOIN rawImfile
    ON chipRun.exp_id = rawImfile.exp_id
    AND chipProcessedImfile.class_id = rawImfile.class_id
LEFT JOIN magicDSRun
    ON chipRun.chip_id = magicDSRun.stage_id AND magicDSRun.stage = 'chip'
LEFT JOIN magicDSFile
    ON magicDSFile.magic_ds_id = magicDSRun.magic_ds_id 
        AND chipProcessedImfile.class_id = magicDSFile.component
WHERE
    1
