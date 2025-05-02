# include "ppStatsInternal.h"

// loop over the metadata, adding entries of interest to their data arrays
bool ppStatsFromMetadataParse (psMetadata *input, psArray *entries) {

    psMetadataItem *item = NULL;

    // loop over the items, selecting those with the name "ENTRY"
    psMetadataIterator *iter = psMetadataIteratorAlloc(input, PS_LIST_HEAD, NULL); // Iterator

    while ((item = psMetadataGetAndIncrement(iter))) {
	if (item->type == PS_DATA_METADATA) {
	    psMetadata *folder = item->data.md;
	    if (!ppStatsFromMetadataParse (folder, entries)) {
		psError(PS_ERR_UNKNOWN, false, "error parse metadata folder %s\n", item->name);
		return false;
	    }
	    continue;
	}

	// find the matching entry, if one exists (if not, this is not an error: we are not
	// obliged to determine stats for all entries).  we may have more than one entry for a
	// given keyword (multiple stats for a single input item are allowed).
	for (int i = 0; i < entries->n; i++) {
	    ppStatsEntry *entry = entries->data[i];
	    if (strcmp (entry->keyword, item->name)) continue;

	    // check if the types match?
	    if (entry->type != item->type) {
		fprintf (stderr, "WARNING?  mismatched type, skipping\n");
		continue;
	    }

	    // save the value

	    // if constant, save or compare with existing value
	    if (!strcasecmp (entry->statistic, "constant")) {
		if (entry->value) {
		    // check that they match
		} else {
		    if ((item->type == PS_DATA_STRING) && (item->data.str == NULL)) {
			continue;
		    }
		    entry->value = psMemIncrRefCounter (item);
		}
		continue;
	    }

	    bool useRMS = false;
	    if (!strcasecmp (entry->statistic, "rms")) {
		useRMS = true;
	    }

	    // only numerical values can have stats; all others must be 'constant'
	    if (entry->type >= PS_DATA_BOOL) {
		psError(PS_ERR_UNKNOWN, false, "only numerical types can have non-constant stats: %s\n", entry->keyword);
		return false;
	    }

	    if (!entry->vector) {
		entry->vector = psVectorAllocEmpty (16, entry->type);
	    }

	    switch (item->type) {
	      case PS_DATA_U8:
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.U8));
		} else {
		    psVectorAppend (entry->vector, item->data.U8);
		}
		break;
	      case PS_DATA_U16:
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.U16));
		} else {
		    psVectorAppend (entry->vector, item->data.U16);
		}
		break;
	      case PS_DATA_U32:
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.U32));
		} else {
		    psVectorAppend (entry->vector, item->data.U32);
		}
		break;
	      case PS_DATA_U64:
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.U64));
		} else {
		    psVectorAppend (entry->vector, item->data.U64);
		}
		break;

	      case PS_DATA_S8:
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.S8));
		} else {
		    psVectorAppend (entry->vector, item->data.S8);
		}
		break;
	      case PS_DATA_S16:
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.S16));
		} else {
		    psVectorAppend (entry->vector, item->data.S16);
		}
		break;
	      case PS_DATA_S32:
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.S32));
		} else {
		    psVectorAppend (entry->vector, item->data.S32);
		}
		break;
	      case PS_DATA_S64:
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.S64));
		} else {
		    psVectorAppend (entry->vector, item->data.S64);
		}
		break;

	      case PS_DATA_F32:
		if (!isfinite(item->data.F32)) continue;
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.F32));
		} else {
		    psVectorAppend (entry->vector, item->data.F32);
		}
		break;
	      case PS_DATA_F64:
		if (!isfinite(item->data.F64)) continue;
		if (useRMS) {
		    psVectorAppend (entry->vector, PS_SQR(item->data.F64));
		} else {
		    psVectorAppend (entry->vector, item->data.F64);
		}
		break;
	      default:
		psError(PS_ERR_UNKNOWN, true, "ppStatsFromMetadata recipe ENTRY %s has invalid type\n", entry->keyword);
		return false;
	    }
	}
    }
    return true;
}
