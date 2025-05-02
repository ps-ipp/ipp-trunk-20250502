SELECT DISTINCT lapGroup.*,
    lapRun.dist_group
FROM lapGroup JOIN lapRun USING(seq_id, tess_id, projection_cell)
WHERE lapGroup.state ='new' AND lapGroup.fault = 0
