SELECT
    diff_phot_id,
    diffPhotSkyfile.magicked
FROM diffPhotRun
JOIN diffSkyfile USING(diff_id)
LEFT JOIN diffPhotSkyfile USING(diff_phot_id, skycell_id)
WHERE diffPhotRun.state = 'new'
    AND diffSkyfile.fault = 0
    AND diffSkyfile.quality = 0
-- WHERE hook %s
GROUP BY diff_phot_id
HAVING COUNT(diffPhotSkyfile.skycell_id) = COUNT(diffSkyfile.skycell_id)
    AND SUM(IF(diffPhotSkyfile.fault > 0, 1, 0)) = 0
