DELETE FROM rcDSFileset
USING rcDSFileset, distRun
WHERE distRun.dist_id = rcDSFileset.dist_id
    AND rcDSFileset.fault != 0
