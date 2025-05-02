#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

#include "psType.h"
#include "psError.h"
#include "psAbort.h"
#include "psTrace.h"
#include "psMetadata.h"
#include "psMetadataItemCompare.h"

// Parse a string according to some provided format; check for leftovers
#define PARSE_STRING(GOOD, RESULT, STRING, FORMAT) \
    bool GOOD = true; \
    { \
        int read;                       /* Number of characters read */ \
        if (sscanf(STRING, "%" FORMAT "%n", &(RESULT), &read) <= 0) { \
            GOOD = false; \
        } \
        if (read < strlen(STRING)) { \
            /* Something's left over in the string, so they don't match */ \
            GOOD = false; \
        } \
    }

// Parse a string as a sexagesimal value
static bool parseSexagesimal(double *value, const char *string)
{
    int big, medium;                    // Big and medium-sized values
    float small;                        // Small value
    int read;                           // Number of characters read
    if (sscanf(string, "%d:%d:%f%n", &big, &medium, &small, &read) != 3 &&
        sscanf(string, "%d %d %f%n", &big, &medium, &small, &read) != 3) {
        return false;
    }
    if (read < strlen(string)) {
        // Something's left over in the string, so they don't match
        return false;
    }
    *value = abs(big) + (float)medium/60.0 + small/3600.0;
    if (big < 0) {
        *value *= -1.0;
    }
    return true;
}

// Parse a string as a boolean value
static bool parseBool(bool *value, const char *string)
{
    if (strcasecmp(string, "T") == 0 || strcasecmp(string, "TRUE") == 0 || strcmp(string, "1") == 0) {
        *value = true;
        return true;
    }
    if (strcasecmp(string, "F") == 0 || strcasecmp(string, "FALSE") == 0 || strcmp(string, "0") == 0) {
        *value = false;
        return true;
    }
    return false;
}


psMetadataItemCompareOp psMetadataItemCompareOperation(const psMetadataItem *item)
{
    // Default is equality
    if (!item || !item->comment) {
        return PS_METADATA_ITEM_COMPARE_OP_NONE;
    }

    char *p1 = strstr(item->comment, "OP:");
    if (!p1) {
        return PS_METADATA_ITEM_COMPARE_OP_NONE;
    }
    p1 += 3; // point to first char after @OP:
    while (isblank(p1[0])) {
        p1++;
    }

    // XXX a bit crude: does not catch the case of invalid chars after boolean op
    if (!strncmp(p1, "==", 2)) return PS_METADATA_ITEM_COMPARE_OP_EQ;
    if (!strncmp(p1, "=",  1)) return PS_METADATA_ITEM_COMPARE_OP_EQ;
    if (!strncmp(p1, "<=", 2)) return PS_METADATA_ITEM_COMPARE_OP_LE;
    if (!strncmp(p1, "<",  1)) return PS_METADATA_ITEM_COMPARE_OP_LT;
    if (!strncmp(p1, ">=", 2)) return PS_METADATA_ITEM_COMPARE_OP_GE;
    if (!strncmp(p1, ">",  1)) return PS_METADATA_ITEM_COMPARE_OP_GT;
    if (!strncmp(p1, "!",  1)) return PS_METADATA_ITEM_COMPARE_OP_NE;

    return PS_METADATA_ITEM_COMPARE_OP_NONE;
}

// XXX better value for tolerance?
// XXX Put tolerance in metadata comment?
#define EQ_TOL 1e-6                     // Tolerance for equality

// Compare values directly
#define COMPARE_VALUES(TEMPLATE, COMPARE, FLOATINGPOINT) { \
     /* does template specify a boolean operation in comment? */ \
     psMetadataItemCompareOp op = psMetadataItemCompareOperation(template);  \
     psTrace("psLib.types", 10, "Comparing %f %x %f\n", (float)(COMPARE), op, (float)(TEMPLATE)); \
     switch (op) { \
       case PS_METADATA_ITEM_COMPARE_OP_NONE: /* Default is equality */ \
       case PS_METADATA_ITEM_COMPARE_OP_EQ: \
         if (FLOATINGPOINT) { \
             return fabs((TEMPLATE) - ((COMPARE))) < EQ_TOL; \
         } else { \
             return (TEMPLATE) == (COMPARE); \
         } \
       case PS_METADATA_ITEM_COMPARE_OP_LT: \
         return (COMPARE) < (TEMPLATE); \
       case PS_METADATA_ITEM_COMPARE_OP_LE: \
         return (COMPARE) <= (TEMPLATE); \
       case PS_METADATA_ITEM_COMPARE_OP_GT: \
         return (COMPARE) > (TEMPLATE); \
       case PS_METADATA_ITEM_COMPARE_OP_GE: \
         return (COMPARE) >= (TEMPLATE); \
       case PS_METADATA_ITEM_COMPARE_OP_NE: \
         return (COMPARE) != (TEMPLATE); \
       default: \
         psAbort("all cases should have been handled..."); \
    } \
}

#define COMPARE_NUMERICAL_CASE(TEMPLATENAME, TEMPLATETYPE, COMPARENAME, COMPARETYPENAME) \
  case PS_TYPE_##COMPARETYPENAME: { \
      TEMPLATETYPE valueC = (TEMPLATETYPE)compare->data.COMPARENAME; \
      TEMPLATETYPE valueT = (TEMPLATETYPE)template->data.TEMPLATENAME;	\
      /* XXX check the validiy of the type casting? */			\
      if (valueC != compare->data.COMPARENAME) {			\
	return false;							\
      }									\
      COMPARE_VALUES(valueT, valueC, (template->type == PS_DATA_F32 || template->type == PS_DATA_F64)); \
      /* COMPARE_VALUES(valueT, valueC, 1);				*/  \
      psAbort("Should never reach here."); \
  }

// Compare a string template with an int
#define COMPARE_STRING_INT_CASE(TYPE, FORMAT) \
  case PS_TYPE_##TYPE: { \
    ps##TYPE valueT; \
    PARSE_STRING(status, valueT, template->data.V, FORMAT); \
    if (!status) { \
        return false; \
    } \
    ps##TYPE valueC = compare->data.TYPE; \
    COMPARE_VALUES(valueT, valueC, false); \
}


// Compare a boolean template with a string
#define COMPARE_BOOL_STRING(TYPE, FORMAT) { \
    bool valueC; \
    if (!parseBool(&valueC, compare->data.V)) { \
        return false; \
    } \
    return (template->data.B == valueC); \
}

// Compare an integer template with a string
#define COMPARE_INT_STRING(TYPE, FORMAT) { \
    ps##TYPE valueC; \
    PARSE_STRING(status, valueC, compare->data.V, FORMAT); \
    if (!status) { \
        return false; \
    } \
    ps##TYPE valueT = template->data.TYPE; \
    COMPARE_VALUES(valueT, valueC, false); \
}

// Compare a float template with a string
#define COMPARE_FLOAT_STRING(TYPE, FORMAT) { \
    ps##TYPE valueC; \
    PARSE_STRING(status, valueC, compare->data.V, FORMAT); \
    if (!status) { \
        double sexValue; /* Sexagesimal value; just in case the type is not F32 */ \
        if (parseSexagesimal(&sexValue, compare->data.V)) { \
            valueC = sexValue; \
        } else { \
            return false; \
        } \
    } \
    ps##TYPE valueT = template->data.TYPE; \
    COMPARE_VALUES(valueT, valueC, true); \
}

#define TEMPLATE_CASE(TYPENAME, NAME, TYPE, COMPARESTRING, FORMAT) \
    case PS_TYPE_##TYPENAME: \
    switch(compare->type) { \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, B  , BOOL); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, U8 , U8 ); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, U16, U16); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, U32, U32); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, U64, U64); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, S8 , S8 ); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, S16, S16); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, S32, S32); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, F32, F32); \
        COMPARE_NUMERICAL_CASE(NAME, TYPE, F64, F64); \
      case PS_DATA_STRING: { \
          if (!template->data.V) { \
              return false; \
          } \
          COMPARESTRING(TYPENAME, FORMAT); \
      } \
      default: \
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Don't know how to compare types %x and %x\n", \
                compare->type, template->type); \
        return false; \
    }


bool psMetadataItemCompare(const psMetadataItem *compare, // Item to compare to the template
                           const psMetadataItem *template) // The template
{
    // First order checks:

    // both items must exist
    if (! compare || ! template) {
        return false;
    }

    // the names of both items must match
    if (strcmp(compare->name, template->name)) {
        return false;
    }

    switch (template->type) {
        TEMPLATE_CASE(BOOL, B,   bool,  COMPARE_BOOL_STRING, "");
        TEMPLATE_CASE(U8,   U8,  psU8,  COMPARE_INT_STRING,  SCNu8);
        TEMPLATE_CASE(U16,  U16, psU16, COMPARE_INT_STRING,  SCNu16);
        TEMPLATE_CASE(U32,  U32, psU32, COMPARE_INT_STRING,  SCNu32);
        TEMPLATE_CASE(U64,  U64, psU64, COMPARE_INT_STRING,  SCNu64);
        TEMPLATE_CASE(S8,   S8,  psS8,  COMPARE_INT_STRING,  SCNd8);
        TEMPLATE_CASE(S16,  S16, psS16, COMPARE_INT_STRING,  SCNd16);
        TEMPLATE_CASE(S32,  S32, psS32, COMPARE_INT_STRING,  SCNd32);
        TEMPLATE_CASE(S64,  S64, psS64, COMPARE_INT_STRING,  SCNd64);
        TEMPLATE_CASE(F32,  F32, psF32, COMPARE_FLOAT_STRING, "f");
        TEMPLATE_CASE(F64,  F64, psF64, COMPARE_FLOAT_STRING, "lf");
      case PS_DATA_STRING: {
          switch (compare->type) {
            case PS_DATA_STRING: {
                psTrace("psLib.types", 10, "Comparing '%s' with '%s'\n",
                        compare->data.str, template->data.str);
                if ((!compare->data.V && template->data.V) || (!template->data.V && compare->data.V)) {
                    return false;
                }
                return (strcasecmp(compare->data.V, template->data.V) == 0) ? true : false;
            }
            case PS_TYPE_BOOL: {
                bool templateValue;
                if (!parseBool(&templateValue, template->data.V)) {
                    return false;
                }
                return templateValue == compare->data.B;
            }
              COMPARE_STRING_INT_CASE(U8,  SCNu8);
              COMPARE_STRING_INT_CASE(U16, SCNu16);
              COMPARE_STRING_INT_CASE(U32, SCNu32);
              COMPARE_STRING_INT_CASE(U64, SCNu64);
              COMPARE_STRING_INT_CASE(S8,  SCNd8);
              COMPARE_STRING_INT_CASE(S16, SCNd16);
              COMPARE_STRING_INT_CASE(S32, SCNd32);
              COMPARE_STRING_INT_CASE(S64, SCNd64);
            case PS_TYPE_F32:
            case PS_TYPE_F64: {
                // Check for sexagesimal formating
                if (!template->data.V) {
                    return false;
                }
                double templateValue; // Value of template
                // Attempt to read the string first as a plain floating-point value, then as sexagesimal
                PARSE_STRING(status, templateValue, template->data.V, "lf");
                if (!status && !parseSexagesimal(&templateValue, template->data.V)) {
                    return false;
                }
                double compareValue = compare->type == PS_DATA_F32 ? compare->data.F32 : compare->data.F64;
                COMPARE_VALUES(templateValue, compareValue, true);
            }
            case PS_DATA_METADATA_MULTI: {
                // for MULTI, try each one & succeed if any match (valid = true is default state)
                for (int j = 0; j < compare->data.list->n; j++) {
                    psMetadataItem *entry = psListGet(compare->data.list, j);
                    if (!entry) {
                        continue;
                    }
                    if (entry->type != PS_DATA_STRING) {
                        continue;
                    }
                    if (template->data.V && !entry->data.V) { // expecting valid data, found NULL
                        continue;
                    }
                    if (!template->data.V && entry->data.V) { // expecting NULL, found valid data
                        continue;
                    }
                    // XXX should we return true for both NULL?
                    if (!template->data.V && !entry->data.V) { // expecting NULL, found NULL
                        return true;
                    }
                    if (!strcasecmp(entry->data.V, template->data.V)) {
                        return true;
                    }
                    // XXX this is a hack : also compare with the comment field (for HISTORY, COMMENT)
                    if (!strcasecmp(entry->comment, template->data.V)) {
                        return true;
                    }
                }
                return false;
            }
            default:
              // any other type is a mis-match against a string
              return false;
          }
      }
      default:
        // Simply don't know how to compare more complex types.
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Don't know how to compare types %x and %x\n",
                compare->type, template->type);
        return false;
    }

    psAbort("Should never get here.\n");
    return false;
}
