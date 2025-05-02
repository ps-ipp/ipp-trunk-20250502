SELECT
    mergedvodbProcessed.*,
    mergedvodbRun.mergedvodb_path
FROM mergedvodbProcessed
JOIN mergedvodbRun
    USING(merge_id)

