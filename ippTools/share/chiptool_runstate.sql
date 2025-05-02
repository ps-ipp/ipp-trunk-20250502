SELECT 
    chip_id,
    chipRun.state,
    exp_id,
    magic_ds_id,
    magicDSRun.state as magic_ds_state
FROM chipRun
    JOIN rawExp USING(exp_id)
    LEFT JOIN magicDSRun on stage = 'chip' and stage_id = chip_id and re_place

