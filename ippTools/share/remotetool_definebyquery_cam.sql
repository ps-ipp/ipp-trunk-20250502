SELECT stage_id FROM

( SELECT cam_id AS stage_id
    FROM camRun
WHERE
camRun.state = 'new'
-- where hook %s
GROUP BY cam_id
) AS W

LEFT JOIN

(SELECT remote_id,remoteRun.stage,remoteRun.label,remoteComponent.stage_id,remoteRun.state
   FROM remoteRun
   JOIN remoteComponent USING (remote_id)
WHERE
remoteRun.stage = 'camera' and remoteComponent.state != 'retry'
-- where hook %s
) AS R
USING (stage_id)
WHERE R.remote_id IS NULL
