SELECT 
       chipProcessedImfile.*, 
       rawExp.filelevel ,
       rawExp.camera,
       rawExp.telescope,
       rawExp.filter
FROM chipProcessedImfile
JOIN rawExp
USING (exp_id)
