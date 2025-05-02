SELECT warp_id,warp_state,W.skycell_id,stack_id,stack_state,S.seq_id
  FROM
( SELECT warp_id,skycell_id,seq_id,warpRun.state AS warp_state
   FROM warpSkyfile
   JOIN warpRun USING(warp_id)
   JOIN fakeRun USING(fake_id)
   JOIN camRun USING(cam_id)
   JOIN chipRun USING(chip_id)
   JOIN rawExp USING(exp_id)
   JOIN lapExp USING(exp_id,chip_id)
   JOIN lapRun USING(lap_id)
   WHERE 1
   @WHERE@
) AS W
LEFT JOIN
( SELECT stack_id,skycell_id,seq_id,stackRun.state AS stack_state
   FROM stackRun
   JOIN stackAssociationMap USING(stack_id)
   JOIN lapRun ON (stackAssociationMap.sass_id = lapRun.final_sass_id)
   ) AS S
ON (    (W.seq_id = S.seq_id)
    AND (W.skycell_id = S.skycell_id) )


