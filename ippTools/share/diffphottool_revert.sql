DELETE diffPhotSkyfile
FROM diffPhotRun
JOIN diffPhotSkyfile USING(diff_phot_id)
WHERE fault != 0
