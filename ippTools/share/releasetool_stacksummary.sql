SELECT DISTINCT sass_id,
       stackAssociation.tess_id,
       stackAssociation.projection_cell,
       stackAssociation.filter,
       stackSummary.path_base,
       stackAssociation.data_group,
       survey.surveyName,
       ippRelease.release_name,
       ippRelease.rel_id

FROM relStack 
    JOIN ippRelease USING(rel_id)
    JOIN survey USING(surveyID)
    JOIN stackAssociationMap USING(stack_id) 
    JOIN stackAssociation USING(sass_id, filter)
    JOIN stackSummary using(sass_id, projection_cell)

-- WHERE rel_id = 37 and skycell_id like 'skycell.0635.045' and filter = 'r.00000'\G
-- WHERE rel_id = 37 and projection_cell = 'skycell.0635' and filter = 'r.00000'\G
