SELECT 
       sass_id,
       data_group,
       projection_cell,
       tess_id,
       filter
       FROM stackAssociation
RIGHT JOIN (
      SELECT 
      	     data_group,
	     tess_id,
	     filter,
	     CASE WHEN LOCATE('.',skycell_id,9) > 0 THEN
       	     	  SUBSTRING_INDEX(skycell_id,'.',2) ELSE
	    	  SUBSTRING_INDEX(skycell_id,'.',1) END
       	     AS projection_cell
       FROM stackRun 
       WHERE stackRun.stack_id = @STACK_ID@ 
       	     ) AS RUN USING (data_group,tess_id,filter,projection_cell) LIMIT 1