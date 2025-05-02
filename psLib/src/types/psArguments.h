/** @file  psArguments.h
 *
 *  @brief Contains operations for parsing command line input arguments.
 *
 *  @author David Robbins, MHPCC
 *
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-06-01 03:37:19 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_ARGUMENTS_H
#define PS_ARGUMENTS_H

/// @addtogroup SysUtils
/// @{

#include "psMetadata.h"

/** Implements the various verbosity controls.
 *
 *  Arguments shall be removed from the argument list as they are processed.
 *
 *  @return int:        The resultant logging level.
 */
int psArgumentVerbosity(
    int *argc,                         ///< number of arguments
    char **argv                        ///< the argument list
);

/** Checks for an argument and returns its index position if found.
 *
 *  @return int:        The index of the element in the argument list, otherwise 0.
 */
int psArgumentGet(
    int argc,                          ///< number of arguments
    char **argv,                       ///< the argument list
    const char *arg                    ///< the specified argument to match
);

/** Removes from the argument list the argument whose index is argnum.
 *
 *  The number of entries in the argument list shall be decremented.
 *
 *  @return bool:       True if the argnum is in the argument list, otherwise false.
 */
bool psArgumentRemove(
    int argnum,                        ///< the argument to remove
    int *argc,                         ///< number of arguments
    char **argv                        ///< the argument list
);

/** Parses the command line arguments into a metadata container of arguments.
 *
 *  The input arguments shall contain the list of possible arguments as the keywords providing
 *  the default values.  As matching arguments are found on the command line, the values shall
 *  be read into the arguments metadata, with the appropriate type.  The arguments and their
 *  values shall be removed from the list of command line arguments as they are processed.
 *
 *  @return bool:       False if any argument was encountered that is not present in arguments.
 */
bool psArgumentParse(
    psMetadata *arguments,             ///< metadata container for arguments
    int *argc,                         ///< number of arguments
    char **argv                        ///< the argument list
);

/** Prints to stdout a guide to the command-line arguments.      */
void psArgumentHelp(
    psMetadata *arguments              ///< metadata container for arguments
);

/** Prints to stdout a simplifed guide to the command-line arguments.      */
void psArgumentHelpSimple(
    FILE *stream,                      ///< FILE* to write too
    psMetadata *arguments              ///< metadata container for arguments
);

/** Macro to implement some generic command line arguments.  A package-
 *  specific routine called pkgnameVersionLong() is presumed to exist.
 */
#define PS_ARGUMENTS_GENERIC( pkgname, config, argc, argv )   \
  { int N= psArgumentGet (argc, argv, "-version");                        \
    if (N) {                                                              \
      psString version;                                                   \
      version = pkgname ## VersionLong();                                 \
      fprintf (stdout, "%s\n", version);                                  \
      psFree (version);                                                   \
      version = psModulesVersionLong();                                   \
      fprintf (stdout, "%s\n", version);                                  \
      psFree (version);                                                   \
      version = psLibVersionLong();                                       \
      fprintf (stdout, "%s\n", version); psFree (version);                \
      exit (0);                                                           \
    }                                                                     \
    N = psArgumentGet(argc, argv, "-dumpconfig");                         \
    if (N) {                                                              \
      if (N<argc-1) {							  \
        psArgumentRemove(N, &argc, argv);                                 \
        psMetadataAddStr(config->arguments,                               \
                         PS_LIST_TAIL, "DUMP_CONFIG", PS_META_REPLACE,    \
                         "Filename for configuration dump", argv[N]);     \
        psArgumentRemove(N, &argc, argv);                                 \
      }                                                                   \
      else {                                                              \
        fprintf(stderr,"-dumpconfig requires a filename argument\n");     \
        exit(-1);                                                         \
      }                                                                   \
    }                                                                     \
  }

/** Macro to implement handling of '-threads' argument.  A package-
 *  specific routine called pkgnameSetThreads(int nThreads) is
 *  presumed to exist.
 */
#define PS_ARGUMENTS_THREADS( pkgname, config, argc, argv )   \
  { int N= psArgumentGet(argc, argv, "-threads");                           \
    if (N) {                                                                \
      psArgumentRemove(N, &argc, argv);                                     \
      int nThreads = atoi(argv[N]);                                         \
      psMetadataAddS32(config->arguments, PS_LIST_TAIL, "NTHREADS", 0, "number of "#pkgname" threads", nThreads); \
      psArgumentRemove(N, &argc, argv);                                     \
      psThreadPoolInit (nThreads);                                          \
    }                                                                       \
    pkgname ## SetThreads();                                                \
  }


/// @}
#endif // #ifndef PS_ARGUMENTS_H
