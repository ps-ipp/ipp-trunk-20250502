/*                                             -*- c-file-style: "Ellemtel" -*-

`fh_validate.c' - A routine to validate the completeness of header.

This file is part of version 1 of the FITS Handling Library.
Read the `License' file for terms of use and distribution.
Copyright 2001, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

#include <stdio.h>
#include <stdlib.h>

#include "fh.h"

fh_result
fh_validate(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   fh_result rtn = FH_SUCCESS;
   int i, xtension=0, naxis=0, bitpix=0;
   int warned_comment = 0;
   char* s;

   fh_sort(list);

   for (i=0; i<list->len; i++)
   {
      s = list->hdr[i]->card;

      if (fix_characters(s, /*repair=*/1) != FH_SUCCESS)
      {
	 if (rtn == FH_SUCCESS) rtn = FH_BAD_VALUE;
	 fprintf(stderr, "error: libfh: Illegal character(s) in FITS card [%.8s]\n", s);
      }
      else
      {
	 if (!memcmp(s, "COMMENT ", 8) ||
	     !memcmp(s, "HISTORY ", 8) ||
	     !memcmp(s, "        ", 8))
	 {
	    if (s[8] == '=')
	    {
	       rtn = FH_BAD_VALUE;
	       if (!warned_comment++)
		  fprintf(stderr,
			  "warning: libfh: FITS standard recommends COMMENT/HISTORY not begin with '='\n");
	    }
	 }
	 else
	 {
	    if (s[8] == '=')
	    {
	       int j;
	       fh_bool k;
	       char tmp[80];

	       if (s[9] != ' ')
	       {
		  rtn = FH_INVALID;
		  fprintf(stderr,
			  "error: libfh: Missing ' ' character after [%.9s]; Not correcting.\n", s);
	       }
	       /*
		* Search for duplicated cards
		*/
	       for (j = i-1; j>=0; j--)
	       {
		  if (!memcmp(s, list->hdr[j]->card, 8))
		  {
		     rtn = FH_INVALID;
		     fprintf(stderr,
			     "error: libfh: Line %d:[%.80s] duplicates previous line %d:[%.80s]\n",
			     i, s, j, list->hdr[j]->card);
		  }
	       }
	       for (j = 10; j < 80; j++)
		  if (s[j] != ' ') break;

	       if (j < 80) switch (s[j])
	       {
		  case '\'': /* Check for fixed format string */
		  {
		     if (j != 10 || fh_get_str(list, s, tmp, sizeof(tmp)) != FH_SUCCESS)
		     {
			rtn = FH_BAD_VALUE;
			fprintf(stderr, "warning: libfh: String [%.80s] is not in \"fixed\" format.\n", s);
		     }
		     break;
		  }
		  case 'T': /* Check for fixed format boolean */
		  case 'F':
		  {
		     if (j != 29 || fh_get_bool(list, s, &k) != FH_SUCCESS)
		     {
			rtn = FH_BAD_VALUE;
			fprintf(stderr, "error: libfh: Boolean [%.8s] is not in \"fixed\" format.\n", s);
		     }
		     for (j = 31; j < 80; j++)
			if (s[j] != ' ') break;
		     if (j < 80 && s[j] != '/')
		     {
			rtn = FH_BAD_VALUE;
			fprintf(stderr, "error: libfh: Garbage after boolean [%.8s] value field.\n", s);
		     }
		     break;
		  }
		  case '/': /* No value, only comment field */
		  {
		     if (j < 31)
		     {
			rtn = FH_BAD_VALUE;
			fprintf(stderr, "error: libfh: Comment field for [%.8s] begins before column 32\n", s);
		     }
		     break;
		  }
		  case '(': /* Complex value.  No checking (no fixed format either.) */
		     break;
		  default:
		  {
		     for (k=j; k < 80; k++) /* Find the end of the value. */
			if (s[k] == ' ') break;
		     if ((j > 10 && k != 30) || (k < 30))
		     {
			rtn = FH_BAD_VALUE;
			fprintf(stderr, "error: libfh: Value of [%.8s] is not in \"fixed\" format.\n", s);
		     }
		  }
	       } /* switch (s[j]) */
	    }
	    else
	    {
	       /*
		* The '=' sign is missing, so this should be treated as a comment
		* line.  However, since we do not use any other comment cards
		* in our files other than COMMENT HISTORY or "        ", this is
		* probably an error.
		*/
	       rtn = FH_INVALID;
	       fprintf(stderr,
		       "warning: libfh: Treating [%.8s] as a comment card (No '=' in column 9)\n", s);
	    }
	 }

	 switch (i)
	 {
	    case 0: /* must be SIMPLE T or XTENSION */
	    {
	       if (!memcmp(s, "XTENSION", 8))
		  xtension=1;
	       else
	       {
		  if (memcmp(s, "SIMPLE  =                    T ", 31))
		  {
		     rtn = FH_INVALID;
		     fprintf(stderr, "error: libfh: First card is not \"SIMPLE  = ... T\"\n");
		  }
	       }
	       break;
	    }
            case 1: /* must be BITPIX */
	    {
	       if (memcmp(s, "BITPIX  ", 8) || fh_get_int(list, "BITPIX", &bitpix) != FH_SUCCESS)
	       {
		  rtn = FH_INVALID;
		  fprintf(stderr, "error: libfh: Second card is not \"BITPIX  = \"\n");
		  bitpix = 0;
	       }
	       else switch (bitpix)
	       {
		  case 8:
		  case 16:
		  case 32:
		  case -32:
		  case -64: break; /* Valid */
		  case 0: if (xtension) break;
		  default:
		  {
		     rtn = FH_INVALID;
		     fprintf(stderr, "error: libfh: Non-standard BITPIX: %d\n", bitpix);
		  }
	       }
	       break;
	    }
	    case 2: /* must be NAXIS */
	    {
	       if (memcmp(s, "NAXIS   ", 8) || fh_get_int(list, "NAXIS", &naxis) != FH_SUCCESS)
	       {
		  rtn = FH_INVALID;
		  fprintf(stderr, "error: libfh: Third card is not \"NAXIS   = \"\n");
	       }
	       break;
	    }
            default: /* NAXIS??? must be next, then PCOUNT, GCOUNT if XTENSION */
	    {
	       if (i <= naxis + 2)
	       {
		  char name[9] = "        ";
		  
		  sprintf(name, "NAXIS%d", i - 2);
		  if (fh_cmp(s, name) < MATCH_KEYWORD)
		  {
		     rtn = FH_INVALID;
		     fprintf(stderr, "error: libfh: Found [%.8s] instead of [%s].\n",
			     s, name);
		  }
	       }
	       else if (xtension && i == naxis + 2 + 1)
	       {
		  if (memcmp(s, "PCOUNT  ", 8))
		  {
		     rtn = FH_INVALID;
		     fprintf(stderr,
			     "error: libfh: Extension has [%.8s] where PCOUNT should be.\n",
			     s);
		  }
	       }
	       else if (xtension && i == naxis + 2 + 2)
	       {
		  if (memcmp(s, "GCOUNT  ", 8))
		  {
		     rtn = FH_INVALID;
		     fprintf(stderr, "error: libfh: Extension has [%.8s] where GCOUNT should be.\n",
			     s);
		  }
	       }
	    }
	 }
      }
   }

   return rtn;
}
