SELECT 
    skycalResult.*,
    skycalRun.stack_id,
    skycalRun.workdir,
    stackRun.filter,
    skycalRun.state,
    skycalRun.label,
    skycalRun.data_group,
    skycalRun.dist_group,
    skycalRun.sky_id,
    skycell.*
FROM skycalRun
JOIN skycalResult USING(skycal_id)
JOIN stackRun USING(stack_id) 
JOIN skycell USING(tess_id, skycell_id)
