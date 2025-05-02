SELECT DISTINCT dqstatsRun.*,rawExp.camera from dqstatsRun
       JOIN dqstatsContent USING(dqstats_id)
       JOIN rawExp USING(exp_id)
       	WHERE dqstatsRun.state = 'new' AND (dqstatsRun.fault = 0)