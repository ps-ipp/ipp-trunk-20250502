/* @file  pmFPAview.h
 * @brief Tools to manipulate the FPA structure elements.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.18 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-11-11 00:03:46 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

/// @addtogroup Camera Camera Layout
/// @{

#ifndef PM_FPA_FILE_DEFINE_H
#define PM_FPA_FILE_DEFINE_H

// load the pmFPAfile information from the camera configuration data
//
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.  Multiple file rules of the same name are permitted
// if multiple is true.
pmFPAfile *pmFPAfileDefineInput (const pmConfig *config, pmFPA *fpa, char *cameraName, const char *name);

// load the pmFPAfile information from the camera configuration data
//
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.
// Define an output pmFPAfile
pmFPAfile *pmFPAfileDefineOutput(const pmConfig *config, // Configuration
                                 pmFPA *fpa, // Optional FPA to bind
                                 const char *name // Name of file rule
    );

/// Same as pmFPAfileDefineOutput, but binds to the fpa in the provided file
pmFPAfile *pmFPAfileDefineOutputFromFile(const pmConfig *config, // Configuration
                                         pmFPAfile *file, // File to bind FPAs, or NULL
                                         const char *name // Name of file rule
    );

/// Define the FPA file using the provided camera and format names.
///
/// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
/// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileDefineOutputForFormat(const pmConfig *config, // Configuration
                                          pmFPA *fpa, // Optional FPA to bind
                                          const char *name, // Name of file rule
                                          psString cameraName, // Name of camera configuration to use
                                          psString formatName // Name of camera format to use
    );

// look for the given argname on the argument list.  find the give filename from the file rules
//
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileDefineFromArgs(bool *found, pmConfig *config, const char *filename, const char *argname);

// look for the given argname on the argument list; bind the associated files to the specified
// fpa.  these are, eg, mask or weight images.
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileBindFromArgs(bool *found, pmFPAfile *input, pmConfig *config, const char *filename, const char *argname);

/// Define a file based on the filenames in the RUN metadata in the configuration
///
/// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
/// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileDefineFromRun(
    bool *found,                        ///< Found files?
    pmFPAfile *bind,                    ///< File to which to bind, or NULL
    pmConfig *config,                   ///< Configuration
    const char *filename                ///< Name of file
    );

/// Define multiple files based on the filenames in the RUN metadata in the configuration
///
/// An array of the files defined is returned
psArray *pmFPAfileDefineMultipleFromRun(
    bool *found,                        ///< Found files?
    psArray *bind,                      ///< Files to which to bind, or NULL
    pmConfig *config,                   ///< Configuration
    const char *filename                ///< Name of file
    );

// find the file associated with the argname & generate a pmFPAfile for it based on the filerule
pmFPAfile *pmFPAfileDefineNewConfig(
    bool *success,                      ///< Found files?
    pmConfig **outConfig, 		///< output configuration for this file
    pmConfig *sysConfig, 		///< existing system config info
    const char *filename, 		///< name of filerule
    const char *argname			///< argument entry
    );


// look for the given argname on the argument list.  find the give filename from the file rules
//
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileDefineFromConf (bool *found, const pmConfig *config, const char *filename);

// look for the given argname on the argument list.  find the give filename from the file rules
//
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileDefineFromDetDB (bool *found, const pmConfig *config, const char *filename,
                                     pmFPA *input, pmDetrendType type);

// create a new output pmFPAfile based on an existing FPA
//
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileDefineFromFPA (const pmConfig *config, pmFPA *src, int xBin, int yBin, const char *filename);

/// Same as pmFPAfileDefineFromFPA, except it uses an FPA file instead of an FPA
///
/// The new pmFPAfile is inserted into the config->files metadata, freed and returned; so that the user does
/// not have to (and should not!) free the result.
pmFPAfile *pmFPAfileDefineFromFile(const pmConfig *config, // Configuration
                                   pmFPAfile *src, // Source file for this file
                                   int xBin, int yBin, // Binning for this file
                                   const char *filename // Name of file rule
    );


// create a new output pmFPAfile based on an existing FPA
// only valid for pmFPAfile->mode == WRITE (or internal?)
//
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileDefineNewCamera (const pmConfig *config, const char *filename);

/// Create a new output pmFPAfile for a skycell of the default camera
///
/// The new pmFPAfile is inserted into the config->files metadata, freed and returned; so that the user does
/// not have to (and should not!) free the result.
pmFPAfile *pmFPAfileDefineSkycell(const pmConfig *config, ///< Configuration data
                                  pmFPA *fpa, ///< FPA to which to bind
                                  const char *filename ///< Output (root) filename
    );


/// Create a new output pmFPAfile based upon a chip mosaic of an existing FPA
///
/// The new pmFPAfile is inserted into the config->files metadata, freed and returned; so that the user does
/// not have to (and should not!) free the result.
pmFPAfile *pmFPAfileDefineChipMosaic(const pmConfig *config, ///< Configuration data
                                     pmFPA *src, ///< Source FPA
                                     const char *filename ///< Output (root) filename
                                    );

/// Create a new output pmFPAfile based upon an FPA mosaic of an existing FPA
///
/// The new pmFPAfile is inserted into the config->files metadata, freed and returned; so that the user does
/// not have to (and should not!) free the result.
pmFPAfile *pmFPAfileDefineFPAMosaic(const pmConfig *config, ///< Configuration data
                                    pmFPA *src, ///< Source FPA
                                    const char *filename ///< Output (root) filename
                                   );

// create a file with the given name, assign it type "INTERNAL", and supply it with an image
// of the requested dimensions. (image only, mask and weight are ignored)
///
/// The new pmFPAfile is inserted into the config->files metadata, freed and returned; so that the user does
/// not have to (and should not!) free the result.
pmReadout *pmFPAfileDefineInternal(psMetadata *files, const char *name, int Nx, int Ny, int type);

// delete the INTERNAL file of the given name (if it exists)
bool pmFPAfileDropInternal(psMetadata *files, const char *name);

// look for the given argname on the argument list.  find the give filename from the file rules
//
// Note that the returned pmFPAfile is a view only, so it should not be freed by the caller --- the only
// reference count is held by the config->files metadata.
pmFPAfile *pmFPAfileDefineSingleFromArgs(bool *found, pmConfig *config, const char *filename,
                                         const char *argname, int entry);

// Select or construct the requested readout.  If the named entry does not exist, generate it based
// on the specified fpa and binning.  We have 4 possibilities: (INTERNAL or I/O file) and (exists or
// not).  This call is used after all user-requested pmFPAfiles have been generated.  A missing
// pmFPAfile is being used internally.
pmReadout *pmFPAGenerateReadout(const pmConfig *config, // configuration information
                                const pmFPAview *view, // select background for this entry
                                const char *name, // name of internal/external file
                                const pmFPA *fpa, // use this fpa to generate
                                const psImageBinning *binning,
				int index
    );

/// @}
# endif
