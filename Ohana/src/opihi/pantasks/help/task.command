
   command (arg0) (arg1) ...

   Define the rule for constructing a job command for a task.  The
   arguments are UNIX command and the collection of command arguments.
   The arguments may include references to Opihi variables.  The task
   command may be included at the top level of the task, or it may be
   defined within the task exec macro.  In the latter case, the
   command is constructed only when the exec macro is run, every job
   construction interval.  Thus, the command arguments may depend on
   the result of the exec macro operation.   

   See also:
   task
   task.exec
