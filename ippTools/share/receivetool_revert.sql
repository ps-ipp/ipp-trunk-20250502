DELETE FROM receiveResult
USING receiveResult, receiveFile, receiveFileset, receiveSource
WHERE receiveResult.file_id = receiveFile.file_id
    AND receiveFile.fileset_id = receiveFileset.fileset_id
    AND receiveFileset.source_id = receiveSource.source_id
    AND receiveResult.fault != 0
