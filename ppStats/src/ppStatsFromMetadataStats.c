# include "ppStatsInternal.h"

static void computeBitwiseOr(ppStatsEntry *entry);

// calculate the stats for the non-constant entries (already calculated)
bool ppStatsFromMetadataStats(psArray *entries)
{
    for (int i = 0; i < entries->n; i++) {
        ppStatsEntry *entry = entries->data[i];

        if (!strcasecmp (entry->statistic, "constant")) continue;

        // XXX skip or warn on missing stats?
        if (!entry->vector) continue;

        if (!strcasecmp (entry->statistic, "bitwiseor")) {
            computeBitwiseOr(entry);
            continue;
        }

        psStatsOptions option;
        if (!strcasecmp (entry->statistic, "RMS")) {
            option = psStatsOptionFromString ("SAMPLE_MEAN");
            goto got_stats;
        }
        if (!strcasecmp (entry->statistic, "SUM")) {
            option = psStatsOptionFromString ("SAMPLE_MEAN");
            goto got_stats;
        }
        if (!strcasecmp (entry->statistic, "UQ")) {
            option = psStatsOptionFromString ("ROBUST_QUARTILE");
            goto got_stats;
        }
        if (!strcasecmp (entry->statistic, "LQ")) {
            option = psStatsOptionFromString ("ROBUST_QUARTILE");
            goto got_stats;
        }

        option = psStatsOptionFromString (entry->statistic);

    got_stats:
        if (entry->vector->n == 0) {
            continue;
        }

        psStats *stats = psStatsAlloc(option);
        if (!psVectorStats(stats, entry->vector, NULL, NULL, 0)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats for %s", entry->statistic);
	    continue;
	}

        double value;
        if (!strcasecmp (entry->statistic, "RMS")) {
            value = sqrt(stats->sampleMean);
            goto got_value;
        }
        if (!strcasecmp (entry->statistic, "SUM")) {
            value = stats->sampleMean * entry->vector->n;
            goto got_value;
        }
        if (!strcasecmp (entry->statistic, "UQ")) {
            value = stats->robustUQ;
            goto got_value;
        }
        if (!strcasecmp (entry->statistic, "LQ")) {
            value = stats->robustLQ;
            goto got_value;
        }
        value = psStatsGetValue(stats, option);

    got_value:
        entry->value = psMetadataItemAllocF32(entry->keyword, entry->statistic, value);
        psFree(stats);
    }
    return true;
}

static void computeBitwiseOr(ppStatsEntry *entry)
{
    psU64 result = 0;
    psVector *vector = entry->vector;
    for (int j = 0; j < vector->n; j++) {
        // XXX: should we handle other types
        if (entry->type == PS_DATA_U64) {
            result |= vector->data.U64[j];
        } else if (entry->type == PS_DATA_U32) {
            result |= vector->data.U32[j];
        } else {
            return;
        } 
    }

    if (entry->type == PS_DATA_U64) {
        entry->value = psMetadataItemAllocU64(entry->keyword, entry->statistic, result);
    } else {
        entry->value = psMetadataItemAllocU32(entry->keyword, entry->statistic, result);
    }

    return;
}

