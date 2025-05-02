SELECT receiveFileset.*,
    receiveSource.source,
    receiveSource.product,
    receiveSource.workdir
FROM receiveFileset
JOIN receiveSource USING(source_id)
LEFT JOIN receiveFile USING(fileset_id)
WHERE receiveSource.state = 'enabled'
    AND receiveFileset.state = 'reg'
    AND receiveFileset.fault = 0
    AND receiveFile.fileset_id IS NULL
