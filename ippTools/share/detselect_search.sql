-- this query needs to use the same fields in both of the tables in
-- the union statement, but we need to report the
-- detRunSummary.iteration in the first case, and this is missing in
-- the second case.

SELECT DISTINCT
    det_id,
    good_iteration as iteration,
    filelevel
FROM
    (SELECT DISTINCT
	detRun.*,
	detRunSummary.iteration as good_iteration
    FROM detRun
    JOIN detRunSummary
        USING(det_id)
    WHERE
       detRun.mode  = 'master'
       AND detRunSummary.accept = 1
    UNION
    SELECT DISTINCT
	detRun.*,
	detRun.iteration as good_iteration
    FROM detRun
    WHERE
       (detRun.mode  = 'register' OR detRun.mode = 'correction')
    ) as Foo
WHERE
    (state = 'stop')
