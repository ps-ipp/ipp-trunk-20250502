# include "ppStatsInternal.h"

// calculate the stats for the non-constant entries (already calculated)
bool ppStatsFromMetadataPrint(psArray *entries, char *filename)
{
    bool status = true;                 // Status of printing
    FILE *f = NULL;
    if (!strcmp (filename, "-")) {
        f = stdout;
    } else {
        f = fopen (filename, "w");
        if (f == NULL) {
            psError(PS_ERR_UNKNOWN, false, "ppStatsFromMetadata cannot open output file %s\n", filename);
            return false;
        }
    }

    // at this point, we have entries with values (of type STR or F32) or NULL
    for (int i = 0; i < entries->n; i++) {
        ppStatsEntry *entry = entries->data[i];

        if (!entry->value) continue;

// Print a value
#define VALUE_NUMERICAL_CASE(TYPE, FORMAT, NAME) \
      case PS_TYPE_##TYPE: \
        fprintf(f, "%s %" FORMAT " ", entry->flag, entry->value->data.NAME); \
        break; \


        switch (entry->value->type) {
            VALUE_NUMERICAL_CASE(U8,  PRIu8,  U8);
            VALUE_NUMERICAL_CASE(U16, PRIu16, U16);
            VALUE_NUMERICAL_CASE(U32, PRIu32, U32);
            VALUE_NUMERICAL_CASE(U64, PRIu64, U64);
            VALUE_NUMERICAL_CASE(S8,  PRId8,  S8);
            VALUE_NUMERICAL_CASE(S16, PRId16, S16);
            VALUE_NUMERICAL_CASE(S32, PRId32, S32);
            VALUE_NUMERICAL_CASE(S64, PRId64, S64);
            VALUE_NUMERICAL_CASE(F32, "f",    F32);
            VALUE_NUMERICAL_CASE(F64, "lf",   F64);
          case PS_DATA_STRING:
            if (entry->value->data.str) {
                fprintf(f, "%s '%s' ", entry->flag, entry->value->data.str);
            }
            break;
          case PS_DATA_BOOL:
            if (entry->value->data.B) {
                fprintf(f, "%s ", entry->flag);
            }
            break;
          case PS_DATA_TIME: {
              psTime *t = (psTime*)entry->value->data.V;
              if (t) {
                  psString str = psTimeToISO(t);
                  fprintf(f, "%s %.19sZ ", entry->flag, str);
                  psFree(str);
              }
              break;
          }
          default:
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unsupported type: %x", entry->value->type);
            status = false;
        }
    }
    fprintf(f, "\n");

    if (f != stdout) fclose (f);
    return status;
}
