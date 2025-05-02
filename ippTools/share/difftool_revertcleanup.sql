UPDATE diffSkyfile
    JOIN diffRun using(diff_id)
SET diffSkyfile.data_state = 'full', 
    diffRun.state = '%s'
WHERE
    diffRun.state = '%s'
    AND diffSkyfile.data_state = '%s'
