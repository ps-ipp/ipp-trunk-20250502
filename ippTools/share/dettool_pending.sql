-- dettool -pending : not sure this is still used (EAM : 2008.07.10)

SELECT
   rawExp.*
 FROM rawExp
 LEFT JOIN detInputExp
   ON rawExp.exp_id = detInputExp.exp_id
 WHERE
    detInputExp.exp_id IS NULL
    AND rawExp.object != 'object'
