INSERT 
    INTO pstampAccessLevel 
        SELECT proj_id, %d, %f, %f FROM pstampProject
        WHERE name = '%s'
