DELETE FROM magicDSFile
    USING magicDSRun, magicDSFile, chipRun, chipProcessedImfile
WHERE chipRun.chip_id = chipProcessedImfile.chip_id
    AND magicDSRun.stage ='chip'
    AND magicDSRun.stage_id = chipRun.chip_id
    AND magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
    AND magicDSFile.component = chipProcessedImfile.class_id
