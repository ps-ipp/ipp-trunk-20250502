SELECT
    skycalRun.skycal_id,
    skycalResult.path_base,
    skycalRun.data_group,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.filter
FROM skycalRun
    JOIN skycalResult USING(skycal_id)
    JOIN stackRun USING(stack_id)
    JOIN skycell USING(tess_id, skycell_id)
    -- join hook %s
WHERE 
    skycalRun.state = 'full' AND skycalResult.quality = 0
