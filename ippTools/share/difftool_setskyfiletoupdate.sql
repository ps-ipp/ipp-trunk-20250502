UPDATE diffRun
    JOIN diffSkyfile USING(diff_id)
SET diffRun.state = 'update', 
    diffSkyfile.data_state = 'update',
    diffSkyfile.fault = 0
    -- set hook %s
WHERE diff_id = %lld
    AND (diffRun.state = 'cleaned' OR diffRun.state = 'update')
    AND (diffSkyfile.data_state = 'cleaned' OR diffSkyfile.data_state = 'update')
    AND (diffSkyfile.quality = 0)
