UPDATE rcInterest 
JOIN distTarget USING(target_id)
JOIN rcDestination USING(dest_id)
SET rcInterest.state = '%s'
