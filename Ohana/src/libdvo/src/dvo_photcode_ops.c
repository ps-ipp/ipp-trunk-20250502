# include <dvo.h>

/* we use a static variable to save the pre-loaded the photcodes.
   photcode conversion functions refer to this table for ref values */

/* We have three indexes:

   table[0].hashcode provides the table photcode sequence for a photcode value
   table[0].hashNsec provides the Nsec sequence for a photcode value
   table[0].codeNsec provides the photcode value for an Nsec sequence

   given photcode = table[0].code[i] and code = photcode[0].code

   hashcode[code] = i
   hashNsec[code] = Nsec or -1 if not a PRI/SEC code
   codeNsec[Nsec] = code
*/

static PhotCodeData *photcodes = NULL;
static PhotCode *genericCodeMag = NULL;
static PhotCode *genericCodeFlux = NULL;

PhotCodeData *GetPhotcodeTable () {

  if (photcodes == NULL) {
    /* allocate space to photcode table */
    ALLOCATE (photcodes, PhotCodeData, 1);
    photcodes[0].code = NULL;
  }
  if (genericCodeMag == NULL) {
    ALLOCATE (genericCodeMag, PhotCode, 1);
    genericCodeMag->code = 0;
    strcpy (genericCodeMag->name, "MAG");
    genericCodeMag->type = PHOT_MAG;
  }
  if (genericCodeFlux == NULL) {
    ALLOCATE (genericCodeFlux, PhotCode, 1);
    genericCodeFlux->code = 0;
    strcpy (genericCodeFlux->name, "MAG");
    genericCodeFlux->type = PHOT_MAG;
  }
  return photcodes;
}

// free a specified photcode set
void FreePhotcodeData (PhotCodeData *myPhotcodes) {
  if (!myPhotcodes) return;
  FREE(myPhotcodes->code);
  FREE(myPhotcodes);
}

// free the internal table
void FreePhotcodeTable (void) {
  FreePhotcodeData (photcodes);
  FREE(genericCodeMag);
  FREE(genericCodeFlux);
}

// set the photcode table. This is used to switch between several previously
// allocated tables
void SetPhotcodeTable (PhotCodeData *new) {
  photcodes = new;  
}

/* the static ZERO_POINT is used by programs as an approximate nominal */
static double ZERO_POINT = 0.0;

void SetZeroPoint (double ZP) {
  ZERO_POINT = ZP;
}

double GetZeroPoint () {
  return ZERO_POINT;
}

# define SCALE 0.001

/********** photcode lookup functions **********/

/* return photcode for given name */
PhotCode *GetPhotcodebyName (char *name) {
  
  int i;

  if (name == NULL) return (NULL);
  
  // "MAG" is a special word, not allowed for a photcode name
  if (!strcasecmp (name, "MAG")) {
    return genericCodeMag;
  }

  // "FLUX" is a special word, not allowed for a photcode name
  if (!strcasecmp (name, "FLUX")) {
    return genericCodeFlux;
  }

  for (i = 0; i < photcodes[0].Ncode; i++) {
    if (!strcmp (photcodes[0].code[i].name, name)) {
      return (&photcodes[0].code[i]);
    }
  }
  return (NULL);
}
/* return photcode.code for given name */
int GetPhotcodeCodebyName (char *name) {
  
  int i;
  
  if (name == NULL) return (0);

  // "MAG" is a special photcode name for internal use
  if (!strcasecmp (name, "MAG")) {
    return genericCodeMag->code;
  }
  // "FLUX" is a special photcode name for internal use
  if (!strcasecmp (name, "FLUX")) {
    return genericCodeFlux->code;
  }

  for (i = 0; i < photcodes[0].Ncode; i++) {
    if (!strcmp (photcodes[0].code[i].name, name)) {
      return (photcodes[0].code[i].code);
    }
  }
  return (0);
}
/* return equivalent photcode for given name */
PhotCode *GetPhotcodeEquivbyName (char *name) {
  
  int i, equiv;
  
  if (name == NULL) return (NULL);

  for (i = 0; i < photcodes[0].Ncode; i++) {
    if (!strcmp (photcodes[0].code[i].name, name)) {
      if (photcodes[0].code[i].equiv == 0) return (NULL);
      equiv = photcodes[0].hashcode[photcodes[0].code[i].equiv];
      if (equiv == -1) return (NULL);
      return (&photcodes[0].code[equiv]);
    }
  }
  return (NULL);
}
/* return equivalent photcode.code for given name */
int GetPhotcodeEquivCodebyName (char *name) {
  
  int i, equiv;
  
  if (name == NULL) return (0);

  for (i = 0; i < photcodes[0].Ncode; i++) {
    if (!strcmp (photcodes[0].code[i].name, name)) {
      if (photcodes[0].code[i].equiv == 0) return (0);
      equiv = photcodes[0].hashcode[photcodes[0].code[i].equiv];
      if (equiv == -1) return (0);
      return (photcodes[0].code[equiv].code);
    }
  }
  return (0);
}

/* return photcode for given code */
PhotCode *GetPhotcodebyCode (int code) {
  
  int entry;
  
  if (code < 0) return (NULL);
  if (code > 0x10000) return (NULL);

  entry = photcodes[0].hashcode[code];
  if (entry == -1) return (NULL);

  return (&photcodes[0].code[entry]);
}
/* return photcode.code for given code */
char *GetPhotcodeNamebyCode (int code) {
  
  int entry;
  
  if (code < 0) return (NULL);
  if (code > 0x10000) return (NULL);

  entry = photcodes[0].hashcode[code];
  if (entry == -1) return (NULL);

  return (photcodes[0].code[entry].name);
}
/* return equivalent photcode for given code */
PhotCode *GetPhotcodeEquivbyCode (int code) {
  
  int entry, equiv;
  
  if (code < 0) return (NULL);
  if (code > 0x10000) return (NULL);

  entry = photcodes[0].hashcode[code];
  if (entry == -1) return (NULL);

  if (photcodes[0].code[entry].equiv == 0) return (NULL);
  equiv = photcodes[0].hashcode[photcodes[0].code[entry].equiv];

  if (equiv == -1) return (NULL);
  return (&photcodes[0].code[equiv]);
}
/* return equivalent photcode.code for given code */
int GetPhotcodeEquivCodebyCode (int code) {
  myAssert (photcodes, "photcodes not loaded");
  
  int entry;
  
  if (code < 0) return (0);
  if (code > 0x10000) return (0);

  entry = photcodes[0].hashcode[code];
  if (entry == -1) return (0);
  return (photcodes[0].code[entry].equiv);
}

// returns Nsec if code is PRI/SEC, else -1
int GetPhotcodeNsec (int code) {
  myAssert (photcodes, "photcodes not loaded");
  
  int Nsec;
  
  if (code < 0) return (-1);
  if (code > 0x10000) return (-1);

  Nsec = photcodes[0].hashNsec[code];
  return (Nsec);
}

/* Nsec of 0 is PRI */
PhotCode *GetPhotcodebyNsec (int Nsec) {
  myAssert (photcodes, "photcodes not loaded");
  
  int Ncode;
  int Nseq;

  if (Nsec > photcodes[0].Nsecfilt) return (NULL);
  if (Nsec < 0) return (NULL);
  
  Ncode = photcodes[0].codeNsec[Nsec];
  if (Ncode < 0) return (NULL);

  Nseq = photcodes[0].hashcode[Ncode];
  return (&photcodes[0].code[Nseq]);
}

int GetPhotcodeNsecfilt () {
  myAssert (photcodes, "photcodes not loaded");
  return (photcodes[0].Nsecfilt);
}

/* ALLOCATE and return list of all photcodes
   with photcode.equiv == code */
int *GetPhotcodeEquivList (int code, int *nlist) {

  int i, Nlist;
  int *list;

  ALLOCATE (list, int, MAX (1, photcodes[0].Ncode));
  Nlist = 0;
  for (i = 0; i < photcodes[0].Ncode; i++) {
    if (photcodes[0].code[i].equiv != code) continue;
    list[Nlist] = photcodes[0].code[i].code;
    Nlist ++;
  }
  REALLOCATE (list, int, MAX (1, Nlist));

  *nlist = Nlist;
  return (list);
}

/******** photometry conversion functions *********/
float PhotInst (Measure *measure, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  float Mraw = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Mraw = measure[0].M;
      break;
    case MAG_CLASS_KRON:
      Mraw = measure[0].Mkron;
      break;
    case MAG_CLASS_APER:
      Mraw = measure[0].Map;
      break;
    default:
      break;
  }
  if (code->type == PHOT_REF) {
    return (Mraw);
  }
  float Minst = Mraw - measure[0].dt - ZERO_POINT;

  return (Minst);
}

float PhotCat (Measure *measure, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  float Mraw = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Mraw = measure[0].M;
      break;
    case MAG_CLASS_KRON:
      Mraw = measure[0].Mkron;
      break;
    case MAG_CLASS_APER:
      Mraw = measure[0].Map;
      break;
    default:
      break;
  }
  if (code->type == PHOT_REF) {
    return (Mraw);
  }

  float Mcat = Mraw - ZERO_POINT + code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;
  
  return (Mcat);
}

float PhotSys (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  float Mraw = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Mraw = measure[0].M;
      break;
    case MAG_CLASS_KRON:
      Mraw = measure[0].Mkron;
      break;
    case MAG_CLASS_APER:
      Mraw = measure[0].Map;
      break;
    default:
      break;
  }
  if (code->type == PHOT_REF) {
    return (Mraw);
  }
  float Mcat = Mraw - ZERO_POINT + code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;

  /* for DEP, color must be made of PRI/SEC */
  float mc = PhotColorForCode (average, secfilt, NULL, code);
  if (isnan(mc)) return (Mcat);
  mc -= SCALE*code[0].dX;

  int i = 0;
  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Msys = Mcat + Mcol;
  return (Msys);
}

// old: Mrel = Mcat - measure.Mcal
// new: Mrel = Mcat - measure.Mcal - measure.Mflat
//  or: Mave = Mcal - measure.Mcal - measure.Mflat
float PhotRel (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  float Mraw = NAN;
  float Mcal = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Mraw = measure[0].M;
      Mcal = measure[0].McalPSF;
      break;
    case MAG_CLASS_KRON:
      Mraw = measure[0].Mkron;
      Mcal = measure[0].McalAPER;
      break;
    case MAG_CLASS_APER:
      Mraw = measure[0].Map;
      Mcal = measure[0].McalAPER;
      break;
    default:
      break;
  }
  if (code->type == PHOT_REF) {
    return (Mraw);
  }
  float Mflat = isfinite(measure[0].Mflat) ? measure[0].Mflat : 0.0;
  float Mcat = Mraw - ZERO_POINT + code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C - Mcal - Mflat;

  /* for DEP, color must be made of PRI/SEC */
  float mc = PhotColorForCode (average, secfilt, NULL, code);
  if (isnan(mc)) return (Mcat);
  mc -= SCALE*code[0].dX;

  double Mc = mc;
  float Mcol = 0;
  int i = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Mrel = Mcat + Mcol;
  return (Mrel);
}

/* return calibrated magnitude from measure for given photcode */
float PhotCal (Measure *thisone, Average *average, SecFilt *secfilt, Measure *measure, PhotCode *code, dvoMagClassType class) {

  int i; 

  if (code == NULL) return NAN;

  /* code must be the matching PRI/SEC code for this measurement or an equivalent ALT */
  int Np = photcodes[0].hashcode[thisone[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *myCode = &photcodes[0].code[Np];

  if (code->code != myCode->equiv) return (NAN);

  // if we are REF type, just get the right version and return
  if (myCode->type == PHOT_REF) {
    float Mraw = NAN;
    switch (class) {
      case MAG_CLASS_PSF:
	Mraw = thisone[0].M;
	break;
      case MAG_CLASS_KRON:
	Mraw = thisone[0].Mkron;
	break;
      case MAG_CLASS_APER:
	Mraw = thisone[0].Map;
	break;
      default:
	break;
    }
    return (Mraw);
  }

  float Mcal = PhotRel (thisone, average, secfilt, class) + SCALE*code[0].C;

  float mc = PhotColorForCode (average, secfilt, measure, code);
  if (isnan(mc)) return (Mcal);
  mc -= SCALE*code[0].dX;

  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  Mcal += Mcol;
  return (Mcal);
}

/***/
float PhotAve (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return NAN;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return (NAN);

  float Mave = NAN;
  switch (source) {
    case MAG_SRC_CHP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Mave = secfilt[Ns].MpsfChp;
	  break;
	case MAG_CLASS_KRON:
	  Mave = secfilt[Ns].MkronChp;
	  break;
	case MAG_CLASS_APER:
	  Mave = secfilt[Ns].MapChp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_WRP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Mave = secfilt[Ns].MpsfWrp;
	  break;
	case MAG_CLASS_KRON:
	  Mave = secfilt[Ns].MkronWrp;
	  break;
	case MAG_CLASS_APER:
	  Mave = secfilt[Ns].MapWrp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_STK:
      switch (class) {
	case MAG_CLASS_PSF:
	  Mave = secfilt[Ns].MpsfStk;
	  break;
	case MAG_CLASS_KRON:
	  Mave = secfilt[Ns].MkronStk;
	  break;
	case MAG_CLASS_APER:
	  Mave = secfilt[Ns].MapStk;
	  break;
	default:
	  break;
      }
      break;
    default:
      break;
  }
  return (Mave);
}

/* return calibrated magnitude from average/secfilt for given photcode */
float PhotRef (PhotCode *code, Average *average, SecFilt *secfilt, Measure *measure, dvoMagClassType class, dvoMagSourceType source) {

  int i;

  float Mave = PhotAve (code, average, secfilt, class, source);
  if (isnan(Mave)) return NAN;

  // correct for relative zero-point
  float Mref = Mave + SCALE*code[0].C;

  float mc = PhotColorForCode (average, secfilt, measure, code);
  if (isnan(mc)) return (Mref);
  mc -= SCALE*code[0].dX;

  // correct for color terms
  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;    /* the 0.001 is needed for higher order terms to keep the units mag = mag^n */
  }
  Mref += Mcol;
  return (Mref);
}

float PhotErr (Measure *measure, dvoMagClassType class) {

  float dMraw = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      dMraw = measure[0].dM;
      break;
    case MAG_CLASS_KRON:
      dMraw = measure[0].dMkron;
      break;
    case MAG_CLASS_APER:
      dMraw = measure[0].dMap;
      break;
    default:
      break;
  }
  return (dMraw);
}

float PhotCalErr (Measure *measure, dvoMagClassType class) {
  OHANA_UNUSED_PARAM(class);

  float dMcal = measure[0].dMcal;
  return (dMcal);
}

float PhotAveErr (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return NAN;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return (NAN);

  float dMave = NAN;
  switch (source) {
    case MAG_SRC_CHP:
      switch (class) {
	case MAG_CLASS_PSF:
	  dMave = secfilt[Ns].dMpsfChp;
	  break;
	case MAG_CLASS_KRON:
	  dMave = secfilt[Ns].dMkronChp;
	  break;
	case MAG_CLASS_APER:
	  dMave = secfilt[Ns].dMapChp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_WRP:
      switch (class) {
	case MAG_CLASS_PSF:
	  dMave = secfilt[Ns].dFpsfWrp / secfilt[Ns].FpsfWrp;
	  break;
	case MAG_CLASS_KRON:
	  dMave = secfilt[Ns].dFkronWrp / secfilt[Ns].FkronWrp;
	  break;
	case MAG_CLASS_APER:
	  dMave = secfilt[Ns].dFapWrp / secfilt[Ns].FapWrp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_STK:
      switch (class) {
	case MAG_CLASS_PSF:
	  dMave = secfilt[Ns].dFpsfStk / secfilt[Ns].FpsfStk;
	  break;
	case MAG_CLASS_KRON:
	  dMave = secfilt[Ns].dFkronStk / secfilt[Ns].FkronStk;
	  break;
	case MAG_CLASS_APER:
	  dMave = secfilt[Ns].dFapStk / secfilt[Ns].FapStk;
	  break;
	default:
	  break;
      }
      break;
    default:
      break;
  }
  return (dMave);
}

/********************* other support functions ********************************/

float PhotZeroPoint (Measure *measure, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);
  OHANA_UNUSED_PARAM(secfilt);

  int Np;
  float ZP;
  PhotCode *code;

  Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);

  if (photcodes[0].code[Np].type == PHOT_REF) {
    ZP = 0.0;
    return (ZP);
  }
  code = &photcodes[0].code[Np];
  ZP = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;

  return (ZP);
}

/* color term may not use DEP magnitude */
// currently only returns the PSF color
float PhotColorForCode (Average *average, SecFilt *secfilt, Measure *measure, PhotCode *code) {

  int i, Ns1, Ns2, Ns;
  float m1, m2, mc;
  PhotCode *color;

  m1 = m2 = NAN;

  if (measure == NULL) {
    Ns1 = photcodes[0].hashNsec[code[0].c1];
    Ns2 = photcodes[0].hashNsec[code[0].c2];
  
    m1 = (Ns1 == -1) ? NAN : secfilt[Ns1].MpsfChp;
    m2 = (Ns2 == -1) ? NAN : secfilt[Ns2].MpsfChp;
    mc = (isnan(m1) || isnan(m2)) ? NAN : (m1 - m2);
    return (mc);
  }

  /* find magnitude matching first color term */
  color = GetPhotcodebyCode (code[0].c1);
  if (color == NULL) return (NAN);
  if (color[0].type == PHOT_REF) {
    for (i = 0; (i < average[0].Nmeasure) && (isnan(m1)); i++) {
      if (measure[i].photcode == color[0].code) {
	m1 = measure[i].M;
      }
    }
  } else {
    Ns = photcodes[0].hashNsec[color[0].code];
    m1 = (Ns == -1) ? NAN : secfilt[Ns].MpsfChp;
  }	

  /* find magnitude matching second color term */
  color = GetPhotcodebyCode (code[0].c2);
  if (color == NULL) return (NAN);
  if (color[0].type == PHOT_REF) {
    for (i = 0; (i < average[0].Nmeasure) && (isnan(m2)); i++) {
      if (measure[i].photcode == color[0].code) {
	m2 = measure[i].M;
      }
    }
  } else {
    Ns = photcodes[0].hashNsec[color[0].code];
    m2 = (Ns == -1) ? NAN : secfilt[Ns].MpsfChp;
  }	
  mc = (isnan(m1) || isnan(m2)) ? NAN : (m1 - m2);
  return (mc);
}

float PhotMstdev (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return NAN;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return (NAN);

  float Mstdev = NAN;
  switch (source) {
    case MAG_SRC_CHP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Mstdev = secfilt[Ns].sMpsfChp;
	  break;
	case MAG_CLASS_KRON:
	  Mstdev = secfilt[Ns].sMkronChp;
	  break;
	case MAG_CLASS_APER:
	  Mstdev = secfilt[Ns].sMapChp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_WRP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Mstdev = secfilt[Ns].sFpsfWrp;
	  break;
	case MAG_CLASS_KRON:
	  Mstdev = secfilt[Ns].sFkronWrp;
	  break;
	case MAG_CLASS_APER:
	  Mstdev = secfilt[Ns].sFapWrp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_STK:
    default:
      break;
  }
  return (Mstdev);
}

// return the number of detections in this filter (gpc1)
int PhotNwarp (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  return secfilt[Ns].Nwarp;
}

// return the number of detections in this filter (gpc1)
int PhotNwarpGood (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  return secfilt[Ns].NwarpGood;
}

// return the number of detections in this filter (gpc1)
int PhotNstack (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  return secfilt[Ns].Nstack;
}

// return the number of detections in this filter (gpc1)
int PhotNstackDet (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  return secfilt[Ns].NstackDet;
}

// return the number of detections in this filter (gpc1)
int PhotNcode (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  return secfilt[Ns].Ncode;
}

// return the number of detections in this filter (gpc1)
int PhotSecfiltFlags (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  return secfilt[Ns].flags;
}

// return the number of detections in this filter (gpc1)
float PhotSecfiltPsfQf (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  return secfilt[Ns].psfQfMax;
}

// return the number of detections in this filter (gpc1)
float PhotSecfiltPsfQfPerfect (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  return secfilt[Ns].psfQfPerfMax;
}

int PhotNphot (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return 0;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return 0;

  int Nphot = 0;
  switch (source) {
    case MAG_SRC_CHP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Nphot = secfilt[Ns].Nused;
	  break;
	case MAG_CLASS_KRON:
	  Nphot = secfilt[Ns].NusedKron;
	  break;
	case MAG_CLASS_APER:
	  Nphot = secfilt[Ns].NusedAp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_WRP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Nphot = secfilt[Ns].NusedWrp;
	  break;
	case MAG_CLASS_KRON:
	  Nphot = secfilt[Ns].NusedKronWrp;
	  break;
	case MAG_CLASS_APER:
	  Nphot = secfilt[Ns].NusedApWrp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_STK:
    default:
      break;
  }
  return (Nphot);
}

float PhotAveFluxPSF (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float Fpsf;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  Fpsf = (Ns == -1) ? NAN : secfilt[Ns].FpsfStk;
  return (Fpsf);
}

float PhotAvedFluxPSF (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float dFpsf;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  dFpsf = (Ns == -1) ? NAN : secfilt[Ns].dFpsfStk;
  return (dFpsf);
}

float PhotAveFluxKron (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float Fkron;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  Fkron = (Ns == -1) ? NAN : secfilt[Ns].FkronStk;
  return (Fkron);
}

float PhotAvedFluxKron (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float dFkron;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  dFkron = (Ns == -1) ? NAN : secfilt[Ns].dFkronStk;
  return (dFkron);
}

float PhotMmin (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float Mmin;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  Mmin  = (Ns == -1) ? NAN : secfilt[Ns].Mmin;
  return (Mmin);
}

float PhotMmax (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float Mmax;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  Mmax  = (Ns == -1) ? NAN : secfilt[Ns].Mmax;
  return (Mmax);
}

float PhotUCdist (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float Muc;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  Muc  = (Ns == -1) ? NAN : secfilt[Ns].ubercalDist;
  return (Muc);
}

// XX unsigned int PhotStackID (PhotCode *code, Average *average, SecFilt *secfilt) {
// XX 
// XX   int Ns;
// XX   unsigned int ID;
// XX 
// XX   if (code == NULL) return 0;
// XX 
// XX   Ns = photcodes[0].hashNsec[code[0].code];
// XX   ID = (Ns == -1) ? 0 : secfilt[Ns].stackDetectID;
// XX   return (ID);
// XX }

// Xm is now (2014.07.03) stored as the chisq except in dvo formats which use as short
float PhotXm (PhotCode *code, Average *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float Xm;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  Xm = (Ns == -1) ? NAN : secfilt[Ns].Mchisq;
  return (Xm);
}

/* given a photcode pair c1 & c2, return the color of this star (NaN if not found) */
int PhotColor (Average *average, SecFilt *secfilt, Measure *measure, int c1, int c2, double *color) {

  int i, Ns;
  double M1, M2, dM;
  PhotCode *code;
  
  code = GetPhotcodebyCode (c1);
  if (code == NULL) return (FALSE);
  if (code[0].type == PHOT_REF) {
    for (i = 0; i < average[0].Nmeasure; i++) {
      if (measure[i].photcode == c1) {
	M1 = measure[i].M;
	goto filter1;
      }
    }	
    return (FALSE);
  } else {
    Ns = photcodes[0].hashNsec[code[0].code];
    M1 = (Ns == -1) ? NAN : secfilt[Ns].MpsfChp;
  }	

filter1:
  code = GetPhotcodebyCode (c2);
  if (code == NULL) return (FALSE);
  if (code[0].type == PHOT_REF) {
    for (i = 0; i < average[0].Nmeasure; i++) {
      if (measure[i].photcode == c2) {
	M2 = measure[i].M;
	goto filter2;
      }
    }	
    return (FALSE);
  } else {
    Ns = photcodes[0].hashNsec[code[0].code];
    M2 = (Ns == -1) ? NAN : secfilt[Ns].MpsfChp;
  }	
  
filter2:

  dM = M1 - M2;
  *color = dM;
  
  return (TRUE);
}

float MagToFlux (float Mag) {
  float Flux = pow(10.0, -0.4*(Mag));
  return (Flux);
}

/** flux conversion *******************************************************************/
float PhotFluxInst (Measure *measure, dvoMagClassType class) {

  float Moff = - measure[0].dt - ZERO_POINT;
  float Finst = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Finst = isnan (measure[0].FluxPSF)  ? MagToFlux(measure[0].M + Moff)     : measure[0].FluxPSF;
      break;
    case MAG_CLASS_KRON:
      Finst = isnan (measure[0].FluxKron) ? MagToFlux(measure[0].Mkron + Moff) : measure[0].FluxKron;
      break;
    case MAG_CLASS_APER:
      Finst = isnan (measure[0].FluxAp)   ? MagToFlux(measure[0].Map + Moff)   : measure[0].FluxAp;
      break;
    default:
      break;
  }
  return (Finst);
}

// returns Jy fluxes assuming mag in AB? (needs an extra AP factor, right?)
float PhotFluxCat (Measure *measure, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  // mag_AB = -2.5 log (flux_cgs) - 48.6 (by definition) [~Vega flux in V-band]
  // flux_Jy = flux_cgs * 10^23

  // -2.5 log (flux_cgs) = -2.5 log (flux_Jy * 10^-23) = -2.5 log (flux_Jy) + 2.5*23
  // -2.5 log (flux_cgs) = -2.5 log (flux_Jy) + 57.5

  // from these we get:
  // mag_AB = -2.5 log (flux_Jy) + 8.9
  // log (flux_Jy) = -0.4*mag_AB + 3.56

  // mag_AB = mag_inst + ZP

  // log(flux_Jy) = -0.4*mag_inst - 0.4*ZP + 3.56
  // mag_inst = -2.5 log (flux_Jy) - ZP + 8.9

  // log (flux_Jy) = -0.4*(mag_inst + ZP - 8.9)

  // -2.5 log (flux_inst) = -2.5 log (flux_Jy) - ZP + 8.9

  // log (flux_inst) = log (flux_Jy) + 0.4 ZP - 3.56

  // flux_inst = flux_Jy * 10^(0.4*ZP - 3.56)

  // flux_Jy = flux_inst * 10^(3.56 - 0.4*ZP) = flux_inst * zpFactor

  // flux_Jy = flux_inst * 10^(-0.4*(ZP - 8.9)) = flux_inst * zpFactor

  // flux_Jy = flux_inst * 3630.8 * 10^(-0.4*ZP)

  // measure.M has the static ZERO_POINT (25.0) applied, but not measure.Flux
  float Mzpt = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;
  float Moff = Mzpt - ZERO_POINT + 8.9;
  float Foff = 3630.8 * MagToFlux(Mzpt);
  float Fcat = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Fcat = isnan (measure[0].FluxPSF)  ? MagToFlux(measure[0].M + Moff)     : measure[0].FluxPSF * Foff;
      break;
    case MAG_CLASS_KRON:
      Fcat = isnan (measure[0].FluxKron) ? MagToFlux(measure[0].Mkron + Moff) : measure[0].FluxKron * Foff;
      break;
    case MAG_CLASS_APER:
      Fcat = isnan (measure[0].FluxAp)   ? MagToFlux(measure[0].Map + Moff)   : measure[0].FluxAp * Foff;
      break;
    default:
      break;
  }
  return (Fcat);
}

// corrected for color trends
float PhotFluxSys (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  // measure.M has the static ZERO_POINT (25.0) applied, but not measure.Flux
  float Mzpt = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;
  float Moff = Mzpt - ZERO_POINT + 8.9;
  float Foff = 3630.8 * MagToFlux(Mzpt);
  float Fcat = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Fcat = isnan (measure[0].FluxPSF)  ? MagToFlux(measure[0].M + Moff)     : measure[0].FluxPSF * Foff;
      break;
    case MAG_CLASS_KRON:
      Fcat = isnan (measure[0].FluxKron) ? MagToFlux(measure[0].Mkron + Moff) : measure[0].FluxKron * Foff;
      break;
    case MAG_CLASS_APER:
      Fcat = isnan (measure[0].FluxAp)   ? MagToFlux(measure[0].Map + Moff)   : measure[0].FluxAp * Foff;
      break;
    default:
      break;
  }
  if (isnan(Fcat)) return (NAN);

  // find the relevant color 
  float mc = PhotColorForCode (average, secfilt, NULL, code);
  if (isnan(mc)) return (Fcat);
  mc -= SCALE*code[0].dX;

  // find the color correction in mags
  int i = 0;
  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Fcol = MagToFlux (Mcol);

  float Fsys = Fcat * Fcol;
  return (Fsys);
}

float PhotFluxRel (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  // measure.M has the static ZERO_POINT (25.0) applied, but not measure.Flux

  // use Mcal APER for aperture-like data
  float Mcal = (class == MAG_CLASS_PSF) ? measure[0].McalPSF : measure[0].McalAPER;
  float Mflat = isfinite(measure[0].Mflat) ? measure[0].Mflat : 0.0;

  float Mzpt = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C - Mcal - Mflat;
  float Moff = Mzpt - ZERO_POINT + 8.9;
  float Foff = 3630.8 * MagToFlux(Mzpt);
  float Fcat = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Fcat = isnan (measure[0].FluxPSF)  ? MagToFlux(measure[0].M + Moff)     : measure[0].FluxPSF * Foff;
      break;
    case MAG_CLASS_KRON:
      Fcat = isnan (measure[0].FluxKron) ? MagToFlux(measure[0].Mkron + Moff) : measure[0].FluxKron * Foff;
      break;
    case MAG_CLASS_APER:
      Fcat = isnan (measure[0].FluxAp)   ? MagToFlux(measure[0].Map + Moff)   : measure[0].FluxAp * Foff;
      break;
    default:
      break;
  }
  if (isnan(Fcat)) return (NAN);

  /* for DEP, color must be made of PRI/SEC */
  float mc = PhotColorForCode (average, secfilt, NULL, code);
  if (isnan(mc)) return (Fcat);
  mc -= SCALE*code[0].dX;

  double Mc = mc;
  float Mcol = 0;
  int i = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Fcol = MagToFlux (Mcol);

  float Frel = Fcat * Fcol;
  return (Frel);
}

/* return calibrated magnitude from measure for given photcode */
float PhotFluxCal (Measure *thisone, Average *average, SecFilt *secfilt, Measure *measure, PhotCode *code, dvoMagClassType class) {

  int i; 

  if (code == NULL) return NAN;

  /* code must be the matching PRI/SEC code for this measurement or an equivalent ALT */
  int Np = photcodes[0].hashcode[thisone[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *myCode = &photcodes[0].code[Np];

  if (code->code != myCode->equiv) return (NAN);

  float Frel = PhotFluxRel (thisone, average, secfilt, class) + SCALE*code[0].C;

  // get the relevant color term
  float mc = PhotColorForCode (average, secfilt, measure, code);
  if (isnan(mc)) return (Frel);
  mc -= SCALE*code[0].dX;

  // get the corresponding color correction in mags, convert to flux
  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Fcol = MagToFlux (Mcol);

  float Fcal = Frel * Fcol;
  return (Fcal);
}

/***/
float PhotFluxAve (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return NAN;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return (NAN);

  float Fave = NAN;
  switch (source) {
    case MAG_SRC_CHP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Fave = MagToFlux(secfilt[Ns].MpsfChp - 8.9);
	  break;
	case MAG_CLASS_KRON:
	  Fave = MagToFlux(secfilt[Ns].MkronChp - 8.9);
	  break;
	case MAG_CLASS_APER:
	  Fave = MagToFlux(secfilt[Ns].MapChp - 8.9);
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_WRP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Fave = isnan (secfilt[Ns].FpsfWrp)  ? MagToFlux(secfilt[Ns].MpsfWrp - 8.9)  : secfilt[Ns].FpsfWrp;
	  break;
	case MAG_CLASS_KRON:
	  Fave = isnan (secfilt[Ns].FkronWrp) ? MagToFlux(secfilt[Ns].MkronWrp - 8.9) : secfilt[Ns].FkronWrp;
	  break;
	case MAG_CLASS_APER:
	  Fave = isnan (secfilt[Ns].FapWrp)   ? MagToFlux(secfilt[Ns].MapWrp - 8.9)   : secfilt[Ns].FapWrp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_STK:
      switch (class) {
	case MAG_CLASS_PSF:
	  Fave = isnan (secfilt[Ns].FpsfStk)  ? MagToFlux(secfilt[Ns].MpsfStk - 8.9)  : secfilt[Ns].FpsfStk;
	  break;
	case MAG_CLASS_KRON:
	  Fave = isnan (secfilt[Ns].FkronStk) ? MagToFlux(secfilt[Ns].MkronStk - 8.9) : secfilt[Ns].FkronStk;
	  break;
	case MAG_CLASS_APER:
	  Fave = isnan (secfilt[Ns].FapStk)   ? MagToFlux(secfilt[Ns].MapStk - 8.9)   : secfilt[Ns].FapStk;
	  break;
	default:
	  break;
      }
      break;
    default:
      break;
  }
  return (Fave);
}

float PhotFluxAveErr (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return NAN;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return (NAN);

  float dFave = NAN;
  switch (source) {
    case MAG_SRC_CHP:
      switch (class) {
	case MAG_CLASS_PSF:
	  dFave = secfilt[Ns].dMpsfChp * MagToFlux(secfilt[Ns].MpsfChp - 8.9);
	  break;
	case MAG_CLASS_KRON:
	  dFave = secfilt[Ns].dMkronChp * MagToFlux(secfilt[Ns].MkronChp - 8.9);
	  break;
	case MAG_CLASS_APER:
	  dFave = secfilt[Ns].dMapChp * MagToFlux(secfilt[Ns].MapChp - 8.9);
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_WRP:
      switch (class) {
	case MAG_CLASS_PSF:
	  dFave = secfilt[Ns].dFpsfWrp;
	  break;
	case MAG_CLASS_KRON:
	  dFave = secfilt[Ns].dFkronWrp;
	  break;
	case MAG_CLASS_APER:
	  dFave = secfilt[Ns].dFapWrp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_STK:
      switch (class) {
	case MAG_CLASS_PSF:
	  dFave = secfilt[Ns].dFpsfStk;
	  break;
	case MAG_CLASS_KRON:
	  dFave = secfilt[Ns].dFkronStk;
	  break;
	case MAG_CLASS_APER:
	  dFave = secfilt[Ns].dFapStk;
	  break;
	default:
	  break;
      }
      break;
    default:
      break;
  }
  return (dFave);
}

/* return calibrated magnitude from average/secfilt for given photcode */
float PhotFluxRef (PhotCode *code, Average *average, SecFilt *secfilt, Measure *measure, dvoMagClassType class, dvoMagSourceType source) {

  int i;

  float Fave = PhotFluxAve (code, average, secfilt, class, source);
  if (isnan(Fave)) return NAN;

  // correct for relative zero-point
  float Moff = SCALE*code[0].C;

  float mc = PhotColorForCode (average, secfilt, measure, code);
  if (isnan(mc)) return (Fave * MagToFlux(Moff));
  mc -= SCALE*code[0].dX;

  // correct for color terms
  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;    /* the 0.001 is needed for higher order terms to keep the units mag = mag^n */
  }
  float Foff = MagToFlux(Mcol);
  return (Fave * Foff);
}

float PhotFluxInstErr (Measure *measure, dvoMagClassType class) {

  float Moff = - measure[0].dt - ZERO_POINT;

  // use dFlux if we can, but use dMag if we must:
  // dFlux = Flux * dMag

  float dFinst = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      if (isnan (measure[0].dFluxPSF)) {
	float Finst = MagToFlux(measure[0].M + Moff);
	dFinst = measure[0].dM * Finst;
      } else {
	dFinst = measure[0].dFluxPSF;
      }
      break;
    case MAG_CLASS_KRON:
      if (isnan (measure[0].dFluxKron)) {
	float Finst = MagToFlux(measure[0].Mkron + Moff);
	dFinst = measure[0].dMkron * Finst;
      } else {
	dFinst = measure[0].dFluxKron;
      }
      break;
    case MAG_CLASS_APER:
      if (isnan (measure[0].dFluxAp)) {
	float Finst = MagToFlux(measure[0].Map + Moff);
	dFinst = measure[0].dMap * Finst;
      } else {
	dFinst = measure[0].dFluxAp;
      }
      break;
    default:
      break;
  }
  return (dFinst);
}

float PhotFluxCatErr (Measure *measure, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  // measure.M has the static ZERO_POINT (25.0) applied, but not measure.Flux
  float Mzpt = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;
  float Moff = Mzpt - ZERO_POINT + 8.9;
  float Foff = 3630.8 * MagToFlux(Mzpt);

  // use dFlux if we can, but use dMag if we must:
  // dFlux = Flux * dMag

  float dFcat = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      if (isnan (measure[0].dFluxPSF)) {
	float Finst = MagToFlux(measure[0].M + Moff);
	dFcat = measure[0].dM * Finst;
      } else {
	dFcat = measure[0].dFluxPSF * Foff;
      }
      break;
    case MAG_CLASS_KRON:
      if (isnan (measure[0].dFluxKron)) {
	float Finst = MagToFlux(measure[0].Mkron + Moff);
	dFcat = measure[0].dMkron * Finst;
      } else {
	dFcat = measure[0].dFluxKron * Foff;
      }
      break;
    case MAG_CLASS_APER:
      if (isnan (measure[0].dFluxAp)) {
	float Finst = MagToFlux(measure[0].Map + Moff);
	dFcat = measure[0].dMap * Finst;
      } else {
	dFcat = measure[0].dFluxAp * Foff;
      }
      break;
    default:
      break;
  }
  return (dFcat);
}

float PhotFluxSysErr (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  // measure.M has the static ZERO_POINT (25.0) applied, but not measure.Flux
  float Mzpt = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;
  float Moff = Mzpt - ZERO_POINT + 8.9;
  float Foff = 3630.8 * MagToFlux(Mzpt);

  // use dFlux if we can, but use dMag if we must:
  // dFlux = Flux * dMag

  float dFcat = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      if (isnan (measure[0].dFluxPSF)) {
	float Finst = MagToFlux(measure[0].M + Moff);
	dFcat = measure[0].dM * Finst;
      } else {
	dFcat = measure[0].dFluxPSF * Foff;
      }
      break;
    case MAG_CLASS_KRON:
      if (isnan (measure[0].dFluxKron)) {
	float Finst = MagToFlux(measure[0].Mkron + Moff);
	dFcat = measure[0].dMkron * Finst;
      } else {
	dFcat = measure[0].dFluxKron * Foff;
      }
      break;
    case MAG_CLASS_APER:
      if (isnan (measure[0].dFluxAp)) {
	float Finst = MagToFlux(measure[0].Map + Moff);
	dFcat = measure[0].dMap * Finst;
      } else {
	dFcat = measure[0].dFluxAp * Foff;
      }
      break;
    default:
      break;
  }

  /* color correction */
  float mc = PhotColorForCode (average, secfilt, NULL, code);
  if (isnan(mc)) return (dFcat);
  mc -= SCALE*code[0].dX;

  double Mc = mc;
  float Mcol = 0;
  int i = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Fcol = MagToFlux (Mcol);

  float dFsys = dFcat * Fcol;

  return (dFsys);
}

float PhotFluxRelErr (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  // measure.M has the static ZERO_POINT (25.0) applied, but not measure.Flux
  // XXX fix this too:
  float Mflat = isfinite(measure[0].Mflat) ? measure[0].Mflat : 0.0;
  float Mcal = (class == MAG_CLASS_PSF) ? measure[0].McalPSF : measure[0].McalAPER;

  float Mzpt = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C - Mcal - Mflat;
  float Moff = Mzpt - ZERO_POINT + 8.9;
  float Foff = 3630.8 * MagToFlux(Mzpt);

  // use dFlux if we can, but use dMag if we must:
  // dFlux = Flux * dMag

  float dFcat = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      if (isnan (measure[0].dFluxPSF)) {
	float Finst = MagToFlux(measure[0].M + Moff);
	dFcat = measure[0].dM * Finst;
      } else {
	dFcat = measure[0].dFluxPSF * Foff;
      }
      break;
    case MAG_CLASS_KRON:
      if (isnan (measure[0].dFluxKron)) {
	float Finst = MagToFlux(measure[0].Mkron + Moff);
	dFcat = measure[0].dMkron * Finst;
      } else {
	dFcat = measure[0].dFluxKron * Foff;
      }
      break;
    case MAG_CLASS_APER:
      if (isnan (measure[0].dFluxAp)) {
	float Finst = MagToFlux(measure[0].Map + Moff);
	dFcat = measure[0].dMap * Finst;
      } else {
	dFcat = measure[0].dFluxAp * Foff;
      }
      break;
    default:
      break;
  }

  /* color correction */
  float mc = PhotColorForCode (average, secfilt, NULL, code);
  if (isnan(mc)) return (dFcat);
  mc -= SCALE*code[0].dX;

  double Mc = mc;
  float Mcol = 0;
  int i = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Fcol = MagToFlux (Mcol);

  float dFrel = dFcat * Fcol;

  return (dFrel);
}

/******** alternate photometry conversion functions using MeasureTiny and AverageTiny *********/
float PhotInstTiny (MeasureTiny *measure, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  float Mraw = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Mraw = measure[0].M;
      break;
    case MAG_CLASS_KRON:
      Mraw = measure[0].Mkron;
      break;
    case MAG_CLASS_APER:
      // Mraw = measure[0].Map; // MeasureTiny does not have Map
      break;
    default:
      break;
  }
  if (code->type == PHOT_REF) {
    return (Mraw);
  }
  float Minst = Mraw - measure[0].dt - ZERO_POINT;

  return (Minst);
}

float PhotCatTiny (MeasureTiny *measure, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  float Mraw = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Mraw = measure[0].M;
      break;
    case MAG_CLASS_KRON:
      Mraw = measure[0].Mkron;
      break;
    case MAG_CLASS_APER:
      // Mraw = measure[0].Map; // MeasureTiny does not have Map
      break;
    default:
      break;
  }
  if (code->type == PHOT_REF) {
    return (Mraw);
  }

  float Mcat = Mraw - ZERO_POINT + code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;
  
  return (Mcat);
}

float PhotSysTiny (MeasureTiny *measure, AverageTiny *average, SecFilt *secfilt, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  float Mraw = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Mraw = measure[0].M;
      break;
    case MAG_CLASS_KRON:
      Mraw = measure[0].Mkron;
      break;
    case MAG_CLASS_APER:
      // Mraw = measure[0].Map; // MeasureTiny does not have Map
      break;
    default:
      break;
  }
  if (code->type == PHOT_REF) {
    return (Mraw);
  }
  float Mcat = Mraw - ZERO_POINT + code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C;

  /* for DEP, color must be made of PRI/SEC */
  float mc = PhotColorForCodeTiny (average, secfilt, NULL, code);
  if (isnan(mc)) return (Mcat);
  mc -= SCALE*code[0].dX;

  int i = 0;
  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Msys = Mcat + Mcol;
  return (Msys);
}

float PhotRelTiny (MeasureTiny *measure, AverageTiny *average, SecFilt *secfilt, dvoMagClassType class) {

  int Np = photcodes[0].hashcode[measure[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *code = &photcodes[0].code[Np];

  float Mraw = NAN;
  float Mcal = NAN;
  switch (class) {
    case MAG_CLASS_PSF:
      Mraw = measure[0].M;
      Mcal = measure[0].McalPSF;
      break;
    case MAG_CLASS_KRON:
      Mraw = measure[0].Mkron;
      Mcal = measure[0].McalAPER;
      break;
    case MAG_CLASS_APER:
      // Mraw = measure[0].Map; // MeasureTiny does not have Map
      // Mcal = measure[0].McalAPER;
      break;
    default:
      break;
  }
  if (code->type == PHOT_REF) {
    return (Mraw);
  }
  float Mflat = isfinite(measure[0].Mflat) ? measure[0].Mflat : 0.0;
  float Mcat = Mraw - ZERO_POINT + code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C - Mcal - Mflat;

  /* for DEP, color must be made of PRI/SEC */
  float mc = PhotColorForCodeTiny (average, secfilt, NULL, code);
  if (isnan(mc)) return (Mcat);
  mc -= SCALE*code[0].dX;

  double Mc = mc;
  float Mcol = 0;
  int i = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  float Mrel = Mcat + Mcol;
  return (Mrel);
}

/* return calibrated magnitude from measure for given photcode */
float PhotCalTiny (MeasureTiny *thisone, AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, PhotCode *code, dvoMagClassType class) {

  int i; 

  if (code == NULL) return NAN;

  /* code must be the matching PRI/SEC code for this measurement or an equivalent ALT */
  int Np = photcodes[0].hashcode[thisone[0].photcode];
  if (Np == -1) return (NAN);
  PhotCode *myCode = &photcodes[0].code[Np];

  if (code->code != myCode->equiv) return (NAN);

  // if we are REF type, just get the right version and return
  if (myCode->type == PHOT_REF) {
    float Mraw = NAN;
    switch (class) {
      case MAG_CLASS_PSF:
	Mraw = thisone[0].M;
	break;
      case MAG_CLASS_KRON:
	Mraw = thisone[0].Mkron;
	break;
      case MAG_CLASS_APER:
	// Mraw = thisone[0].Map; // MeasureTiny does not have Map
	break;
      default:
	break;
    }
    return (Mraw);
  }

  float Mcal = PhotRelTiny (thisone, average, secfilt, class) + SCALE*code[0].C;

  float mc = PhotColorForCodeTiny (average, secfilt, measure, code);
  if (isnan(mc)) return (Mcal);
  mc -= SCALE*code[0].dX;

  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;
  }
  Mcal += Mcol;
  return (Mcal);
}

/***/
float PhotAveTiny (PhotCode *code, AverageTiny *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source) {
  OHANA_UNUSED_PARAM(average);

  if (code == NULL) return NAN;

  int Ns = photcodes[0].hashNsec[code[0].code];
  if (Ns == -1) return (NAN);

  float Mave = NAN;
  switch (source) {
    case MAG_SRC_CHP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Mave = secfilt[Ns].MpsfChp;
	  break;
	case MAG_CLASS_KRON:
	  Mave = secfilt[Ns].MkronChp;
	  break;
	case MAG_CLASS_APER:
	  Mave = secfilt[Ns].MapChp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_WRP:
      switch (class) {
	case MAG_CLASS_PSF:
	  Mave = secfilt[Ns].MpsfWrp;
	  break;
	case MAG_CLASS_KRON:
	  Mave = secfilt[Ns].MkronWrp;
	  break;
	case MAG_CLASS_APER:
	  Mave = secfilt[Ns].MapWrp;
	  break;
	default:
	  break;
      }
      break;
    case MAG_SRC_STK:
      switch (class) {
	case MAG_CLASS_PSF:
	  Mave = secfilt[Ns].MpsfStk;
	  break;
	case MAG_CLASS_KRON:
	  Mave = secfilt[Ns].MkronStk;
	  break;
	case MAG_CLASS_APER:
	  Mave = secfilt[Ns].MapStk;
	  break;
	default:
	  break;
      }
      break;
    default:
      break;
  }
  return (Mave);
}

/* return calibrated magnitude from average/secfilt for given photcode */
float PhotRefTiny (PhotCode *code, AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, dvoMagClassType class, dvoMagSourceType source) {

  int i;

  float Mave = PhotAveTiny (code, average, secfilt, class, source);
  if (isnan(Mave)) return NAN;

  // correct for relative zero-point
  float Mref = Mave + SCALE*code[0].C;

  float mc = PhotColorForCodeTiny (average, secfilt, measure, code);
  if (isnan(mc)) return (Mref);
  mc -= SCALE*code[0].dX;

  // correct for color terms
  double Mc = mc;
  float Mcol = 0;
  for (i = 0; i < code[0].Nc; i++) {
    Mcol += code[0].X[i]*Mc;
    Mc *= mc;    /* the 0.001 is needed for higher order terms to keep the units mag = mag^n */
  }
  Mref += Mcol;
  return (Mref);
}

/* color term may not use DEP magnitude */
float PhotColorForCodeTiny (AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, PhotCode *code) {

  int i, Ns1, Ns2, Ns;
  float m1, m2, mc;
  PhotCode *color;

  m1 = m2 = NAN;

  if (measure == NULL) {
    Ns1 = photcodes[0].hashNsec[code[0].c1];
    Ns2 = photcodes[0].hashNsec[code[0].c2];
  
    m1 = (Ns1 == -1) ? NAN : secfilt[Ns1].MpsfChp;
    m2 = (Ns2 == -1) ? NAN : secfilt[Ns2].MpsfChp;
    mc = (isnan(m1) || isnan(m2)) ? NAN : (m1 - m2);
    return (mc);
  }

  /* find magnitude matching first color term */
  color = GetPhotcodebyCode (code[0].c1);
  if (color == NULL) return (NAN);
  if (color[0].type == PHOT_REF) {
    for (i = 0; (i < average[0].Nmeasure) && (isnan(m1)); i++) {
      if (measure[i].photcode == color[0].code) {
	m1 = measure[i].M;
      }
    }
  } else {
    Ns = photcodes[0].hashNsec[color[0].code];
    m1 = (Ns == -1) ? NAN : secfilt[Ns].MpsfChp;
  }	

  /* find magnitude matching second color term */
  color = GetPhotcodebyCode (code[0].c2);
  if (color == NULL) return (NAN);
  if (color[0].type == PHOT_REF) {
    for (i = 0; (i < average[0].Nmeasure) && (isnan(m2)); i++) {
      if (measure[i].photcode == color[0].code) {
	m2 = measure[i].M;
      }
    }
  } else {
    Ns = photcodes[0].hashNsec[color[0].code];
    m2 = (Ns == -1) ? NAN : secfilt[Ns].MpsfChp;
  }	
  mc = (isnan(m1) || isnan(m2)) ? NAN : (m1 - m2);
  return (mc);
}

float PhotdMTiny (PhotCode *code, AverageTiny *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float dM;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  dM  = (Ns == -1) ? NAN : secfilt[Ns].dMpsfChp;
  return (dM);
}

// Xm is now (2014.07.03) stored as the chisq except in dvo formats which use as short
float PhotXmTiny (PhotCode *code, AverageTiny *average, SecFilt *secfilt) {
  OHANA_UNUSED_PARAM(average);

  int Ns;
  float Xm;

  if (code == NULL) return NAN;

  Ns = photcodes[0].hashNsec[code[0].code];
  Xm = (Ns == -1) ? NAN : secfilt[Ns].Mchisq;
  return (Xm);
}

/* given a photcode pair c1 & c2, return the color of this star (NaN if not found) */
int PhotColorTiny (AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, int c1, int c2, double *color) {

  int i, Ns;
  double M1, M2, dM;
  PhotCode *code;
  
  code = GetPhotcodebyCode (c1);
  if (code == NULL) return (FALSE);
  if (code[0].type == PHOT_REF) {
    for (i = 0; i < average[0].Nmeasure; i++) {
      if (measure[i].photcode == c1) {
	M1 = measure[i].M;
	goto filter1;
      }
    }	
    return (FALSE);
  } else {
    Ns = photcodes[0].hashNsec[code[0].code];
    M1 = (Ns == -1) ? NAN : secfilt[Ns].MpsfChp;
  }	

filter1:
  code = GetPhotcodebyCode (c2);
  if (code == NULL) return (FALSE);
  if (code[0].type == PHOT_REF) {
    for (i = 0; i < average[0].Nmeasure; i++) {
      if (measure[i].photcode == c2) {
	M2 = measure[i].M;
	goto filter2;
      }
    }	
    return (FALSE);
  } else {
    Ns = photcodes[0].hashNsec[code[0].code];
    M2 = (Ns == -1) ? NAN : secfilt[Ns].MpsfChp;
  }	
  
filter2:

  dM = M1 - M2;
  *color = dM;
  
  return (TRUE);
}
/***********************************************/


/* photcode table should have the following format: 

# code name     type  zero  airmass  offset  c1 c2  slope  <color>  primary
1    B        pri   24.0  0.15     -       -  -   -      -        -
2    B        pri   24.0  0.15     -       -  -   -      -        -
3    B1       sec   22.5  0.18     0.15    1  2   0.10   0.50     1
1000 USNO_B   ref   -     -        -       -  -   -      -        -

*/


/*
  Nc1 = photcodes[0].code[Np].c1;
  Ns1 = photcodes[0].hashNsec[Nc1];

  Nc2 = photcodes[0].code[Np].c2;
  Ns2 = photcodes[0].hashNsec[Nc2];

  Xlam = photcodes[0].code[Np].X[0];
  Klam = photcodes[0].code[Np].K;
  
  m1 = (Ns1 == -1) ? average[0].M : secfilt[Ns1].M;
  m2 = (Ns2 == -1) ? average[0].M : secfilt[Ns2].M;
*/

/*** lensing *********************************************************************************/

// lensobj points an Nsecfilt entries, if any exist.  note comment in dvolens/src/update_objects_catalog.c:32
// >> Nlensobj tracks how many lensobj entries have I actually generated so far for this
// >> catalog This is not necessarily 1-to-1 with secfilt entries, but any average entry
// >> will either have 0 or Nsecfilt lensobj entries

# define LENSFIELD(NAME)						\
  float LensValue_##NAME (PhotCode *code, Lensobj *lensobj, int Nlensobj) { \
    if (lensobj == NULL) return NAN;					\
    if (code == NULL) return NAN;					\
    if (Nlensobj < 1) return NAN;					\
    for (int i = 0; i < Nlensobj; i++) {				\
      if (lensobj[i].photcode == code->code) return (lensobj[i].NAME);	\
    }									\
    return (NAN);							\
  }

LENSFIELD(X11_sm_obj)
LENSFIELD(X12_sm_obj)
LENSFIELD(X22_sm_obj)
LENSFIELD(E1_sm_obj)
LENSFIELD(E2_sm_obj)

LENSFIELD(X11_sh_obj)
LENSFIELD(X12_sh_obj)
LENSFIELD(X22_sh_obj)
LENSFIELD(E1_sh_obj)
LENSFIELD(E2_sh_obj)

LENSFIELD(X11_sm_psf)
LENSFIELD(X12_sm_psf)
LENSFIELD(X22_sm_psf)
LENSFIELD(E1_sm_psf)
LENSFIELD(E2_sm_psf)

LENSFIELD(X11_sh_psf)
LENSFIELD(X12_sh_psf)
LENSFIELD(X22_sh_psf)
LENSFIELD(E1_sh_psf)
LENSFIELD(E2_sh_psf)

LENSFIELD( F_ApR5)
LENSFIELD(dF_ApR5)
LENSFIELD(sF_ApR5)
LENSFIELD(fF_ApR5)

LENSFIELD( F_ApR6)
LENSFIELD(dF_ApR6)
LENSFIELD(sF_ApR6)
LENSFIELD(fF_ApR6)

LENSFIELD( F_ApR7)
LENSFIELD(dF_ApR7)
LENSFIELD(sF_ApR7)
LENSFIELD(fF_ApR7)

LENSFIELD(E1)
LENSFIELD(E2)

# if (0)
float GalphotValue_GAL_MAG (PhotCode *code, dvoMagClassType class, GalPhot *galphot, int Ngalphot) { 
  int n;								 
  if (code == NULL) return NAN;					 
  for (n = 0; n < Ngalphot; n++) {					 
    short equivCode = GetPhotcodeEquivCodebyCode (galphot[n].photcode); 
    if (!equivCode) continue;						
    if (equivCode != code->code) continue;				
    switch (class) {							
      case MAG_CLASS_SER:						
	if (galphot[n].modelType != 5) continue;			
	break;							
      case MAG_CLASS_EXP:						
	if (galphot[n].modelType != 6) continue;			
	break;							
      case MAG_CLASS_DEV:						
	if (galphot[n].modelType != 7) continue;			
	break;							
      default:							
	return NAN;							
    }									
    float value = galphot[n].mag;					
    return (value);							
  }									
  return NAN;								
}
# endif

// the block above is an example of the generic version below

# define GALPHOT_FIELD(NAME, VALUE, TYPE, DEFAULT)				\
  TYPE GalphotValue_##NAME (PhotCode *code, dvoMagClassType class, GalPhot *galphot, int Ngalphot) { \
    int n;								\
    if (code == NULL) return DEFAULT;					\
    for (n = 0; n < Ngalphot; n++) {					\
      short equivCode = GetPhotcodeEquivCodebyCode (galphot[n].photcode); \
      if (!equivCode) continue;						\
      if (equivCode != code->code) continue;				\
      switch (class) {							\
	case MAG_CLASS_SER:						\
	  if (galphot[n].modelType != 5) continue;			\
	  break;							\
	case MAG_CLASS_EXP:						\
	  if (galphot[n].modelType != 6) continue;			\
	  break;							\
	case MAG_CLASS_DEV:						\
	  if (galphot[n].modelType != 7) continue;			\
	  break;							\
	default:							\
	  return DEFAULT;						\
      }									\
      TYPE value = galphot[n].VALUE;					\
      return (value);							\
    }									\
    return DEFAULT;							\
  }

GALPHOT_FIELD(GAL_MAG, 	       mag,          float, NAN) 
GALPHOT_FIELD(GAL_MAG_ERR,     magErr,       float, NAN) 
GALPHOT_FIELD(GAL_MAJ, 	       majorAxis,    float, NAN) 
GALPHOT_FIELD(GAL_MAJ_ERR,     majorAxisErr, float, NAN)
GALPHOT_FIELD(GAL_MIN, 	       minorAxis,    float, NAN) 
GALPHOT_FIELD(GAL_MIN_ERR,     minorAxisErr, float, NAN) 
GALPHOT_FIELD(GAL_THETA,       theta,        float, NAN) 
GALPHOT_FIELD(GAL_THETA_ERR,   thetaErr,     float, NAN) 
GALPHOT_FIELD(GAL_INDEX,       index,        float, NAN)      
GALPHOT_FIELD(GAL_CHISQ,       chisq,        float, NAN)      
GALPHOT_FIELD(GAL_NPIX,        Npix,         float, NAN)       

GALPHOT_FIELD(GAL_TYPE,        modelType,    short, 0)       

GALPHOT_FIELD(GAL_FLAGS,       flags,        unsigned int, 0)       
GALPHOT_FIELD(GAL_OBJ_ID,      objID,        unsigned int, 0)       
GALPHOT_FIELD(GAL_CAT_ID,      catID,        unsigned int, 0)       
GALPHOT_FIELD(GAL_DET_ID,      detID,        unsigned int, 0)       
GALPHOT_FIELD(GAL_IMAGE_ID,    imageID,      unsigned int, 0)       
