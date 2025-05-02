DELETE FROM relStack USING relStack, ippRelease, stackRun
WHERE relStack.rel_id = ippRelease.rel_id
    AND relStack.stack_id = stackRun.stack_id
