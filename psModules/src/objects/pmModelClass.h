/* @file  pmModelClass.h
 *
 * The object model function types are desined to allow for the flexible addition of new
 * object models. Every object model, with parameters represented by pmModel, has an
 * associated set of functions which provide necessary support operations.  A These
 * functions allow the programmer to select the approriate function or property for a
 * specific object model class.
 *
 * Every model instance belongs to a class of models, defined by the value of the
 * pmModelType type entry. Various functions need access to information about each of the
 * models. Some of this information varies from model to model, and may depend on the
 * current parameter values or other data quantities. In order to keep the code from
 * requiring the information about each model to be coded into the low-level fitting
 * routines, we define a collection of functions which allow us to abstract this type of
 * model-dependent information. These generic functions take the model type and return the
 * corresponding function pointer for the specified model. Each model is defined by
 * creating this collection of specific functions, and placing them in a single file for
 * each model. We define the following structure to carry the collection of information
 * about the models.
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-11-27 03:14:57 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_MODEL_CLASS_H
# define PM_MODEL_CLASS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct {
    char *name;
    int nParams;
    bool useReff;
    pmModelFunc          modelFunc;
    pmModelFlux          modelFlux;
    pmModelRadius        modelRadius;
    pmModelSetFWHM       modelSetFWHM;
    pmModelLimits        modelLimits;
    pmModelGuessFunc     modelGuess;
    pmModelFromPSFFunc   modelFromPSF;
    pmModelParamsFromPSF modelParamsFromPSF;
    pmModelFitStatusFunc modelFitStatus;
    pmModelSetLimitsFunc modelSetLimits;
} pmModelClass;

// allocate a pmModelClass to hold nModels entries
pmModelClass *pmModelClassAlloc (int nModels);

//
bool psMemCheckModelClass(psPtr ptr);

// initialize the internal (static) model class with the default models
bool pmModelClassInit (void);

// free the internal (static) model class
void pmModelClassCleanup (void);

// add a new model class to the collection of model classes
void pmModelClassAdd (pmModelClass *modelClass);

// get the specified model class
pmModelClass *pmModelClassSelect (pmModelType type);

// This function returns the number of parameters used by the listed function.
int pmModelClassParameterCount (pmModelType type);

// This function returns the user-space model names for the specified model type.
char *pmModelClassGetName (pmModelType type);

// This function returns the internal model type code for the user-space model names.
pmModelType pmModelClassGetType (const char *name);

/// Set parameter limits for all models
void pmModelClassSetLimits(pmModelLimitsType type);

// write keywords to header definining the model type values used by this program
bool pmModelClassWriteHeader(psMetadata *header);
// create a lookup table for translating input model type values to local model type values
bool pmModelClassReadHeader(psMetadata *header);
// translate input model type value to local value
pmModelType pmModelClassGetLocalType(pmModelType inputType);

/// @}
# endif /* PM_MODEL_CLASS_H */
