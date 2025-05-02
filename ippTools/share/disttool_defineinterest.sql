INSERT INTO rcInterest
    SELECT 0, rcDestination.dest_id, distTarget.target_id, '@STATE@'
    FROM distTarget
        JOIN rcDestination
        LEFT JOIN rcInterest USING(dest_id, target_id)
    WHERE (int_id IS NULL)
