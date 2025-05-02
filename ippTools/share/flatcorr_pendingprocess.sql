SELECT 
  corr_id,
  det_type,
  dvodb,
  camera,
  filter,
  state,
  make_corr,
  workdir,
  label,
  reduction,
  region,
  cam_count,
  add_count
FROM 
  (SELECT 
     flatcorrRun.*,
     flatcorrCamLink.corr_id as cam_corr_id,
     count(flatcorrCamLink.cam_id) as cam_count
   FROM flatcorrRun
   JOIN flatcorrCamLink
  USING (corr_id)
  WHERE flatcorrCamLink.include = 1
   GROUP BY
     flatcorrCamLink.corr_id) AS t1
LEFT JOIN
  (SELECT 
     flatcorrAddstarLink.corr_id as add_corr_id,
     count(flatcorrAddstarLink.add_id) as add_count
   FROM flatcorrAddstarLink
   JOIN addRun
   USING (add_id)
   WHERE addRun.state = 'full'
     AND flatcorrAddstarLink.include = 1
   GROUP BY
       flatcorrAddstarLink.corr_id) AS t2
ON t1.cam_corr_id = t2.add_corr_id
WHERE cam_count = add_count
AND state = 'new'
