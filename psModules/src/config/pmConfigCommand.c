#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "pmConfig.h"
#include "pmConfigCommand.h"

bool pmConfigDatabaseCommand(psString *command, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(command, false);
    PS_ASSERT_PTR_NON_NULL(config, false);

    // Connection details
    psMetadataItem *server = pmConfigUserSite(config, "DBSERVER",   PS_DATA_STRING);
    psMetadataItem *user   = pmConfigUserSite(config, "DBUSER",     PS_DATA_STRING);
    psMetadataItem *pass   = pmConfigUserSite(config, "DBPASSWORD", PS_DATA_STRING);
    psMetadataItem *name   = pmConfigUserSite(config, "DBNAME",     PS_DATA_STRING);

    if (!server || !user || !pass || !name) {
        psWarning("Cannot find DBSERVER/DBUSER/DBPASSWORD/DBNAME in user or site configuration: "
                  "unable to connect to database.");
        psErrorClear();
        return NULL;
    }

    psStringAppend(command, " -dbserver %s -dbname %s -dbuser %s -dbpassword %s",
                   server->data.str, name->data.str, user->data.str, pass->data.str);

    return true;
}


bool pmConfigTraceCommand(psString *command)
{
    PS_ASSERT_PTR_NON_NULL(command, false);

    psMetadata *levels = psTraceLevels(); // Metadata levels
    psMetadataIterator *iter = psMetadataIteratorAlloc(levels, PS_LIST_HEAD, NULL); // Iterator for levels
    psMetadataItem *item;               // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        assert(item->type == PS_DATA_S32);
        psStringAppend(command, " -trace %s %d", item->name, item->data.S32);
    }
    psFree(iter);
    psFree(levels);

    return true;
}
