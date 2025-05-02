SELECT DISTINCT
    diffPhotRun.*,
    diffRun.tess_id,
    diffSkyfile.skycell_id
FROM diffPhotRun
JOIN diffRun USING(diff_id)
JOIN diffSkyfile USING(diff_id)
LEFT JOIN diffPhotSkyfile USING(diff_phot_id, skycell_id)
WHERE diffPhotSkyfile.skycell_id IS NULL
    AND diffRun.state = 'full'
    AND diffSkyfile.magicked >= 0
    AND diffSkyfile.fault = 0
    AND diffSkyfile.quality = 0
