#ifndef PM_CONFIG_RUN_H
#define PM_CONFIG_RUN_H

#include <pslib.h>
#include <pmConfig.h>
#include <pmFPAfile.h>

/// Add a file to the list of files read in the run-time information
bool pmConfigRunFileAddRead(
    pmConfig *config,                   ///< Configuration
    const pmFPAfile *file               ///< File to add
    );

/// Add a filename to the list of files read in the run-time information
bool pmConfigRunFilenameAddRead(
    pmConfig *config,                   ///< Configuration
    const char *description,            ///< Description of file
    const char *name                    ///< Name of file
    );

/// Add a file to the list of files written in the run-time information
bool pmConfigRunFileAddWrite(
    pmConfig *config,                   ///< Configuration
    const pmFPAfile *file               ///< File to add
    );

/// Add a filename to the list of files written in the run-time information
bool pmConfigRunFilenameAddWrite(
    pmConfig *config,                   ///< Configuration
    const char *description,            ///< Description of file
    const char *name                    ///< Name of file
    );

/// Retrieve file names for a symbolic file from the run-time information
psArray *pmConfigRunFileGet(
    pmConfig *config,                   ///< Configuration
    const char *name                    ///< Name of symbolic file (pmFPAfile)
    );

/// Add the command line to the run-time information
bool pmConfigRunCommand(
    pmConfig *config,                   ///< Configuration
    int argc, char **argv               ///< Command line arguments
    );

/// Record the random number generator seed in the run-time information
bool pmConfigRunSeed(
    pmConfig *config,                   ///< Configuration
    psU64 seed                          ///< RNG seed
    );

#endif

