-- this needs to join a table of chipRuns and a table of camRuns based on exp_id

SELECT * FROM
(
 SELECT
     exp_id,
     chip_id
 FROM chipRun
 JOIN rawExp
      USING(exp_id)
 WHERE
      chipRun.state = 'full'
 -- WHERE HOOK for chip:
  AND %s
) AS myChip
JOIN
(
SELECT
    exp_id,
    camRun.chip_id as cam_chip_id,
    camRun.cam_id,
    camRun.state,
    camRun.workdir,
    camRun.workdir_state,
    camRun.label,
    camRun.data_group,
    camRun.dist_group,
    camRun.reduction,
    camRun.dvodb,
    camRun.software_ver,
    camRun.note
FROM camRun
JOIN chipRun
     USING(chip_id)
JOIN rawExp
     USING(exp_id)
WHERE
     camRun.state = 'full'
-- WHERE HOOK for cam:
  AND %s
) AS myCam
USING(exp_id)
