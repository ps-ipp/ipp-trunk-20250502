/*                                             -*- c-file-style: "Ellemtel" -*-

`macros.h' - CPP macros to make fhreg work, automatically included

This file is part of version 0 of the FITS Keyword Registry.
Read the `License' file for terms of use and distribution.
Copyright 2004, Pan-STARRS, isani@ifa.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

/*
 * Documentation: See http://software.cfht.hawaii.edu/fh_registry/
 *
 * === NOTE: That only describes the CFHT version of the registry!
 */

#ifndef _INCLUDED_fhreg_macros
#define _INCLUDED_fhreg_macros 1

/*
 * This macro is used inside the other macros (ID_FLT, ID_INT, etc.)
 * so that the keyword names can be referred to as fh_kw_FOO
 * This can be used as an identifier to fh_get() or any other function
 * instead of using the string constant "FOO".
 */

#define ID_FLT(idx, name, keyword, prec, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, double value) \
{ fh_set_flt(hu, idx, keyword, value, prec, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, double value, const char* new_comment) \
{ fh_set_flt(hu, idx, keyword, value, prec, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, double* value) \
{ return fh_get_flt(hu, keyword, value); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_PFL(idx, name, keyword, prec, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, double value) \
{ fh_set_pfl(hu, idx, keyword, value, prec, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, double value, const char* new_comment) \
{ fh_set_pfl(hu, idx, keyword, value, prec, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, double* value) \
{ return fh_get_flt(hu, keyword, value); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_INT(idx, name, keyword, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, int value) \
{ fh_set_int(hu, idx, keyword, value, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, int value, const char* new_comment) \
{ fh_set_int(hu, idx, keyword, value, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, int* value) \
{ return fh_get_int(hu, keyword, value); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_BLN(idx, name, keyword, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, fh_bool value) \
{ fh_set_bool(hu, idx, keyword, value, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, fh_bool value, const char* new_comment) \
{ fh_set_bool(hu, idx, keyword, value, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, fh_bool* value) \
{ return fh_get_bool(hu, keyword, value); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_STR(idx, name, keyword, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, const char* value) \
{ fh_set_str(hu, idx, keyword, value, comment); } \
static inline void \
fh_setval_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, const char* value, const char* new_comment) \
{ fh_set_str(hu, idx, keyword, value, new_comment); } \
static inline fh_result \
fh_get_##name (HeaderUnit hu, char* value, int maxlen) \
{ return fh_get_str(hu, keyword, value, maxlen); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_VAL(idx, name, keyword, comment) \
static inline void \
fh_set_##name (HeaderUnit hu, const char* value) \
{ fh_set_val(hu, idx, keyword, value, comment); } \
static inline void \
fh_setcmt_##name (HeaderUnit hu, const char* value, const char* new_comment) \
{ fh_set_val(hu, idx, keyword, value, new_comment); } \
static inline const char* \
fh_kw_##name(void) { return keyword; } \
static inline const char* \
fh_cmt_##name(void) { return comment; }

#define ID_CMT(idx, name, comment) \
static inline void \
fh_set_##name (HeaderUnit hu) \
{ fh_set_com(hu, idx, "COMMENT", comment); }

/*
 * Now the auto-stringified versions of ID_*() ...
 */
#define MK_FLT(idx, name, dig, comment) ID_FLT(idx, name, #name, dig, comment)
#define MK_PFL(idx, name, dec, comment) ID_PFL(idx, name, #name, dec, comment)
#define MK_INT(idx, name, comment) ID_INT(idx, name, #name, comment)
#define MK_STR(idx, name, comment) ID_STR(idx, name, #name, comment)
#define MK_VAL(idx, name, comment) ID_VAL(idx, name, #name, comment)
#define MK_BLN(idx, name, comment) ID_BLN(idx, name, #name, comment)
#define MK_CMT(idx, name, comment) ID_CMT(idx, name, comment)

#endif /* !_INCLUDED_fhreg_macros */
