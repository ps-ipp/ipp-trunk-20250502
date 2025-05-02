/*                                             -*- c-file-style: "Ellemtel" -*-

`fh_rt.c' - FITS RealTime routines

This file is part of version 1 of the FITS Handling Library.
Read the `License' file for terms of use and distribution.
Copyright 2001, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

#include "fh.h"
#include "fh_rt.h"

fh_bool
fh_rt_present(HeaderUnit hu)
{
}

fh_bool
fh_rt_active(HeaderUnit hu)
{
   if (fh_rt_present(hu) == FH_FALSE) then return FH_FALSE;
}

int
fh_rt_version(HeaderUnit hu)
{
   if (fh_rt_active(hu) == FH_FALSE)
      return ...;
}

int
fh_rt_plane_completed(HeaderUnit hu)
{
   if (fh_rt_active(hu) == FH_FALSE)

}

int
fh_rt_plane_latest(HeaderUnit hu)
{
}

int
fh_rt_plane_row_latest(HeaderUnit hu, int plane)
{
}
