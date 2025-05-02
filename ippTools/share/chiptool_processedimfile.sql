SELECT DISTINCT
    chipProcessedImfile.chip_id,
    chipImfile.chip_imfile_id,
    chipProcessedImfile.class_id,
    chipProcessedImfile.uri,
    chipProcessedImfile.bg,
    chipProcessedImfile.bg_stdev,
    chipProcessedImfile.bg_mean_stdev,
    chipProcessedImfile.path_base,
    chipProcessedImfile.magicked,
    chipProcessedImfile.data_state,
    chipProcessedImfile.fault,
    chipProcessedImfile.quality,
    chipRun.state,
    chipRun.workdir,
    chipRun.label,
    chipRun.data_group,
    rawExp.exp_id,
    rawExp.exp_tag,
    rawExp.exp_name,
    rawExp.filter,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel,
    rawExp.dateobs,
    rawExp.ra,
    rawExp.decl,
    rawExp.exp_time,
    rawImfile.magicked AS raw_magicked,
    rawImfile.burntool_state,
    IFNULL(magicDSRun.magic_ds_id, 0) AS magic_ds_id,
    IFNULL(magicDSRun.state,0) AS dsRun_state,
    IFNULL(magicDSFile.fault,0) AS dsFile_fault,
    IFNULL(magicDSFile.data_state,0) AS dsFile_data_state
FROM chipRun
JOIN chipImfile
    USING(chip_id)
JOIN chipProcessedImfile
    USING(chip_id, class_id)
JOIN rawExp
    ON chipProcessedImfile.exp_id = rawExp.exp_id
JOIN rawImfile
    ON rawExp.exp_id = rawImfile.exp_id 
    AND chipProcessedImfile.class_id = rawImfile.class_id
LEFT JOIN magicDSRun
    ON stage_id = chip_id AND stage = 'chip' AND magicDSRun.re_place AND magicDSRun.state != 'drop'
LEFT JOIN magicDSFile ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id AND chipProcessedImfile.class_id = magicDSFile.component
