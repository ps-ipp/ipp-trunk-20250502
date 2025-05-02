/*                                             -*- c-file-style: "Ellemtel" -*-

`fh_rt.h' - FITS RealTime routines

This file is part of version 1 of the FITS Handling Library.
Read the `License' file for terms of use and distribution.
Copyright 2001, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */
#ifndef _INCLUDED_fh_rt
#define _INCLUDED_fh_rt 1

fh_bool fh_rt_present(HeaderUnit hu);
fh_bool fh_rt_active(HeaderUnit hu);
int     fh_rt_version(HeaderUnit hu);
int     fh_rt_plane_completed(HeaderUnit hu); /* Returns 0..(NAXIS3-1) */
int     fh_rt_plane_latest(HeaderUnit hu);    /* Returns 0..(NAXIS3-1) */
int     fh_rt_plane_row_latest(HeaderUnit hu, int plane); /* 0..(NAXIS2-1) */

#endif
