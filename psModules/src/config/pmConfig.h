/*  @file pmConfig.h
 *  @brief Configuration functions
 *
 *  @author Paul Price, IfA
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.44 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 19:41:50 $
 *  Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONFIG_H
#define PM_CONFIG_H

/// @addtogroup Config Configuration System
/// @{

/// Sources for recipes.
///
/// Defines what recipe sources have been read.  This allows us to read recipes from different sources as they
/// become available.  For example, we may not have access to the camera configuration until we have read a
/// FITS file.  We allow symbolic links, which means the user can specify on the command-line the name of a
/// recipe that's defined elsewhere, instead of typing the entire filename.  This structure is private to
/// psModules --- there is no need for the user to know about it.
typedef enum {
    PM_RECIPE_SOURCE_NONE        = 0x00, ///< None yet
    PM_RECIPE_SOURCE_SYSTEM      = 0x01, ///< System configuration
    PM_RECIPE_SOURCE_CAMERA      = 0x02, ///< Camera configuration
    PM_RECIPE_SOURCE_CL          = 0x04, ///< Command-line
    PM_RECIPE_SOURCE_SYMBOLIC    = 0x14, ///< Symbolic link, specified on command-line
    PM_RECIPE_SOURCE_ALL         = 0xff  ///< All sources
} pmRecipeSource;

/// Configuration information
///
/// This structure stores the configuration information: user, site, system, camera and recipe configuration, the
/// command-line arguments, the pmFPAfiles used, and the database handle.
typedef struct {
    psMetadata *user;                   ///< User configuration
    psMetadata *site;                   ///< Site configuration
    psMetadata *system;                 ///< System configuration
    psMetadata *camera;                 ///< Camera specification
    psString cameraName;                ///< Camera name
    psMetadata *format;                 ///< Camera format description
    psString formatName;                ///< Camera format name
    psMetadata *recipes;                ///< Recipes for processing
    psMetadata *recipesCamera;          ///< Recipes for processing
    psMetadata *arguments;              ///< Processed command-line arguments
    psMetadata *files;                  ///< pmFPAfiles used for analysis
    psDB *database;                     ///< Database handle
    const char *defaultRecipe;          ///< name of top-level recipe for this program
    psString program;                   ///< Name of program
    // Private members
    pmRecipeSource recipesRead;         ///< Which recipe sources have been read
    psMetadata *recipeSymbols;          ///< Where each recipe came from
    int traceFD;                        ///< File descriptor for trace messages
    int logFD;                          ///< File descriptor for log messages
    psS64 sourceId;                    ///< Database source id for output file
    psS64 imageId;                     ///< Database image id for output file
} pmConfig;

/// Allocator for pmConfig
pmConfig *pmConfigAlloc(void);

/// Set static configuration information
///
/// The search path for the configuration files is a local static variable, set by this function.
void pmConfigSet(const char *path ///< Search paths for configuration files; colon-delimited directories
                );

/// Free static memory used in the configuration system
void pmConfigDone(void);

/// Read configuration information from the command line.
///
/// pmConfigRead loads the user configuration (the file name is specified by "-ipprc FILE" on the
/// command-line, the IPPRC environment variable, or it is $HOME/.ipprc).  The configuration search path is
/// set. The camera configuration is loaded if it is specified on the command line ("-camera
/// CAMERA_FILE"). Recipes specified on the command line ("-recipe RECIPE_NAME RECIPE_SOURCE") are also
/// loaded.  These command-line arguments are removed from from the command-line, to simplify parsing.  The
/// psLib log, trace and time setups are also performed if specified in the user configuration.
pmConfig *pmConfigRead(int *argc,       ///< Number of command-line arguments
                       char **argv, ///< Array of command-line arguments
                       const char *defaultRecipe ///< name of top-level recipe for this program
                      );

/// Read a configuration file
///
/// Read a metadata configuration file into the supplied metadata.  Produce an error and
/// return false if there's a problem.
bool pmConfigFileRead(psMetadata **config, ///< Config to output
                      const char *name, ///< Name of file
                      const char *description ///< Description of file
    );

/// Ingest a configuration file
///
/// Ingest a metadata configuration file into the supplied metadata item, if required.  Produce an error and
/// return false if there's a problem.
bool pmConfigFileIngest(psMetadataItem *item, // Item into which to read file
                        const char *description // Description, for error messages
    );

/// Validate a header against the camera format
///
/// Given a FITS header (the PHU header), check it against the RULE metadata contained within the camera
/// format; return found = true if it matches. return false on serious errors
bool pmConfigValidateCameraFormat(bool *valid,
                                  const psMetadata *cameraFormat, ///< Camera format containing the RULE
                                  const psMetadata *header // FITS header for the PHU
                                 );

/// Determine the camera format (and camera if unknown) from examining the header
///
/// Given a FITS header, check it against all known cameras (unless we already know which camera, from
/// pmConfigRead) and all known formats for those cameras in order to identify which is appropriate.  The
/// first matching format is accepted; further matches produce warnings.  The accepted camera is saved in the
/// configuration.  The accepted format is returned.
psMetadata *pmConfigCameraFormatFromHeader(psMetadata **camera, // selected camera (or meta-camera)
                                           psString *formatName, // selected format name
                                           psString *cameraName, // selected camera name
                                           pmConfig *config, ///< The configuration
                                           const psMetadata *header, ///< The FITS header
                                           bool readRecipes ///< optionally read the recipes as well as the format
    );

/// Return the camera configuration specified by name
///
/// Given a camera name, returns the camera configuration metadata.
psMetadata *pmConfigCameraByName(pmConfig *config, ///< The configuration
                                 const char *cameraName ///< The camera name header
                                );

/// Derive a value from the user or site configuration
///
/// The value in the user configuration takes precedence.  Returns NULL if the value isn't present in either,
/// or has the wrong type.
psMetadataItem *pmConfigUserSite(const pmConfig *config, // Configuration
                                 const char *name, // Name of value
                                 psDataType type // Expected type
    );


/// Setup the database
///
/// Initialise the database connection using the DBSERVER, DBNAME, DBUSER, DBPASSWORD values provided in the
/// site configuration.  Stores the database handle in the configuration, and also returns it.
psDB *pmConfigDB(pmConfig *config       ///< Configuration
                );

/// Make the supplied header conform to the nominated camera format.
///
/// Given a FITS header, make it conform to the RULE in the specified camera format.  This is useful for
/// switching between formats, or generating fake data that must be recognised by
/// pmConfigCameraFormatFromHeader.
bool pmConfigConformHeader(psMetadata *header, ///< Header to conform
                           const psMetadata *format ///< Camera format
                          );

/// Read the command-line for files (or a text file containing a list of files)
///
/// Given the 'file' and 'list' arguments (e.g., "-file" and "-list"), find the arguments associated with
/// these words and interpret them as lists of files.  Return an array of the resulting filenames.
psArray *pmConfigFileSets(int *argc,    ///< Number of arguments (I/O)
                          char **argv,  ///< Array of arguments
                          const char *file, ///< CL argument specifying a filename
                          const char *list ///< CL argument specifying a text file with a list of filenames
                         );

/// Stuff associated files from the command-line into a metadata
///
/// Calls pmConfigFileSets to parse the command line for filenames (or a list which provides filenames), and
/// stuffs the array of filenames into the metadata under "name".
bool pmConfigFileSetsMD(psMetadata *metadata, ///< Metadata into which to stuff the array
                        int *argc,    ///< Number of arguments (I/O)
                        char **argv,  ///< Array of arguments
                        const char *name, ///< Name for array in the metadata
                        const char *file, ///< CL argument specifying a filename
                        const char *list ///< CL argument specifying a text file with a list of filenames
                       );

/// Convert the supplied name, create a new output psString
psString pmConfigConvertFilename(
    const char *filename,               ///< file path/URI
    const pmConfig *config,             ///< configuration
    bool create,                        ///< create the file if it doesn't exist
    bool trunc                          ///< truncate the file (if it exists)
);

/// Set whether all config parameters are read on startup
bool pmConfigReadParamsSet(bool newReadCameraConfig // Desired mode for camera configuration reading
                          );

/// Get the file rule of interest
///
/// Look up the name of the set of file rules to use, get that set from the system configuration, and return the
/// appropriate rule from the set.
psMetadata *pmConfigFileRule(const pmConfig *config, ///< Configuration
                             const psMetadata *camera, ///< Camera configuration of interest
                             const char *name ///< Name of rule to read
    );

// look up the specified fitstype, interpolating the file if needed
psMetadata *pmConfigFitsType (const pmConfig *config, const psMetadata *camera, const char *fitsType);

/// @}
#endif
