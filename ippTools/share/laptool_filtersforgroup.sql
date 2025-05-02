SELECT DISTINCT filter
FROM lapGroup JOIN lapRun USING(seq_id, tess_id, projection_cell)
