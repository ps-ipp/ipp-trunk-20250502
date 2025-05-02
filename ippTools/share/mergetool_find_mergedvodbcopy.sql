SELECT
    mergedvodbCopy.*,
    mergedvodbRun.mergedvodb_path
FROM mergedvodbCopy
    JOIN mergedvodbRun using (mergedvodb_id)


