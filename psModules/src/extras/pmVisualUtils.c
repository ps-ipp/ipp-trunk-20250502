/** These utility functions manage the visual level information (equivalent to pmVisual levels)
 *  @author Eugene Magnier, IfA
 *  @date June 18, 2010
 */

/* Include Files  */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pslib.h>

#include <pmVisual.h>
#include <pmVisualUtils.h>

#define PM_UNKNOWN_VISUAL_LEVEL -9999   // we don't know this name's level
#define PM_DEFAULT_VISUAL_LEVEL -1
#define PM_THE_OTHER_DEFAULT_VISUAL_LEVEL 0 // ???
#define MAX_COMPONENT_LENGTH 1024

/** Basic structure for the component tree.  A component is a string of the
    form aaa.bbb.ccc, and may itself contain further subcomponents.  The
    Component structure doesn't in fact contain it's full name, but only the
    last part. */

static pmVisualComponent* cRoot = NULL; // The root of the visual component tree

/** static function prototypes **/
static void componentFree(pmVisualComponent* comp);
static pmVisualComponent* componentAlloc(const char *name, int level);
static int getLevel(const char *facil);
static void initVisualLevels(void);
static void componentFree(pmVisualComponent* comp);
static pmVisualComponent* componentAlloc(const char *name, int level);
static void doGetVisualLevels(psMetadata *out, const pmVisualComponent* comp, psString parent, int defLevel);
static void doPrintVisualLevels(const pmVisualComponent* comp, FILE *output, psS32 depth, psS32 defLevel);
static psS32 doGetVisualLevel(const char *aname);
static bool componentAdd(const char *addNodeName, psS32 level);

/*****************************************************************************
Set all visual levels at or below the specified node to zero.
 *****************************************************************************/
void pmVisualReset(pmVisualComponent* currentNode)
{
    psAssert(currentNode, "impossible");

    psS32 i = 0;

    if (NULL == currentNode) {
        return;
    }

    currentNode->level = 0;
    for (i = 0; i < currentNode->n; i++) {
        if (!currentNode->subcomp[i]) {
            psLogMsg("pmVisualReset", PS_LOG_WARN, _("Sub-component %d of node %s in visual tree is NULL."), i, currentNode->name);
        } else {
            pmVisualReset(currentNode->subcomp[i]);
        }
    }
    return;
}

/*****************************************************************************
Free all visual levels
 *****************************************************************************/
void pmVisualCleanup()
{
    psFree(cRoot);
    cRoot = NULL;
}

// psSetVisualLevel(): add the component named "comp" to the component tree,
int pmVisualSetLevel(const char *comp,   // component of interest
                    int level)  // desired visual level
{
    PS_ASSERT_STRING_NON_EMPTY(comp, 0);

    char *compName = NULL;
    int prevLevel = -1;

    // If the root component tree does not exist, then initialize it.
    if (cRoot == NULL) {
        initVisualLevels();
    }

    // turn on overall visualization
    pmVisualSetVisual(true);

    // If the component name has no leading dot, then supply it.
    if (comp[0] != '.') {
        compName = (char *) psAlloc(10 + strlen(comp));
        strcpy(compName, ".");
        compName = strcat(compName, comp);
    } else {
        compName = (char *) comp;
    }
    prevLevel = getLevel(compName);
    // Add the new component to the component tree.
    if ( !componentAdd(compName, level) ) {
        psError(PS_ERR_UNKNOWN, false,
                _("Failed to set visual level (%d) to '%s'."),
                level,
                compName);

        if (comp[0] != '.') {
            psFree(compName);
        }
        //        return false;
        return -1;
    }

    if (comp[0] != '.') {
        psFree(compName);
    }

    //    return true;
    return prevLevel;
}

// Append the function name to the facility
// NB: declares TARGET!
#define FACILITY(TARGET, FUNC, FACIL) \
    size_t _facilLength = strlen(FACIL); /* Length of facility name */ \
    size_t _funcLength = strlen(FUNC);   /* Length of function name */ \
    char TARGET[_facilLength + _funcLength + 2]; /* facility + the function name */ \
    strcpy(&TARGET[0], FACIL); \
    TARGET[_facilLength] = '.'; \
    strcpy(&TARGET[_facilLength + 1], FUNC);

int p_pmVisualGetLevel(const char *func, const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, 0);
    PS_ASSERT_STRING_NON_EMPTY(func, 0);

    FACILITY(facility, func, name);
    return getLevel(facility);
}

/** the main tool in the pmVisual system : check that the specified level matches the desired level **/
bool p_pmVisualTestLevel(const char *func, const char *name, int level)
{
    PS_ASSERT_STRING_NON_EMPTY(name, false);
    PS_ASSERT_STRING_NON_EMPTY(func, false);

    // if visualization is turned off, just skip
    if (!pmVisualIsVisual()) return false;

    // return true if level is set to be shown
    FACILITY(facility, func, name);

    bool valid = level <= getLevel(facility);

    if (valid) {
	psLogMsg ("psModules.visual", PS_LOG_DETAIL, "visualization %s (%d)\n", facility, level);
    }

    return (valid);
}

// psPrintVisualLevels(): Simply print all the visual levels in the visual level
void pmVisualPrintLevels(FILE *output)
{
    if (cRoot == NULL) {
        return;
    }

    doPrintVisualLevels(cRoot, output, 0, PM_THE_OTHER_DEFAULT_VISUAL_LEVEL);
}

// generate a metadata of the visual levels (is this really needed?)
psMetadata *pmVisualLevels(void)
{
    if (cRoot == NULL) {
        return psMetadataAlloc();
    }

    psMetadata *out = psMetadataAlloc();// Output metadata with the levels
    doGetVisualLevels(out, cRoot, NULL, PM_THE_OTHER_DEFAULT_VISUAL_LEVEL);

    return out;
}

/****************************** static functions ******************************/

/*****************************************************************************
componentAdd(): Adds the component named "addNodeName" to the root tree.
 *****************************************************************************/
static bool componentAdd(const char *addNodeName, psS32 level)
{
    psAssert(addNodeName, "impossible");

    psS32 i = 0;                        // Loop index variable.
    char name[MAX_COMPONENT_LENGTH]; // buffer for writeable copy.
    char *firstComponent = NULL;        // first component of name
    pmVisualComponent* currentNode = cRoot;
    psS32 nodeExists = 0;

    // XXX: Verify that this is the correct behavior.
    if (strcmp("", addNodeName) == 0) {
        psError(PS_ERR_BAD_PARAMETER_NULL,true,
                _("Failed to add null component to visual tree."));
        return false;
    }

    // Is this the root node? If so, simply set level and return.
    if (strcmp(".", addNodeName) == 0) {
        cRoot->level = level;
        return true;
    }

    if (addNodeName[0] != '.') {
        psError(PS_ERR_BAD_PARAMETER_VALUE,true,
                _("Failed to add '%s' to the root component tree; component must start with '.'."),
                addNodeName);
        return false;
    }

    ps_strncpy_nowarn(name, addNodeName, MAX_COMPONENT_LENGTH);
    char *pname = name+1;               // Take off the period
    // Iterate through the components of addNodeName.  Strip off the first
    // component of the name, find that in the root tree, or add it if it
    // does not exist, then move to the next component in the name.

    while (pname != NULL) {
        firstComponent = pname;
        pname = strchr(firstComponent, '.');
        if (pname != NULL) {
            *pname = '\0';
            pname++;
        }
        nodeExists = 0;
        for (i = 0; i < currentNode->n; i++) {
            if (strcmp(currentNode->subcomp[i]->name, firstComponent) == 0) {
                currentNode = currentNode->subcomp[i];
                nodeExists = 1;
                if (pname == NULL) {
                    currentNode->level = level;
                }
            }
        }

        if (nodeExists == 0) {
            currentNode->subcomp = psRealloc(currentNode->subcomp,
                                             (currentNode->n + 1) * sizeof(pmVisualComponent* ));
            psMemSetPersistent(currentNode->subcomp,true);

            currentNode->n = (currentNode->n) + 1;

            if (pname == NULL) {
                // This is the final component to add.
                currentNode->subcomp[(currentNode->n) - 1] =
                    componentAlloc(firstComponent, level);
            } else {
                // We are adding an intermediate component.  The visual level is not defined.
                // An undefined visual level inherits the visual level of it's parent.  However,
                // we do not set that specifically here since that would result in a static,
                // one-time, type of behavior.

                currentNode->subcomp[(currentNode->n) - 1] =
                    componentAlloc(firstComponent, PM_DEFAULT_VISUAL_LEVEL);
            }
            currentNode = currentNode->subcomp[(currentNode->n) - 1];
        }
    }

    return true;
}

/*****************************************************************************
    doGetVisualLevel()
 This function recursively searches the root component tree for the
 component named "name", which is supplied by a parameter.  If it
 finds that component, it returns the level of that component.
 Otherwise, it returns ???.

    NOTE: We modified this so that the user may omit the leading "," in a
    component name.  Since the code was already implemented assuming the "."
    was required, rather than change all that code, in this function, I
    simply add a leading "." to the component name if there is none.

    Inputs:
 name:
    Outputs:
 none
    Returns:
 The visual level of the "name" component.
 *****************************************************************************/
static psS32 doGetVisualLevel(const char *aname)
{
    psAssert(aname, "impossible");
    char name[strlen(aname) + 1];       // need a writeable copy: for strsep()
    char *pname = name;
    char *firstComponent = NULL;        // first component of name
    pmVisualComponent* currentNode = cRoot;
    psS32 i = 0;
    psS32 defaultLevel = 0;

    if (NULL == currentNode) {
        return (PM_UNKNOWN_VISUAL_LEVEL);
    }

    if (strcmp(".", aname) == 0) {
        return (cRoot->level);
    }

    if (aname[0] != '.') {
        return (PM_UNKNOWN_VISUAL_LEVEL);
    }

    defaultLevel = cRoot->level;
    strcpy(name, aname);
    pname = name+1;
    while (pname != NULL) {
        firstComponent = pname;
        pname = strchr(firstComponent, '.');
        if (pname != NULL) {
            *pname = '\0';
            pname++;
        }
        for (i = 0; i < currentNode->n; i++) {
            if (NULL == currentNode->subcomp[i]) {
                psLogMsg("p_pmVisualReset", PS_LOG_WARN,
                         _("Sub-component %d of node %s in visual tree is NULL."),
                         i, currentNode->name);
            }

            if (strcmp(currentNode->subcomp[i]->name, firstComponent) == 0) {
                currentNode = currentNode->subcomp[i];
                // For level inheritance purpose, we save the level of this
                // component if it is not DEFAULT.
                if (currentNode->level != PM_DEFAULT_VISUAL_LEVEL) {
                    defaultLevel = currentNode->level;
                }
                // Determine if this is the last component:
                if (pname == NULL) {
                    if (currentNode->level != PM_DEFAULT_VISUAL_LEVEL) {
                        return (currentNode->level);
                    } else {
                        return(defaultLevel);
                    }
                }
            }
        }
    }
    return(defaultLevel);
}

/*****************************************************************************
    getLevel()
 Return a visual level of "name" in the root component tree.  If the
 exact string of components in "name" does not exist in the root
 tree, we return the deepest level of the match.
 *****************************************************************************/
static int getLevel(const char *name)
{
    if (cRoot == NULL) {
        return (PM_UNKNOWN_VISUAL_LEVEL);
    }

    psS32 visualLevel;

    // If the component name has no leading dot, then supply it.
    if (name[0] != '.') {
        char compName[strlen(name) + 2];
        compName[0] = '.';
        strcpy(&compName[1], name);

        visualLevel = doGetVisualLevel(compName);
    } else {
        // Search the component root tree, determine the visual level.
        visualLevel = doGetVisualLevel(name);
    }

    // XXX: The default visual level is currently set at -1, which is not a
    // valid visual level.  This is convenient in determining whether or not
    // a component should inherit the visual level from parent nodes.  However,
    // it's not clear that -1 should ever be returned by this function.
    // The SDR is unclear on this point and we should probably request IfA
    // comment.
    if (visualLevel == PM_DEFAULT_VISUAL_LEVEL) {
        visualLevel = PM_THE_OTHER_DEFAULT_VISUAL_LEVEL;
    }

    return(visualLevel);
}

/*****************************************************************************
    doPrintVisualLevels()
 This function recursively searches the component tree supplied by the
 parameter "comp" and prints the name and level of each component.
    Inputs:
 comp: a node in the component tree.
 level: the level of that node
    Outputs:
 none
    Returns:
 null
 *****************************************************************************/
static void doPrintVisualLevels(const pmVisualComponent* comp,
				FILE *output,
				psS32 depth,
				psS32 defLevel)
{
    psAssert(comp, "impossible");

    char line[1024];
    psS32 i = 0;

    if (comp->name[0] == '\0') {
        return;
    } else {
        int nwritten = 0;
        if (comp->level == PM_DEFAULT_VISUAL_LEVEL) {
	    sprintf(line,"%*s%-*s %d\n", depth, "", 20 - depth, comp->name, defLevel);
	    nwritten = fwrite (line, 1, strlen(line), output);
        } else {
	    sprintf(line, "%*s%-*s %d\n", depth, "", 20 - depth, comp->name, comp->level);
	    nwritten = fwrite (line, 1, strlen(line), output);
        }
	if (nwritten < 1) {
	}
    }

    for (i = 0; i < comp->n; i++) {
        if (comp->level == PM_DEFAULT_VISUAL_LEVEL) {
            doPrintVisualLevels(comp->subcomp[i], output, depth + 1, defLevel);
        } else {
            doPrintVisualLevels(comp->subcomp[i], output, depth + 1, comp->level);
        }
    }
}

static void doGetVisualLevels(psMetadata *out, // Output metadata with the visual levels
                             const pmVisualComponent* comp, // Component to add
                             psString parent, // Name of parent level
                             int defLevel // Default level
                            )
{
    if (comp->name[0] == '\0') {
        return;
    }

    psString name = psStringCopy(parent); // Name of this level
    if (comp->name[0] == '.') {
        psStringAppend(&name, "%s", comp->name + 1);
    } else if (!parent) {
        psStringAppend(&name, "%s", comp->name);
    } else {
        psStringAppend(&name, ".%s", comp->name);
    }

    int level = (comp->level == PM_DEFAULT_VISUAL_LEVEL) ? defLevel : comp->level; // Level for component
    if (name) {
        psMetadataAddS32(out, PS_LIST_TAIL, name, 0, NULL, level);
    }
    for (int i = 0; i < comp->n; i++) {
        doGetVisualLevels(out, comp->subcomp[i], name, level);
    }

    psFree(name);

    return;
}

/*****************************************************************************
componentAlloc(): allocate memory for a new node, and initialize members.
 *****************************************************************************/
static pmVisualComponent* componentAlloc(const char *name, int level)
{
    psAssert(name, "impossible");

    pmVisualComponent* comp = psAlloc(sizeof(pmVisualComponent));
    psMemSetDeallocator(comp, (psFreeFunc) componentFree);

    comp->name = psStringCopy(name);
    comp->level = level;
    comp->n = 0;
    comp->specified = false;
    comp->subcomp = NULL;
    return comp;
}

/*****************************************************************************
componentFree(): free the current node in the root tree, and all children
nodes as well.
 *****************************************************************************/
static void componentFree(pmVisualComponent* comp)
{
    if (comp == NULL) {
        return;
    }

    if (comp->subcomp != NULL) {
        for (psS32 i = 0; i < comp->n; i++) {
            psFree(comp->subcomp[i]);
        }
        psFree(comp->subcomp);
    }
    psFree(comp->name);
}

/*****************************************************************************
initVisualLevels(): simply initialize the component root tree.
*****************************************************************************/
static void initVisualLevels(void)
{
    if (cRoot == NULL) {
        cRoot = componentAlloc(".", PM_DEFAULT_VISUAL_LEVEL);
    }
}
