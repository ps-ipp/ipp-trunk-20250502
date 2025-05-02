SELECT
    receiveFileset.fileset_id,
    receiveFileset.fileset,
    receiveFileset.dbinfo,
    receiveSource.status_product,
    receiveSource.ds_dbname,
    receiveSource.ds_dbhost
FROM receiveFileset 
    JOIN receiveSource USING(source_id)
    JOIN receiveFile USING(fileset_id)
    LEFT JOIN receiveResult USING(file_id)
WHERE receiveSource.state = 'enabled'
    AND receiveFileset.state = 'new' 
    AND receiveFileset.fault = 0
GROUP BY fileset_id 
HAVING COUNT(receiveFile.file_id) = COUNT(receiveResult.file_id) 
    AND SUM(receiveResult.fault) = 0
