SELECT 
    int_id,
    rcInterest.state AS state,
    dest_id,
    name AS dest_name,
    rcDestination.state as dest_state,
    dist_group,
    filter,
    stage,
    clean,
    target_id,
    distTarget.state as target_state
FROM rcInterest 
    JOIN distTarget USING(target_id)
    JOIN rcDestination USING(dest_id)
