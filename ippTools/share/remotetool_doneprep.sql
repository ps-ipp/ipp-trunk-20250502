SELECT * FROM
 (SELECT
 remote_id,
 remoteRun.path_base,
 stage,
 count(stage_id) as Ndone
FROM remoteRun
JOIN remoteComponent
USING (remote_id)
WHERE
((remoteComponent.state = 'prep_done') OR (remoteComponent.state = 'prep_fail') OR (remoteComponent.state = 'fail'))
-- and hook %s
GROUP BY remote_id)
AS DONE

JOIN

(SELECT
remote_id,
count(stage_id) as Njobs
FROM remoteRun
JOIN remoteComponent
USING (remote_id)
-- where hook %s
GROUP BY remote_id)
AS JOBS

using (remote_id)

where Ndone = Njobs
