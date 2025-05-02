#ifndef PP_SIM_SEQUENCE_H
#define PP_SIM_SEQUENCE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <pslib.h>
#include <psmodules.h>
#include <psastro.h>

bool ppSimSequenceBias 	 (FILE *simfile, FILE *inject, psMetadata *sequence, int nSeq, psRandom *rng, const char *path, const char *basename, const char *ppSimCommand, const char *injectCommand, psArray *files);
bool ppSimSequenceDark 	 (FILE *simfile, FILE *inject, psMetadata *sequence, int nSeq, psRandom *rng, const char *path, const char *basename, const char *ppSimCommand, const char *injectCommand, psArray *files);
bool ppSimSequenceFlat 	 (FILE *simfile, FILE *inject, psMetadata *sequence, int nSeq, psRandom *rng, const char *path, const char *basename, const char *ppSimCommand, const char *injectCommand, psArray *files);
bool ppSimSequenceObject (FILE *simfile, FILE *inject, psMetadata *sequence, int nSeq, psRandom *rng, const char *path, const char *basename, const char *refcat, const char *ppSimCommand, const char *injectCommand, psArray *files);

#endif
