#ifndef PM_CONFIG_COMMAND_H
#define PM_CONFIG_COMMAND_H

/// Extend a command-line to include the necessary database flags
///
/// The command-line is extended with -dbserver, -dbname, -dbuser, -dbpassword
bool pmConfigDatabaseCommand(psString *command, ///< Command to extend
                             const pmConfig *config ///< Configuration
                            );

/// Extend a command-line to propagate the trace flags
///
/// The command-line is extended with -trace
bool pmConfigTraceCommand(psString *command ///< Command to extend
                         );
#endif
