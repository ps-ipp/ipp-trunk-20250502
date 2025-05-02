-- dettool -definebydetrun

INSERT INTO detInputExp
   SELECT
       %lld,
       0,
       detResidExp.exp_id,
       detResidExp.accept
   FROM detResidExp
   JOIN rawExp
       USING(exp_id)
   WHERE det_id = %lld
   AND iteration = %d
