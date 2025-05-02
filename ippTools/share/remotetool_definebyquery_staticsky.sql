SELECT stage_id FROM

( SELECT sky_id AS stage_id
    FROM staticskyRun
WHERE
staticskyRun.state = 'new'
-- where hook %s
GROUP BY sky_id
) AS W

LEFT JOIN

(SELECT remote_id,remoteRun.stage,remoteRun.label,remoteComponent.stage_id,remoteRun.state
   FROM remoteRun
   JOIN remoteComponent USING(remote_id)
WHERE
remoteRun.stage = 'staticsky' AND remoteComponent.state != 'retry'
-- where hook %s
) AS R
USING(stage_id)
WHERE R.remote_id IS NULL
