/** @file psastroMosiacAstrom.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define NONLIN_TOL 0.001 ///< tolerance in pixels
# define DEBUG 0

/**
 * Apply the generic astrometry model to this image.  This assumes the WCS i
 * terms either do not exist or are invalid
 */
bool psastroUseModel (pmConfig *config, psMetadata *recipe) {

  bool status;

  bool useModel = psMetadataLookupBool (&status, config->arguments, "PSASTRO.USE.MODEL");
  if (!status) {
      useModel = psMetadataLookupBool (&status, recipe, "PSASTRO.USE.MODEL");
  }
  if (!useModel) return true;

  // identify reference astrometry table.
  // if not defined, correction was not requested; skip step
  pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.MODEL");
  if (!astrom) psAbort ("programming error: model was not supplied, though requested");

  // select the input data sources
  pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
  if (!input) psAbort ("programming error: no input data");

  // make sure the astrometry model is loaded.  de-activate all files except PSASTRO.MODEL.
  pmFPAfileActivate (config->files, false, NULL);
  pmFPAfileActivate (config->files, true, "PSASTRO.MODEL");

  pmFPAview *view = pmFPAviewAlloc (0);

  // files associated with the science image
  if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
      psError (PS_ERR_IO, false, "Can't load the astrometry model file");
      return false;
  }

  // XXX TEST: apply the input image RA & DEC to the astrometry model, save corners
  if (0) {
      // these externally supplied values are used to set the final transformation terms
      double RA  = psMetadataLookupF64 (&status, input->fpa->concepts, "FPA.RA");
      double DEC = psMetadataLookupF64 (&status, input->fpa->concepts, "FPA.DEC");
      // double POS = PM_RAD_DEG * psMetadataLookupF64 (&status, input->fpa->concepts, "FPA.POSANGLE");

      // get projection scale; center is supplied
      // XXX should this be astrom or input??
      float Xs = psMetadataLookupF32(&status, astrom->fpa->concepts, "XSCALE") * PM_RAD_DEG;
      float Ys = psMetadataLookupF32(&status, astrom->fpa->concepts, "YSCALE") * PM_RAD_DEG;

      // allocate a new toSky projection using the reported position
      psFree (astrom->fpa->toSky);
      astrom->fpa->toSky = psProjectionAlloc (RA, DEC, Xs, Ys, PS_PROJ_DIS);

      // local view
      pmFPAview *myView = pmFPAviewAlloc (0);

      // loop over all chips, replace input astrometry elements with those from astrom
      pmChip *obsChip = NULL;
      while ((obsChip = pmFPAviewNextChip (myView, input->fpa, 1)) != NULL) {
          psTrace ("psastro", 4, "Chip %d: %x %x\n", myView->chip, obsChip->file_exists, obsChip->process);
          if (!obsChip->process || !obsChip->file_exists || !obsChip->data_exists) { continue; }

          // set the chip astrometry using the astrom file
          pmChip *refChip = pmFPAviewThisChip (myView, astrom->fpa);

          psFree (obsChip->toFPA);
          psFree (obsChip->fromFPA);

          // supply astrometry from model
          obsChip->toFPA   = psMemIncrRefCounter (refChip->toFPA);
          obsChip->fromFPA = psMemIncrRefCounter (refChip->fromFPA);

          // XXX if we want to write out the result, update the header here.  this needs to be
          // updated with the correct HDU selection.  obsChip->hdu may not exist.
          // pmAstromWriteBilevelChip (obsChip->hdu->header, obsChip, NONLIN_TOL);
      }

      psFree (input->fpa->toSky);
      psFree (input->fpa->toTPA);
      psFree (input->fpa->fromTPA);
      input->fpa->toSky   = psMemIncrRefCounter (astrom->fpa->toSky);
      input->fpa->toTPA   = psMemIncrRefCounter (astrom->fpa->toTPA);
      input->fpa->fromTPA = psMemIncrRefCounter (astrom->fpa->fromTPA);

      // XXX this is temporarily hardwired because of model error
      input->fpa->toSky->type = PS_PROJ_TAN;

      if (DEBUG) psastroDumpCorners ("corners.up.ast1.dat", "corners.dn.ast1.dat", input->fpa);
  }

  // set the model using the RA, DEC, POSANGLE of the input image.
  pmAstromModelSetTP (astrom, input->fpa->concepts);

  if (DEBUG) psastroDumpCorners ("corners.up.ast2.dat", "corners.dn.ast2.dat", astrom->fpa);

  // loop over all chips, replace input astrometry elements with those from astrom
  pmChip *obsChip = NULL;
  while ((obsChip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
    psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, obsChip->file_exists, obsChip->process);
    if (!obsChip->process || !obsChip->file_exists || !obsChip->data_exists) { continue; }

    // set the chip astrometry using the astrom file
    pmChip *refChip = pmFPAviewThisChip (view, astrom->fpa);

    psFree (obsChip->toFPA);
    psFree (obsChip->fromFPA);

    // supply astrometry from model
    obsChip->toFPA   = psMemIncrRefCounter (refChip->toFPA);
    obsChip->fromFPA = psMemIncrRefCounter (refChip->fromFPA);

    // XXX if we want to write out the result, update the header here.  this needs to be
    // updated with the correct HDU selection.  obsChip->hdu may not exist.
    // pmAstromWriteBilevelChip (obsChip->hdu->header, obsChip, NONLIN_TOL);
  }

  psFree (input->fpa->toSky);
  psFree (input->fpa->toTPA);
  psFree (input->fpa->fromTPA);
  input->fpa->toSky   = psMemIncrRefCounter (astrom->fpa->toSky);
  input->fpa->toTPA   = psMemIncrRefCounter (astrom->fpa->toTPA);
  input->fpa->fromTPA = psMemIncrRefCounter (astrom->fpa->fromTPA);

  // XXX this is temporarily hardwired because of model error
  input->fpa->toSky->type = PS_PROJ_TAN;

  if (DEBUG) psastroDumpCorners ("corners.up.inp.dat", "corners.dn.inp.dat", input->fpa);

  // updated with the correct HDU selection.  obsChip->hdu may not exist.
  // psMetadata *updates = psMetadataAlloc();
  // pmAstromWriteBilevelMosaic (updates, input->fpa, NONLIN_TOL);
  // psMetadataAddMetadata (input->fpa->analysis, PS_LIST_TAIL, "PSASTRO.HEADER",  PS_META_REPLACE, "psastro header stats", updates);
  // psFree (updates);

  // files associated with the science image
  if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) {
      psError (PS_ERR_IO, false, "Can't close the astrometry model file");
      return false;
  }

  psFree (view);
  return true;
}

