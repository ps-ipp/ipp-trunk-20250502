SELECT 
    magicDSFile.*,
    CONCAT_WS('.', CONCAT(outroot, '/', exp_id), 'mds', magic_ds_id, stage_id, component) AS path_base,
    magicDSRun.magic_id,
    magicDSRun.inv_magic_id,
    magicDSRun.state,
    magicDSRun.stage,
    magicDSRun.stage_id,
    magicDSRun.cam_id,
    magicDSRun.label,
    magicDSRun.data_group,
    magicDSRun.outroot,
    magicDSRun.re_place,
    magicDSRun.fault as run_fault,
    magicDSRun.note
FROM
    magicDSRun JOIN magicDSFile USING(magic_ds_id) JOIN magicRun USING(magic_id)
