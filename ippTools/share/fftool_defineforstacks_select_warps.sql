SELECT warp_id
FROM stackInputSkyfile
     JOIN stackRun USING (stack_id)
     JOIN skycalRun USING (stack_id)
WHERE skycal_id = '%d'
