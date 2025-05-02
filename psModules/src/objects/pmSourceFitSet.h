/* @file  pmSourceFitSet.h
 *
 * @author EAM, IfA; GLG, MHPCC
 *
 * @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_SOURCE_FIT_SET_H
# define PM_SOURCE_FIT_SET_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct {
    psArray *modelSet;
    psArray *paramSet;
    psArray *derivSet;
    int nParamSet;
    pthread_t thread;
} pmSourceFitSetData;

// use this function to init the fit sets based on the number of threads
bool pmSourceFitSetInit (int nThreads);
void pmSourceFitSetDone (void);

// initialize data for a group of object models
pmSourceFitSetData *pmSourceFitSetDataAlloc (psArray *modelSet);
bool psMemCheckSourceFitSetData(psPtr ptr);

// functions for selecting the FitSet corresponding to the current thread
pmSourceFitSetData *pmSourceFitSetDataSet (psArray *modelSet);
pmSourceFitSetData *pmSourceFitSetDataGet (void);
void pmSourceFitSetDataClear (void);

// function used to set limits for a group of models
bool pmSourceFitSetCheckLimits (psMinConstraintMode mode, int nParam, float *params, float *betas);

bool pmSourceFitSetJoin (psVector *deriv, psVector *param, pmSourceFitSetData *set);
bool pmSourceFitSetSplit (pmSourceFitSetData *set, const psVector *deriv, const psVector *param);

bool pmSourceFitSetValues (pmSourceFitSetData *set, 
			   const psVector *dparam, const psVector *param, const psImage *covar, 
			   pmSource *source, psMinimization *myMin, int nPix, 
			   bool fitStatus, pmSourceFitOptions *options, psImageMaskType maskVal);

psF32 pmSourceFitSetFunction(psVector *deriv, const psVector *param, const psVector *x);
bool pmSourceFitSetMasks (psMinConstraint *constraint, pmSourceFitSetData *set, pmSourceFitMode mode);

/** pmSourceFitSet()
 *
 * Fit the requested model to the specified source. The starting guess for the model is given
 * by the input source.model parameter values. The pixels of interest are specified by the
 * source.pixels and source.mask entries. This function calls psMinimizeLMChi2() on the image
 * data. The function returns TRUE on success or FALSE on failure.
 *
 */
bool pmSourceFitSet(
    pmSource *source,                   ///< The input pmSource
    psArray *modelSet,                  ///< model to be fitted
    pmSourceFitOptions *options,	///< define options for fitting process
    psImageMaskType maskVal             ///< Value to mask

);

bool pmSourcePrintModelSet (FILE *file, psArray *modelSet);
bool pmSourceFitSetPrint (FILE *file, pmSourceFitSetData *set);

/// @}
# endif /* PM_SOURCE_FIT_MODEL_H */
