SELECT stage_id FROM

( SELECT warp_id AS stage_id, count(warpSkyCellMap.skycell_id) AS N
    FROM warpRun
    LEFT JOIN warpSkyCellMap USING(warp_id)
WHERE
warpRun.state = 'new'
-- where hook %s
GROUP BY warp_id
HAVING N != 0
) AS W

LEFT JOIN

(SELECT remote_id,remoteRun.stage,remoteRun.label,remoteComponent.stage_id,remoteRun.state 
   FROM remoteRun 
   JOIN remoteComponent USING (remote_id)
WHERE
remoteRun.stage = 'warp' and remoteComponent.state != 'retry'
-- where hook %s
) AS R
USING (stage_id)
WHERE R.remote_id IS NULL

