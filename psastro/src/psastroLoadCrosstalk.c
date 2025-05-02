/** @file psastroMaskUpdates.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
#include <time.h>

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "I/O failure in psastroMaskUpdate"); \
  psFree (view); \
  return false; \
}

pmChip * getChipByName (pmFPA *fpa, psString chipname) {
  int i;

  for (i = 0; i < fpa->chips->n; i++) {
    pmChip *chip = fpa->chips->data[i];
    if (strcmp(chipname,psMetadataLookupStr(NULL,chip->concepts,"CHIP.NAME")) == 0) {
      return(chip);
    }
  }
  return(NULL);
}
pmCell * getCellByName (pmChip *chip, psString cellname) {
  int i;

  for (i = 0; i < chip->cells->n; i++) {
    pmCell *cell = chip->cells->data[i];
    if (strcmp(cellname,(char *) psMetadataLookupStr(NULL,cell->concepts,"CELL.NAME")) == 0) {
      return(cell);
    }
  }
  return(NULL);
}


bool psastroLoadCrosstalk (pmConfig *config) {

  bool status;
  pmChip *chip = NULL;
  pmCell *cell = NULL;
  pmReadout *readout = NULL;

  pmFPAview *view = pmFPAviewAlloc (0);
  pmFPAview *viewMask = pmFPAviewAlloc (0);

  float zeropt, exptime, MAX_MAG, INSTR_MAX_MAG, SPIKE_MAX_MAG;

  psLogMsg ("psastro", PS_LOG_INFO, "determine crosstalk positions");

  // select the current recipe
  psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
  if (!recipe) {
    psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
    return false;
  }

  // Determine if we really want to do this masking:
  bool CROSSTALK_MASK  = psMetadataLookupBool (&status, recipe, "CROSSTALK_MASK");
  if (!CROSSTALK_MASK) {
    return(true);
  }

  // select the input astrometry data (also carries the refstars)
  pmFPAfile *astrom = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
  if (!astrom) {
    psError(PSASTRO_ERR_CONFIG, true, "Can't find input data");
    goto escape;
  }
  pmFPA *fpa = astrom->fpa;

  // select the reference mask fpa :: we use this to determine cell boundaries
  pmFPAfile *refMask = psMetadataLookupPtr (NULL, config->files, "PSASTRO.REFMASK");
  if (!refMask) {
    psError(PSASTRO_ERR_CONFIG, true, "Can't find mask reference");
    return false;
  }
  // Activate the reference mask to generate an FPA structure we can use to map stars down to cells
  pmFPAfileActivate (config->files, false, NULL);
  pmFPAfileActivate (config->files, true, "PSASTRO.REFMASK");

  if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

  // really error-out here?  or just skip?
  if (!psastroZeroPointFromRecipe (&zeropt, &exptime, &MAX_MAG, NULL, fpa, recipe)) {
    psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
    goto escape;
  }

  INSTR_MAX_MAG = psMetadataLookupF32 (&status, recipe, "REFSTAR_MASK_CROSSTALK_MAG_MAX");
  // recipe values are given in instrumental magnitudes
  // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
  float MagOffset = zeropt + 2.5*log10(exptime);
  MAX_MAG = INSTR_MAX_MAG + MagOffset;

  // Get the spike maximum, as well
  SPIKE_MAX_MAG = psMetadataLookupF32(&status, recipe, "REFSTAR_MASK_BLEED_MAG_MAX");
  SPIKE_MAX_MAG += MagOffset;

  //we will use one of the new keywords to differentiate between an old and new style crosstalk treatment
  float crossCheck = 0;
  char *crossFile = psMetadataLookupStr (&status, recipe, "CROSSTALK_FILE");
  if (!strcasecmp(crossFile, "NONE")) {
      psLogMsg ("psastro", PS_LOG_INFO, "Assuming old-style crostalk masking, with small number of rules");
      crossCheck = 1;
  }

  //  psTraceSetLevel("psastro.crosstalk",2);  
  psTrace("psastro.crosstalk",2,"%f %f %f %f\n",zeropt,exptime,MAX_MAG,MagOffset);

  // Load each chip.
  while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
    psTrace ("psastro.crosstalk", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
    if (!chip->process || !chip->file_exists) { continue; }
    if (!chip->fromFPA) { continue; }

    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

    // Get the current chip's name, and parse out the grid location.
    pmChip *refChip  = pmFPAviewThisChip (view, refMask->fpa);
    const char *chipName = psMetadataLookupStr(NULL,refChip->concepts, "CHIP.NAME");
    int X = chipName[2] - '0';
    int Y = chipName[3] - '0';
    int chipNum = (X*10)+Y ;

    // Read each cell (at this stage, we should only have one of them)
      while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
        psTrace ("psastro.crosstalk", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
        if (!cell->process || !cell->file_exists) { continue; }

        // Read each readout (similarly, we should only have one of these, too)
        while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
          if (! readout->data_exists) { continue; }

          //old crosstalk method, with limited hard-coded rules
          if(crossCheck) {
            // If this chip doesn't crosstalk, skip to the next one.
            if (! ((X == 2)||(X == 5))) {
              psTrace ("psastro.crosstalk",2,"Chip (%d%d) not a known crosstalk source.",X,Y);
              continue;
            }

            // To check for the presence of crosstalk we first make use the actual detections on this image.
            // That way we can include things like movers creating crosstalk features.
            psArray *calstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.CALSTARS");
            if (calstars == NULL) { continue; }

            for (int i = 0; i < calstars->n; i++) {
              pmAstromObj *cal = calstars->data[i];

              if (cal->Mag > INSTR_MAX_MAG) { continue; }

              // Identify which cell holds the star
              pmChip *CTsourceChip = refChip;
              pmCell *CTsourceCell = pmCellInChip(refChip,cal->chip->x,cal->chip->y);

              if (!CTsourceCell) { continue; }

              psTrace ("psastro.crosstalk",2,"DETEC: %d %f :: %f %f\n",
                       i,cal->Mag, cal->chip->x,cal->chip->y);

              const char *cellName = psMetadataLookupStr(NULL,CTsourceCell->concepts, "CELL.NAME");
              int U = cellName[2] - '0';
              int V = cellName[3] - '0';

              int Xt = X,Yt = Y,Ut = U,Vt = V;
              float x_cell,y_cell;
              float x_t_cell,y_t_cell;
  
              float x_t_chip,y_t_chip;
	      // int faint_ct = 0; NOTE: not currently used
              pmCellCoordsForChip(&x_cell,&y_cell,CTsourceCell,cal->chip->x,cal->chip->y);

              x_t_cell = x_cell;
              y_t_cell = y_cell;

	      // 2020-10-21 TdB: Here is the list of known cross talks from the bright star analysis:
              // The relationships work using OTA"XY"XY"UV"
	      // Inter-chip
	      // OTA2yXY3v => OTA3yXY3v
	      // OTA4yXY3v <= OTA5yXY3v
	      // Intra-chip
	      // OTA2yXY5v <=> OTA2yXY6v
	      // OTA5yXY5v <=> OTA5yXY6v
	      // One way fainter
	      // OTA2yXY7v => OTA3yXY2v
	      // OTA5yXY7v => OTA4yXY2v
              // Determine if this combination of chip and cell produces a crosstalk artifact
              if ((X == 2)&&(! ((U == 3)||(U == 5)||(U == 6)||(U == 7)))) {
                psTrace ("psastro.crosstalk",2,"Cell (%d%d) on chip (%d%d) not a known crosstalk source.",U,V,X,Y);
	        psTrace ("psastro.crosstalk",2,"t1: %d t2: %d",
		         (X == 2),
	  	         (! ((U == 3)||(U == 5)||(U == 6)||(U == 7))));
                continue;
              }
              if ((X == 5)&&(! ((U == 3)||(U == 5)||(U == 6)||(U == 7)))) {
                psTrace ("psastro.crosstalk",2,"Cell (%d%d) on chip (%d%d) not a known crosstalk source.",U,V,X,Y);
                continue;
              }

              // Calculate destination positions
              if (X == 2) {
                if (U == 3) {
                  Xt = 3;
                }
                if (U == 5) {
                  Ut = 6;
                }
                if (U == 6) {
                  Ut = 5;
                }
	        if (U == 7) {
		  Xt = 3;
		  Ut = 2;
		  // faint_ct = 1; NOTE: not currently used
	        }
              }
              if (X == 5) {
                if (U == 3) {
                  Xt = 4;
                }
                if (U == 5) {
                  Ut = 6;
                }
                if (U == 6) {
                  Ut = 5;
                }
	        if (U == 7) {
		  Xt = 4;
		  Ut = 2;
		  // faint_ct = 1; NOTE: not currently used
  	        }
              }

              // Convert target cell coordinates to target chip coordinates
              psString targetChipName = NULL;
              psStringAppend(&targetChipName,"XY%d%d",Xt,Yt);
              psString targetCellName = NULL;
              psStringAppend(&targetCellName,"xy%d%d",Ut,Vt);

              psTrace ("psastro.crosstalk",2,"CTsource from DETEC @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%d%dxy%d%d@%f,%f [%s %s]",
                       X,Y,U,V,x_cell,y_cell,Xt,Yt,Ut,Vt,x_t_cell,y_t_cell,
                       targetChipName,targetCellName);

              pmCell *CTtargetCell = getCellByName (CTsourceChip,targetCellName);
              if (!CTtargetCell) continue;

              pmChipCoordsForCell(&x_t_chip,&y_t_chip,CTtargetCell,x_t_cell,y_t_cell);

              psTrace ("psastro.crosstalk",2,"CTsource from DETEC @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%d%dxy%d%d@%f,%f ChipLoc: (%f,%f)",
                       X,Y,U,V,x_cell,y_cell,Xt,Yt,Ut,Vt,x_t_cell,y_t_cell,x_t_chip,y_t_chip);

              // Hunt down the readout for the target, and save the chip position and magnitude of source star.
              pmChip *targetChip = getChipByName(fpa,targetChipName);
              if (!targetChip) continue;
              if (!targetChip->cells) continue;
              if (!targetChip->cells->n) continue;

              pmCell *targetCell = targetChip->cells->data[0];
              if (!targetCell) continue;
              if (!targetCell->readouts) continue;
              if (!targetCell->readouts->n) continue;
              pmReadout *targetReadout = targetCell->readouts->data[0];
              if (!targetReadout) continue;

              pmAstromObj *crosstalk = pmAstromObjAlloc();
	    
              crosstalk->Mag = cal->Mag;
              crosstalk->chip->x = x_t_chip;
              crosstalk->chip->y = y_t_chip;

              psArray *crosstalks = psMetadataLookupPtr (&status, targetReadout->analysis, "PSASTRO.CROSSTALKS");
              if (crosstalks == NULL) {
                crosstalks = psArrayAllocEmpty(100);
                if (!psMetadataAdd(targetReadout->analysis,PS_LIST_TAIL,"PSASTRO.CROSSTALKS", PS_DATA_ARRAY, "crosstalk locations", crosstalks)) {
                  psError(PSASTRO_ERR_CONFIG, false, "failure to add crosstalks to readout");
                  goto escape;
                }
                psFree(crosstalks);
              }
              psArrayAdd(crosstalks,100,crosstalk);
	    
              psFree(targetChipName);
              psFree(targetCellName);
              psFree(crosstalk);

	      // Determine if we need to add a spike mask.
	      // CZW 2014-04-15 I'm not adding the new faint CT to this operation.  There's no evidence they produce such things.
	      if (cal->Mag < SPIKE_MAX_MAG) {
	        if ((X == 2)&&(! ((U == 3)||(U == 5)||(U == 6)))) {
		  psTrace ("psastro.crosstalk",2,"Cell (%d%d) on chip (%d%d) not a known crosstalk source.",U,V,X,Y);
		  continue;
	        }
	        if ((X == 5)&&(! ((U == 3)||(U == 5)||(U == 6)))) {
		  psTrace ("psastro.crosstalk",2,"Cell (%d%d) on chip (%d%d) not a known crosstalk source.",U,V,X,Y);
		  continue;
	        }
	      
	        Xt = X;
	        Yt = Y;
	        Vt = V;
	      
	        for (Ut = 0; Ut < 8; Ut++) {
		  psString targetChipName = NULL;
		  psStringAppend(&targetChipName,"XY%d%d",Xt,Yt);
		  psString targetCellName = NULL;
		  psStringAppend(&targetCellName,"xy%d%d",Ut,Vt);
		
		  psTrace ("psastro.crosstalk",2,"CTsource @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%d%dxy%d%d@%f,%f [%s %s]",
			   X,Y,U,V,x_cell,y_cell,Xt,Yt,Ut,Vt,x_t_cell,y_t_cell,
			   targetChipName,targetCellName);
		
		  pmCell *CTtargetCell = getCellByName (CTsourceChip,targetCellName);
		  if (!CTtargetCell) continue;
		
		  pmChipCoordsForCell(&x_t_chip,&y_t_chip,CTtargetCell,x_t_cell,y_t_cell);

		  psTrace ("psastro.crosstalk",2,"CTsource @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%d%dxy%d%d@%f,%f ChipLoc: (%f,%f)",
		  	   X,Y,U,V,x_cell,y_cell,Xt,Yt,Ut,Vt,x_t_cell,y_t_cell,x_t_chip,y_t_chip);
		
		  // Hunt down the readout for the target, and save the chip position and magnitude of source star.
		  pmChip *targetChip = getChipByName(fpa,targetChipName);
		  if (!targetChip) continue;
		  if (!targetChip->cells) continue;
		  if (!targetChip->cells->n) continue;
		  
		  pmCell *targetCell = targetChip->cells->data[0];
		  if (!targetCell) continue;
		  if (!targetCell->readouts) continue;
		  if (!targetCell->readouts->n) continue;
		  pmReadout *targetReadout = targetCell->readouts->data[0];
		  if (!targetReadout) continue;
		
		  pmAstromObj *crosstalk = pmAstromObjAlloc();
		
		  crosstalk->Mag = cal->Mag;
		  crosstalk->chip->x = x_t_chip;
		  crosstalk->chip->y = y_t_chip;
		
		  psArray *crosstalks = psMetadataLookupPtr (&status, targetReadout->analysis, "PSASTRO.CROSSTALKS.SPIKES");
		  if (crosstalks == NULL) {
		    crosstalks = psArrayAllocEmpty(100);
		    if (!psMetadataAdd(targetReadout->analysis,PS_LIST_TAIL,"PSASTRO.CROSSTALKS.SPIKES", PS_DATA_ARRAY, "crosstalk locations", crosstalks)) {
		      psError(PSASTRO_ERR_CONFIG, false, "failure to add crosstalks to readout");
		      goto escape;
		    }
		    psFree(crosstalks);
		  }
		  psArrayAdd(crosstalks,100,crosstalk);
		
		  psFree(targetChipName);
		  psFree(targetCellName);
		  psFree(crosstalk);
	        }
	      } // End satspike crosstalks

            }
          }

          //-----------------------------------------------------------------
          //-----------------------------------------------------------------
          //new style crostalk masking, with crosstalk rules from the recipes
          if (!crossCheck) {
            // load ghost model metadata structure
            psMetadata *crossModel = NULL;
            if (!pmConfigFileRead (&crossModel, crossFile, "CROSS MODEL")) {
	          psError(PSASTRO_ERR_CONFIG, true, "Trouble loading crosstalk rules file");
                  return false;
            }

             // get the set of crosstalk rules (CROSSTALK.RULE is a MULTI of METADATA items)
            psMetadataItem *crossRules = psMetadataLookup (crossModel, "CROSSTALK.RULE");
            if (crossRules->type != PS_DATA_METADATA_MULTI) {
                  psWarning ("CROSSTALK.RULE is not a MULTI\n");
                  return true;
            }

            pmDetEff *de = psMetadataLookupPtr(NULL, readout->analysis, PM_DETEFF_ANALYSIS); // Detection efficiency
            if (!de) continue; // chip has a problem if DETEFF is missing, just skip it
            if (isnan(de->magRef)) { continue; }

	    // find the CROSSTALK.RULE this chip lands in (if any)
	    psListIterator *crossIter = psListIteratorAlloc(crossRules->data.list, PS_LIST_HEAD, false);
	    psMetadataItem *crossItem = NULL;
	    while ((crossItem = psListGetAndIncrement (crossIter))) {
	      if (crossItem->type != PS_DATA_METADATA) {
		psWarning ("CROSSTALK.RULE entry is not a metadata folder");
		continue;
	      }

	      int crossChip = psMetadataLookupS32 (&status, crossItem->data.md, "CHIP.SRC");
              if (!(X == crossChip)) {
                psTrace ("psastro.crosstalk",2,"Chip (%d%d) not a known crosstalk source.",X,Y); 
                continue;
              }

	      int crossCell = psMetadataLookupS32 (&status, crossItem->data.md, "CELL.SRC");
	      char *strChipOffset = psMetadataLookupStr (&status, crossItem->data.md, "CHIP.OFFSET");
	      char *strCellOffset = psMetadataLookupStr (&status, crossItem->data.md, "CELL.OFFSET");
	      char *strMagOffset = psMetadataLookupStr (&status, crossItem->data.md, "CROSS.MAGDIFF");

              //Parse the strings into individual entries
              int crossChipOffset[40];
              int crossCellOffset[40];
              float crossMagOffset[40];

              int nrul = 0;
              char *p = strtok (strChipOffset, "/");
              while (p != NULL) {
                  crossChipOffset[nrul++] = atoi(p);
                  p = strtok (NULL, "/");
              }  
              int nrul2 = 0;
              char *p2 = strtok (strCellOffset, "/");
              while (p2 != NULL) {
                  crossCellOffset[nrul2++] = atoi(p2);
                  p2 = strtok (NULL, "/");
              }  
              int nrul3 = 0;
              char *p3 = strtok (strMagOffset, "/");
              while (p3 != NULL) {
                  crossMagOffset[nrul3++] = atof(p3);
                  p3 = strtok (NULL, "/");
              }  

	      if ((nrul != nrul2) | (nrul != nrul3)) {
		psWarning ("CROSSTALK.RULE entry is not correctly formatted or incomplete");
		continue;
	      }

              //the source star needs to be bright enough to produce a crosstalk feature on the target chip/cell that is above the refmag
              //float INSTR_CROSS_MAX_MAG = de->magRef - crossMagOff;
              float INSTR_CROSS_MAX_MAG = de->magRef;
              float CROSS_MAX_MAG = INSTR_CROSS_MAX_MAG + MagOffset;

              // To check for the presence of crosstalk we first make use the actual detections on this image.
              // That way we can include things like movers creating crosstalk features.
              psArray *calstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.CALSTARS");
              if (calstars == NULL) { continue; }

              for (int i = 0; i < calstars->n; i++) {
                pmAstromObj *cal = calstars->data[i];
                //kick out stars too faint to produce crosstalk with any rule
                if (cal->Mag > (INSTR_CROSS_MAX_MAG-6.)) { continue; }

                // Identify which cell holds the star
                pmChip *CTsourceChip = refChip;
                pmCell *CTsourceCell = pmCellInChip(refChip,cal->chip->x,cal->chip->y);
                if (!CTsourceCell) { continue; }

                psTrace ("psastro.crosstalk",2,"DETEC: %d %f :: %f %f\n",
                       i,cal->Mag, cal->chip->x,cal->chip->y);

                const char *cellName = psMetadataLookupStr(NULL,CTsourceCell->concepts, "CELL.NAME");
                int U = cellName[2] - '0';
                int V = cellName[3] - '0';
                int cellNum = (U*10)+V ;

                float x_cell,y_cell;
                float x_t_cell,y_t_cell;
  
                float x_t_chip,y_t_chip;
	        // int faint_ct = 0; NOTE: not currently used
                pmCellCoordsForChip(&x_cell,&y_cell,CTsourceCell,cal->chip->x,cal->chip->y);

                x_t_cell = x_cell;
                y_t_cell = y_cell;

                if (! (U == crossCell) ) {
                  psTrace ("psastro.crosstalk",2,"Cell (%d%d) on chip (%d%d) not a known crosstalk source.",U,V,X,Y);
                  continue;
                }

                for (int k = 0 ; k < nrul; k++) {
                    int chipTarget = chipNum + crossChipOffset[k] ; 
                    int cellTarget = cellNum + crossCellOffset[k] ; 
                    if (cal->Mag > (INSTR_CROSS_MAX_MAG-crossMagOffset[k])) { continue; }

                    // Convert target cell coordinates to target chip coordinates
                    psString targetChipName = NULL;
                    if(chipTarget > 8) {
                      psStringAppend(&targetChipName,"XY%2d",chipTarget);
                    }
                    if(chipTarget < 8) {
                      psStringAppend(&targetChipName,"XY0%1d",chipTarget);
                    }
                    psString targetCellName = NULL;
                    if(cellTarget > 8) {
                      psStringAppend(&targetCellName,"xy%2d",cellTarget);
                    }
                    if(cellTarget < 8) {
                      psStringAppend(&targetCellName,"xy0%1d",cellTarget);
                    }

                    psTrace ("psastro.crosstalk",2,"CTsource from DETEC @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%2dxy%2d@%f,%f [%s %s]",
                       X,Y,U,V,x_cell,y_cell,chipTarget,cellTarget,x_t_cell,y_t_cell,
                       targetChipName,targetCellName);

                    pmCell *CTtargetCell = getCellByName (CTsourceChip,targetCellName);
                    if (!CTtargetCell) continue;

                    pmChipCoordsForCell(&x_t_chip,&y_t_chip,CTtargetCell,x_t_cell,y_t_cell);

                    psTrace ("psastro.crosstalk",2,"CTsource from DETEC @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%2dxy%2d@%f,%f ChipLoc: (%f,%f)",
                       X,Y,U,V,x_cell,y_cell,chipTarget,cellTarget,x_t_cell,y_t_cell,x_t_chip,y_t_chip);

                    // Hunt down the readout for the target, and save the chip position and magnitude of source star.
                    pmChip *targetChip = getChipByName(fpa,targetChipName);
                    if (!targetChip) continue;
                    if (!targetChip->cells) continue;
                    if (!targetChip->cells->n) continue;

                    pmCell *targetCell = targetChip->cells->data[0];
                    if (!targetCell) continue;
                    if (!targetCell->readouts) continue;
                    if (!targetCell->readouts->n) continue;
                    pmReadout *targetReadout = targetCell->readouts->data[0];
                    if (!targetReadout) continue;

                    //check again if the crosstalk is above the target chip background level
                    pmDetEff *tarde = psMetadataLookupPtr(NULL, targetReadout->analysis, PM_DETEFF_ANALYSIS); // Detection efficiency
		    if (!tarde) continue;
                    if (isnan(tarde->magRef)) { continue; }

                    float TARGET_CROSS_MAX_MAG = tarde->magRef-crossMagOffset[k];
                    if (cal->Mag > TARGET_CROSS_MAX_MAG) { continue; }
                    pmAstromObj *crosstalk = pmAstromObjAlloc();
	    
                    crosstalk->Mag = cal->Mag;
                    crosstalk->chip->x = x_t_chip;
                    crosstalk->chip->y = y_t_chip;

                    psArray *crosstalks = psMetadataLookupPtr (&status, targetReadout->analysis, "PSASTRO.CROSSTALKS");
                    if (crosstalks == NULL) {
                      crosstalks = psArrayAllocEmpty(100);
                      if (!psMetadataAdd(targetReadout->analysis,PS_LIST_TAIL,"PSASTRO.CROSSTALKS", PS_DATA_ARRAY, "crosstalk locations", crosstalks)) {
                        psError(PSASTRO_ERR_CONFIG, false, "failure to add crosstalks to readout");
                        goto escape;
                      }
                      psFree(crosstalks);
                    }
                    psArrayAdd(crosstalks,100,crosstalk);
	    
                    psFree(targetChipName);
                    psFree(targetCellName);
                    psFree(crosstalk);

	            // Determine if we need to add a spike mask.
                    // TdB20210304: This should not be relevant for faint crosstalk rules. Do this rather ad hoc right now by placing a limit at 8 mags of difference
                    if( crossMagOffset[k] > 8.) { continue; }
	            if ((cal->Mag + MagOffset) < SPIKE_MAX_MAG) {	      
	              int Xt = X;
	              int Yt = Y;
	              int Vt = V;
	      
	              for (int Ut = 0; Ut < 8; Ut++) {
		        psString targetChipName = NULL;
		        psStringAppend(&targetChipName,"XY%d%d",Xt,Yt);
		        psString targetCellName = NULL;
		        psStringAppend(&targetCellName,"xy%d%d",Ut,Vt);
		
		        psTrace ("psastro.crosstalk",2,"CTsource @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%d%dxy%d%d@%f,%f [%s %s]",
			   X,Y,U,V,x_cell,y_cell,Xt,Yt,Ut,Vt,x_t_cell,y_t_cell,
			   targetChipName,targetCellName);
		
		        pmCell *CTtargetCell = getCellByName (CTsourceChip,targetCellName);
		        if (!CTtargetCell) continue;
		
		        pmChipCoordsForCell(&x_t_chip,&y_t_chip,CTtargetCell,x_t_cell,y_t_cell);

		        psTrace ("psastro.crosstalk",2,"CTsource @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%d%dxy%d%d@%f,%f ChipLoc: (%f,%f)",
		  	   X,Y,U,V,x_cell,y_cell,Xt,Yt,Ut,Vt,x_t_cell,y_t_cell,x_t_chip,y_t_chip);
		
		        // Hunt down the readout for the target, and save the chip position and magnitude of source star.
		        pmChip *targetChip = getChipByName(fpa,targetChipName);
		        if (!targetChip) continue;
		        if (!targetChip->cells) continue;
		        if (!targetChip->cells->n) continue;
		  
		        pmCell *targetCell = targetChip->cells->data[0];
	  	        if (!targetCell) continue;
		        if (!targetCell->readouts) continue;
		        if (!targetCell->readouts->n) continue;
		        pmReadout *targetReadout = targetCell->readouts->data[0];
		        if (!targetReadout) continue;
		
		        pmAstromObj *crosstalk = pmAstromObjAlloc();
		
		        crosstalk->Mag = cal->Mag + MagOffset;
		        crosstalk->chip->x = x_t_chip;
		        crosstalk->chip->y = y_t_chip;
		
		        psArray *crosstalks = psMetadataLookupPtr (&status, targetReadout->analysis, "PSASTRO.CROSSTALKS.SPIKES");
		        if (crosstalks == NULL) {
		          crosstalks = psArrayAllocEmpty(100);
		          if (!psMetadataAdd(targetReadout->analysis,PS_LIST_TAIL,"PSASTRO.CROSSTALKS.SPIKES", PS_DATA_ARRAY, "crosstalk locations", crosstalks)) {
		            psError(PSASTRO_ERR_CONFIG, false, "failure to add crosstalks to readout");
		            goto escape;
		          }
		          psFree(crosstalks);
		        }
		        psArrayAdd(crosstalks,100,crosstalk);
		
		        psFree(targetChipName);
		        psFree(targetCellName);
		        psFree(crosstalk);
	              }
	            } // End satspike crosstalks
                  } //end of rules block
              } // End calMags

              // We also want to use the refstars to check for crosstalk. In particular, the detections 
              // do not do well for bright stars, and saturated detections will underestimate the size of the crostalk
              // Get the array of reference stars we're concerned with.
              psArray *refstars = psMetadataLookupPtr (&status, readout->analysis, "PSASTRO.REFSTARS");
              if (refstars == NULL) { return(true); }

              // Check each reference star
              for (int i = 0; i < refstars->n; i++) {
	        // This is the source of the ct.
                pmAstromObj *ref = refstars->data[i];
                if (ref->Mag > (CROSS_MAX_MAG-6.)) { continue; }

                // Identify which cell holds the star
                pmChip *CTsourceChip = refChip;
                pmCell *CTsourceCell = pmCellInChip(refChip,ref->chip->x,ref->chip->y);

                if (!CTsourceCell) { continue; }

                psTrace ("psastro.crosstalk",2,"REF: %d %f (%f %f) %f %f :: %f %f\n",
                       i,ref->Mag, ref->sky->r,ref->sky->d, ref->FP->x,ref->FP->y, ref->chip->x,ref->chip->y);

                const char *cellName = psMetadataLookupStr(NULL,CTsourceCell->concepts, "CELL.NAME");
                int U = cellName[2] - '0';
                int V = cellName[3] - '0';
                int cellNum = (U*10)+V ;

                float x_cell,y_cell;
                float x_t_cell,y_t_cell;

                float x_t_chip,y_t_chip;
	        // int faint_ct = 0; NOTE: not currently used
                pmCellCoordsForChip(&x_cell,&y_cell,CTsourceCell,ref->chip->x,ref->chip->y);

                x_t_cell = x_cell;
                y_t_cell = y_cell;

                if (! (U == crossCell) ) {
                  psTrace ("psastro.crosstalk",2,"Cell (%d%d) on chip (%d%d) not a known crosstalk source.",U,V,X,Y);
                  continue;
                }

                for (int k = 0 ; k < nrul; k++) {
                    int chipTarget = chipNum + crossChipOffset[k] ; 
                    int cellTarget = cellNum + crossCellOffset[k] ; 
                    if (ref->Mag > (CROSS_MAX_MAG-crossMagOffset[k])) { continue; }

                    // Convert target cell coordinates to target chip coordinates
                    psString targetChipName = NULL;
                    if(chipTarget > 8) {
                      psStringAppend(&targetChipName,"XY%2d",chipTarget);
                    }
                    if(chipTarget < 8) {
                      psStringAppend(&targetChipName,"XY0%1d",chipTarget);
                    }
                    psString targetCellName = NULL;
                    if(cellTarget > 8) {
                      psStringAppend(&targetCellName,"xy%2d",cellTarget);
                    }
                    if(cellTarget < 8) {
                      psStringAppend(&targetCellName,"xy0%1d",cellTarget);
                    }

                    psTrace ("psastro.crosstalk",2,"CTsource @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%2dxy%2d@%f,%f [%s %s]",
                       X,Y,U,V,x_cell,y_cell,chipTarget,cellTarget,x_t_cell,y_t_cell,
                       targetChipName,targetCellName);

                    pmCell *CTtargetCell = getCellByName (CTsourceChip,targetCellName);
                    if (!CTtargetCell) continue;

                    pmChipCoordsForCell(&x_t_chip,&y_t_chip,CTtargetCell,x_t_cell,y_t_cell);

                    psTrace ("psastro.crosstalk",2,"CTsource @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%2dxy%2d@%f,%f ChipLoc: (%f,%f)",
                       X,Y,U,V,x_cell,y_cell,chipTarget,cellTarget,x_t_cell,y_t_cell,x_t_chip,y_t_chip);

                    // Hunt down the readout for the target, and save the chip position and magnitude of source star.
                    pmChip *targetChip = getChipByName(fpa,targetChipName);
                    if (!targetChip) continue;
                    if (!targetChip->cells) continue;
                    if (!targetChip->cells->n) continue;

                    pmCell *targetCell = targetChip->cells->data[0];
                    if (!targetCell) continue;
                    if (!targetCell->readouts) continue;
                    if (!targetCell->readouts->n) continue;
                    pmReadout *targetReadout = targetCell->readouts->data[0];
                    if (!targetReadout) continue;

                    //check if the crosstalk is above the target chip background level
                    pmDetEff *tarde = psMetadataLookupPtr(NULL, targetReadout->analysis, PM_DETEFF_ANALYSIS); // Detection efficiency
		    if (!tarde) continue; 
                    if (isnan(tarde->magRef)) { continue; }

                    float TARGET_CROSS_MAX_MAG = tarde->magRef-crossMagOffset[k] + MagOffset;
                    if (ref->Mag > TARGET_CROSS_MAX_MAG) { continue; }

                    pmAstromObj *crosstalk = pmAstromObjAlloc();
	    
                    crosstalk->Mag = ref->Mag - MagOffset;
                    crosstalk->chip->x = x_t_chip;
                    crosstalk->chip->y = y_t_chip;

                    psArray *crosstalks = psMetadataLookupPtr (&status, targetReadout->analysis, "PSASTRO.CROSSTALKS");
                    if (crosstalks == NULL) {
                      crosstalks = psArrayAllocEmpty(100);
                      if (!psMetadataAdd(targetReadout->analysis,PS_LIST_TAIL,"PSASTRO.CROSSTALKS", PS_DATA_ARRAY, "crosstalk locations", crosstalks)) {
                        psError(PSASTRO_ERR_CONFIG, false, "failure to add crosstalks to readout");
                        goto escape;
                      }
                      psFree(crosstalks);
                    }
                    psArrayAdd(crosstalks,100,crosstalk);
	    
                    psFree(targetChipName);
                    psFree(targetCellName);
                    psFree(crosstalk);

	            // Determine if we need to add a spike mask.
                    // TdB20210304: This should not be relevant for faint crosstalk rules. Do this rather ad hoc right now by placing a limit at 8 mags of difference
                    if( crossMagOffset[k] > 8.) { continue; }
	            if (ref->Mag < SPIKE_MAX_MAG) {	      
	              int Xt = X;
	              int Yt = Y;
	              int Vt = V;
	      
	              for (int Ut = 0; Ut < 8; Ut++) {
		        psString targetChipName = NULL;
		        psStringAppend(&targetChipName,"XY%d%d",Xt,Yt);
		        psString targetCellName = NULL;
		        psStringAppend(&targetCellName,"xy%d%d",Ut,Vt);
		
		        psTrace ("psastro.crosstalk",2,"CTsource @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%d%dxy%d%d@%f,%f [%s %s]",
			   X,Y,U,V,x_cell,y_cell,Xt,Yt,Ut,Vt,x_t_cell,y_t_cell,
			   targetChipName,targetCellName);
		
		        pmCell *CTtargetCell = getCellByName (CTsourceChip,targetCellName);
		        if (!CTtargetCell) continue;
		
		        pmChipCoordsForCell(&x_t_chip,&y_t_chip,CTtargetCell,x_t_cell,y_t_cell);

		        psTrace ("psastro.crosstalk",2,"CTsource @ OTA%d%dxy%d%d@%f,%f CTtarget OTA%d%dxy%d%d@%f,%f ChipLoc: (%f,%f)",
		  	   X,Y,U,V,x_cell,y_cell,Xt,Yt,Ut,Vt,x_t_cell,y_t_cell,x_t_chip,y_t_chip);
		
		        // Hunt down the readout for the target, and save the chip position and magnitude of source star.
		        pmChip *targetChip = getChipByName(fpa,targetChipName);
		        if (!targetChip) continue;
		        if (!targetChip->cells) continue;
		        if (!targetChip->cells->n) continue;
		  
		        pmCell *targetCell = targetChip->cells->data[0];
	  	        if (!targetCell) continue;
		        if (!targetCell->readouts) continue;
		        if (!targetCell->readouts->n) continue;
		        pmReadout *targetReadout = targetCell->readouts->data[0];
		        if (!targetReadout) continue;
		
		        pmAstromObj *crosstalk = pmAstromObjAlloc();
		
		        crosstalk->Mag = ref->Mag ;
		        crosstalk->chip->x = x_t_chip;
		        crosstalk->chip->y = y_t_chip;
		
		        psArray *crosstalks = psMetadataLookupPtr (&status, targetReadout->analysis, "PSASTRO.CROSSTALKS.SPIKES");
		        if (crosstalks == NULL) {
		          crosstalks = psArrayAllocEmpty(100);
		          if (!psMetadataAdd(targetReadout->analysis,PS_LIST_TAIL,"PSASTRO.CROSSTALKS.SPIKES", PS_DATA_ARRAY, "crosstalk locations", crosstalks)) {
		            psError(PSASTRO_ERR_CONFIG, false, "failure to add crosstalks to readout");
		            goto escape;
		          }
		          psFree(crosstalks);
		        }
		        psArrayAdd(crosstalks,100,crosstalk);
		
		        psFree(targetChipName);
		        psFree(targetCellName);
		        psFree(crosstalk);
	              }
	            } // End satspike crosstalks
                  } // End rules block
              } // End refMags
            }
          }


        } // readout
      } // cell
      //    }

      // File cleanup code.
    *viewMask = *view;
    while ((cell = pmFPAviewNextCell (viewMask, refMask->fpa, 1)) != NULL) {
      psTrace ("psastro", 4, "Mask Cell %d: %x %x\n", viewMask->cell, cell->file_exists, cell->process);
      if (!cell->process || !cell->file_exists) { continue; }

      while ((readout = pmFPAviewNextReadout (viewMask, refMask->fpa, 1)) != NULL) {
        if (! readout->data_exists) { continue; }
        if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_AFTER)) ESCAPE;
      }
      if (!pmFPAfileIOChecks (config, viewMask, PM_FPA_AFTER)) ESCAPE;
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
  }
  if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;

  psTrace ("psastro.crosstalk", 2, "Leaving crosstalk code.\n");

  psFree(view);
  psFree(viewMask);
  return(true);

 escape:
  psFree(view);
  psFree(viewMask);
  return(false);
}

