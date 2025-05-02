SELECT
    magic_ds_id,
    state
FROM magicDSRun
LEFT JOIN magicDSFile USING(magic_ds_id)
WHERE 
    magicDSRun.state = 'goto_censored' OR magicDSRun.state = 'goto_restored'
GROUP BY magic_ds_id
HAVING COUNT(component) = 0

