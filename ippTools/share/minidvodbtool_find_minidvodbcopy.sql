SELECT
    minidvodbCopy.*,
    minidvodbRun.minidvodb_path
FROM minidvodbCopy
    JOIN minidvodbRun using (minidvodb_id)


