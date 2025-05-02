SELECT
    vpRun.*,
    rawImfile.class_id,
    rawImfile.uri
FROM vpRun
    JOIN rawExp USING(exp_id)
    JOIN rawImfile USING(exp_id)
    LEFT JOIN vpProcessedCell USING(vp_id, class_id)
WHERE rawImfile.video_cells
    AND vpRun.state = 'new'
    AND vpProcessedCell.vp_id IS NULL

