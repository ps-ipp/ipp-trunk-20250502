SELECT 
    magic_ds_id,
    stage,
    stage_id,
    component,
    backup_path_base
FROM magicDSFile
JOIN magicDSRun USING(magic_ds_id)
WHERE magicDSRun.remove
    AND magicDSRun.state = 'full'
    AND backup_path_base IS NOT NULL
