/** @file psTrace.c
 *  \brief basic run-time trace facilities
 *  \ingroup LogTrace
 *
 *  This file will hold the code for procedures to insert
 *  trace messages into the code.
 *
 *  @author Robert Lupton, Princeton University
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.88 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-11-05 10:58:04 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

/*****************************************************************************
    NOTES:
 In the SRD, higher trace levels correspond to a numerically lower trace
 value in the code.  This is a bit confusing.  For example, a high-level
 message might be something like "Begin Processing".  The module programmer
 might give that a numerically low trace level, such as 1, so then any
 non-zero trace level in that code component will display thatmessage.

 We build a tree of trace components.  Every node in the tree has a
 depth, which is it's distance from the root.  However, this is not
 not the same thing as a node's "level", which corresponds to the
 trace level of that node.

I think the following is the correct behavior, but not sure:
    PS_UNKNOWN_TRACE_LEVEL: We never set the level of a component to this
    value.  This value is only used when psTraceGetLevel is called with
    a bad component name, or if the component root is undefined, I think.

    PS_DEFAULT_TRACE_LEVEL: This should only be used when adding the
    intermediate components of a psS64 name.  Ie. the "B" in .A.B.C

 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

// Even if psLib is being built without tracing (PS_NO_TRACE is set) we still
// want to build the symbols for programs linked against psLib
#undef PS_NO_TRACE

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "psAbort.h"
#include "psAssert.h"
#include "psTrace.h"
#include "psString.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psMetadata.h"
#include "psMemory.h"

#define MAX_HOSTNAME_LENGTH 256
#define MAX_HEADER_LENGTH 256
#define MAX_TRACE_LENGTH 1024
#define MAX_COMPONENT_LENGTH 1024


static p_psComponent* cRoot = NULL; // The root of the trace component
static bool traceTime = false;     // Flag to include time info
static bool traceHost = false;     // Flag to include host info
static bool traceLevel = false;    // Flag to include level info
static bool traceName = false;     // Flag to include name info
static bool traceMsg = true;      // Flag to include message info
static int traceFD = STDOUT_FILENO; // default value

static void componentFree(p_psComponent* comp);
static p_psComponent* componentAlloc(const char *name, int level);
static int getLevel(const char *facil);

/*****************************************************************************
componentAlloc(): allocate memory for a new node, and initialize members.
 *****************************************************************************/
static p_psComponent* componentAlloc(const char *name, int level)
{
    psAssert(name, "impossible");

    p_psComponent* comp = psAlloc(sizeof(p_psComponent));

    psMemSetPersistent(comp,true);
    psMemSetDeallocator(comp, (psFreeFunc) componentFree);
    comp->name = psStringCopy(name);
    psMemSetPersistent((psPtr)comp->name,true);
    comp->level = level;
    comp->n = 0;
    comp->p_psSpecified = false;
    comp->subcomp = NULL;
    return comp;
}

/*****************************************************************************
componentFree(): free the current node in the root tree, and all children
nodes as well.
 *****************************************************************************/
static void componentFree(p_psComponent* comp)
{
    if (comp == NULL) {
        return;
    }

    if (comp->subcomp != NULL) {
        for (psS32 i = 0; i < comp->n; i++) {
            psMemSetPersistent(comp->subcomp[i],false);
            psFree(comp->subcomp[i]);
        }
        psMemSetPersistent(comp->subcomp,false);
        psFree(comp->subcomp);
    }

    psMemSetPersistent((psPtr)comp->name,false);
    psFree(comp->name);
}

/*****************************************************************************
initTrace(): simply initialize the component root tree.
*****************************************************************************/
static void initTrace(void)
{
    if (cRoot == NULL) {
        cRoot = componentAlloc(".", PS_DEFAULT_TRACE_LEVEL);
    }
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


/*****************************************************************************
Set all trace levels to zero.

XXX: Currently, no function calls this routine.
 *****************************************************************************/
void p_psTraceReset(p_psComponent* currentNode)
{
    psAssert(currentNode, "impossible");

    psS32 i = 0;

    if (NULL == currentNode) {
        return;
    }

    currentNode->level = 0;
    for (i = 0; i < currentNode->n; i++) {
        if (NULL == currentNode->subcomp[i]) {
            psLogMsg("p_psTraceReset", PS_LOG_WARN,
                     _("Sub-component %d of node %s in trace tree is NULL."),
                     i, currentNode->name);
        } else {
            p_psTraceReset(currentNode->subcomp[i]);
        }
    }
    return;
}

/*****************************************************************************
Set all trace levels to zero.
 *****************************************************************************/
void psTraceReset()
{
    psFree(cRoot);
    cRoot = NULL;
}

/*****************************************************************************
componentAdd(): Adds the component named "addNodeName" to the root tree.
 *****************************************************************************/
static bool componentAdd(const char *addNodeName, psS32 level)
{
    psAssert(addNodeName, "impossible");

    psS32 i = 0;                        // Loop index variable.
    char name[MAX_COMPONENT_LENGTH]; // buffer for writeable copy.
    char *firstComponent = NULL;        // first component of name
    p_psComponent* currentNode = cRoot;
    psS32 nodeExists = 0;

    // XXX: Verify that this is the correct behavior.
    if (strcmp("", addNodeName) == 0) {
        psError(PS_ERR_BAD_PARAMETER_NULL,true,
                _("Failed to add null component to trace tree."));
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
                                             (currentNode->n + 1) * sizeof(p_psComponent* ));
            psMemSetPersistent(currentNode->subcomp,true);

            currentNode->n = (currentNode->n) + 1;

            if (pname == NULL) {
                // This is the final component to add.
                currentNode->subcomp[(currentNode->n) - 1] =
                    componentAlloc(firstComponent, level);
            } else {
                // We are adding an intermediate component.  The trace level
                // is not defined.  An undefined trace level inherits the
                // trace level of it's parent.  However, we do not set that
                // specifically here since that would inheritance to be a
                // static, one-time, type of behavior.

                currentNode->subcomp[(currentNode->n) - 1] =
                    componentAlloc(firstComponent, PS_DEFAULT_TRACE_LEVEL);
            }
            currentNode = currentNode->subcomp[(currentNode->n) - 1];
        }
    }

    return true;
}

/*****************************************************************************
    psSetTraceLevel(): add the component named "comp" to the component tree,
 if it is not already there, and set it's trace level to "level".

    NOTE: We modified this so that the user may omit the leading "," in a
    component name.  Since the code was already implemented assuming the "."
    was required, rather than change all that code, in this function, I
    simply add a leading "." to the component name if there is none.

    Input:
 comp
 level
    Output:
 none
    Returns:
 zero
*****************************************************************************/
int psTraceSetLevel(const char *comp,   // component of interest
                    int level)  // desired trace level
{
    PS_ASSERT_STRING_NON_EMPTY(comp, 0);

    char *compName = NULL;
    int prevLevel = -1;

    // If the root component tree does not exist, then initialize it.
    if (cRoot == NULL) {
        initTrace();
    }

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
                _("Failed to set trace level (%d) to '%s'."),
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

/*****************************************************************************
    doGetTraceLevel()
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
 The trace level of the "name" component.
 *****************************************************************************/
static psS32 doGetTraceLevel(const char *aname)
{
    psAssert(aname, "impossible");
    char name[strlen(aname) + 1];       // need a writeable copy: for strsep()
    char *pname = name;
    char *firstComponent = NULL;        // first component of name
    p_psComponent* currentNode = cRoot;
    psS32 i = 0;
    psS32 defaultLevel = 0;

    if (NULL == currentNode) {
        return (PS_UNKNOWN_TRACE_LEVEL);
    }

    if (strcmp(".", aname) == 0) {
        return (cRoot->level);
    }

    if (aname[0] != '.') {
        return (PS_UNKNOWN_TRACE_LEVEL);
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
                psLogMsg("p_psTraceReset", PS_LOG_WARN,
                         _("Sub-component %d of node %s in trace tree is NULL."),
                         i, currentNode->name);
            }

            if (strcmp(currentNode->subcomp[i]->name, firstComponent) == 0) {
                currentNode = currentNode->subcomp[i];
                // For level inheritance purpose, we save the level of this
                // component if it is not DEFAULT.
                if (currentNode->level != PS_DEFAULT_TRACE_LEVEL) {
                    defaultLevel = currentNode->level;
                }
                // Determine if this is the last component:
                if (pname == NULL) {
                    if (currentNode->level != PS_DEFAULT_TRACE_LEVEL) {
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
 Return a trace level of "name" in the root component tree.  If the
 exact string of components in "name" does not exist in the root
 tree, we return the deepest level of the match.
 *****************************************************************************/
static int getLevel(const char *name)
{
    if (cRoot == NULL) {
        return (PS_UNKNOWN_TRACE_LEVEL);
    }

    psS32 traceLevel;

    // If the component name has no leading dot, then supply it.
    if (name[0] != '.') {
        char compName[strlen(name) + 2];
        compName[0] = '.';
        strcpy(&compName[1], name);

        traceLevel = doGetTraceLevel(compName);
    } else {
        // Search the component root tree, determine the trace level.
        traceLevel = doGetTraceLevel(name);
    }

    // XXX: The default trace level is currently set at -1, which is not a
    // valid trace level.  This is convenient in determining whether or not
    // a component should inherit the trace level from parent nodes.  However,
    // it's not clear that -1 should ever be returned by this function.
    // The SDR is unclear on this point and we should probably request IfA
    // comment.
    if (traceLevel == PS_DEFAULT_TRACE_LEVEL) {
        traceLevel = PS_THE_OTHER_DEFAULT_TRACE_LEVEL;
    }

    return(traceLevel);
}

int p_psTraceGetLevel(const char *file,
                      int lineno,
                      const char *func,
                      const char *name)
{
    PS_ASSERT_STRING_NON_EMPTY(name, 0);
    PS_ASSERT_STRING_NON_EMPTY(func, 0);

    FACILITY(facility, func, name);
    return getLevel(facility);
}

/*****************************************************************************
    doPrintTraceLevels()
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
static void doPrintTraceLevels(const p_psComponent* comp,
                               psS32 depth,
                               psS32 defLevel)
{
    psAssert(comp, "impossible");

    char line[1024];
    psS32 i = 0;

    // XXX EAM : probably should just return if traceFD < 1
    if (traceFD < 1)
        return;

    if (comp->name[0] == '\0') {
        return;
    } else {
        if (comp->level == PS_DEFAULT_TRACE_LEVEL) {
	    sprintf(line,"%*s%-*s %d\n", depth, "", 20 - depth, comp->name, defLevel);
	    if (write (traceFD, line, strlen(line))) {;} // ignore return value
        } else {
	    sprintf(line, "%*s%-*s %d\n", depth, "", 20 - depth, comp->name, comp->level);
	    if (write (traceFD, line, strlen(line))) {;} // ignore return value
        }
    }

    for (i = 0; i < comp->n; i++) {
        if (comp->level == PS_DEFAULT_TRACE_LEVEL) {
            doPrintTraceLevels(comp->subcomp[i], depth + 1, defLevel);
        } else {
            doPrintTraceLevels(comp->subcomp[i], depth + 1, comp->level);
        }
    }
}


/*****************************************************************************
psPrintTraceLevels(): Simply print all the trace levels in the trace level
component tree.
Inputs:
 none
Outputs:
 none
Returns:
 null
*****************************************************************************/
void psTracePrintLevels(void)
{
    if (cRoot == NULL) {
        return;
    }

    doPrintTraceLevels(cRoot, 0, PS_THE_OTHER_DEFAULT_TRACE_LEVEL);
}

void psTraceV(const char *comp,
              int level,
              const char *format,
              va_list ap)
{
    PS_ASSERT_STRING_NON_EMPTY(comp, );
    PS_ASSERT_STRING_NON_EMPTY(format, );

    // XXX EAM : fd < 1 does not print messages
    if (traceFD < 1) {
        return;
    }

    static bool first = true;           // First time we've been called?
    static char hostname[MAX_HOSTNAME_LENGTH + 1]; // Host name
    if (first) {
        first = false;
        gethostname(hostname, MAX_HOSTNAME_LENGTH);
    }

    // Only display this message if it's trace level is less than the level
    // of it's associatedcomponent.
    if (level <= getLevel(comp)) {

        char clevel = 0;                    // letter-name for level
        switch (level) {
        case PS_LOG_ABORT:
            clevel = 'A';
            break;
        case PS_LOG_ERROR:
            clevel = 'E';
            break;
        case PS_LOG_WARN:
            clevel = 'W';
            break;
        case PS_LOG_INFO:
            clevel = 'I';
            break;
        default:
            if (level >= 4) {
                clevel = level + '0';
            } else {
                psTrace("psLib.sys", 2, "Invalid logMsg level: %d (%s)\n", level, format);
                level = (level < 0) ? 0 : 9;
                clevel = level + '0';
            }
        }

        int maxLength = MAX_HEADER_LENGTH; // Maximum length of header string
        char head[maxLength + 2];       // the added two are for the ending | and \0
        char *head_ptr = head;          // where we've got to in head

        // Create the various log fields...
        if (traceTime) {
            time_t clock = time(NULL);  // The current time.
            struct tm *utc = gmtime(&clock);    // The current gm time.
            maxLength -= snprintf(head_ptr, maxLength, "%4d:%02d:%02d %02d:%02d:%02dZ",
                                  utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday,
                                  utc->tm_hour, utc->tm_min, utc->tm_sec) - 1;
            head_ptr += strlen(head_ptr);
        }
        // Hostname should be 20 characters.
        if (traceHost) {
            if (head_ptr > head) {
                *head_ptr++ = '|';
            }
            maxLength -= snprintf(head_ptr, maxLength, "%-20s", hostname);
            head_ptr += strlen(head_ptr);
        }
        if (traceLevel) {
            if (head_ptr > head) {
                *head_ptr++ = '|';
            }
            maxLength -= snprintf(head_ptr, maxLength, "%c", clevel);
            head_ptr += strlen(head_ptr);
        }
        if (traceName) {
            if (head_ptr > head) {
                *head_ptr++ = '|';
            }
            maxLength -= snprintf(head_ptr, maxLength, "%s", comp);

            head_ptr += strlen(head_ptr);
        }

        if (head_ptr > head) {
            *head_ptr++ = '\n';
        } else if (!traceMsg) {                  // no output desired
            return;
        }
        *head_ptr = '\0';

        if(write(traceFD, head, strlen(head)) ) {;} // ignore return value

        if (traceMsg) {
            // We indent each message one space for each level of the message.
            for (int i = 0; i < level; i++) {
                if (write (traceFD, " ", 1)) {;} // ignore return value
            }
            psString line = NULL;       // Line to print
            psStringAppendV(&line, format, ap);
            if (write (traceFD, line, strlen(line))) {;} // ignore return value
            if (line[strlen(line) - 1] != '\n') {
                if (write(traceFD, "\n", 1)) {;} // ignore return value
            }
            psFree(line);
        } else {
            if (write(traceFD, "\n", 1)) {;} // ignore return value
        }

        // XXX: what is fd-appropriate equivalent of fflush?
    }
}

/*****************************************************************************
p_psTrace(): we display the trace message to standard output if the trace
level of that message, supplied by the parameter "level" is higher than the
trace level that is currently associated with the component named by the
parameter "comp".
Input:
 comp
 level
 ...  a printf-style output string.
Output:
 none
Return:
 null
 *****************************************************************************/
void p_psTrace(
    const char* file,                  ///< file name
    int lineno,                        ///< line number in file
    const char* func,                  ///< function name
    const char *facil,                 ///< facilty of interest
    psS32 level,                       ///< desired trace level
    const char *format,                ///< printf-style format command
    ...                                ///< trace message arguments
)
{
    PS_ASSERT_STRING_NON_EMPTY(file, );
    PS_ASSERT_STRING_NON_EMPTY(func, );
    PS_ASSERT_STRING_NON_EMPTY(facil, );
    PS_ASSERT_STRING_NON_EMPTY(format, );

    FACILITY(name, func, facil);

    va_list ap;
    va_start(ap, format);
    psTraceV(name, level, format, ap);
    va_end(ap);
}

bool psTraceSetDestination(int fd)
{
    if (fd < 0) {
        return false;
    }

    if (traceFD > STDERR_FILENO) {
        close(traceFD);
    }
    traceFD = fd;
    return true;
}

int psTraceGetDestination()
{
    return traceFD;
}

/*****************************************************************************
psTraceSetFormat(): Set the format of psTrace output.  More precisely,
    provide a string consisting of the letters {H (host), L (level), M
    (message), N (name), T (time)}.  The default is "HLMNT".  This string
    determines whether or not they associated type of information will be
    included in message traces.  It does not determine the order in which that
    information will appear (that order is fixed).

Input:
    fmt: a string specifying the format.
Return:
    NULL.
 *****************************************************************************/
bool psTraceSetFormat(const char *format)
{
    // assume none.
    traceHost = false;
    traceLevel = false;
    traceMsg = false;
    traceName = false;
    traceTime = false;

    // if fmt is NULL, no logging is desired.
    if (format == NULL) {
        return false;
    }

    // XXX: What is the purpose of this conditional.
    if (strlen(format) == 0) {
        format = "THLNM";
    }
    // Step through each character in the format string.  For each letter
    // in that string, set the appropriate logging.

    for (const char *ptr = format; *ptr != '\0'; ptr++) {
        switch (*ptr) {
        case 'H':
        case 'h':
            traceHost = true;
            break;
        case 'L':
        case 'l':
            traceLevel = true;
            break;
        case 'M':
        case 'm':
            traceMsg = true;
            break;
        case 'N':
        case 'n':
            traceName = true;
            break;
        case 'T':
        case 't':
            traceTime = true;
            break;
        default:
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Unknown logging keyword %c."), *ptr);
            return false;
        }
    }

    // XXX: If one must at least log error messages, why don't we set logMsg = true here?
    if (!traceMsg) {
        psTrace("psLib.sys", 1,
                "You must at least trace error messages (You chose \"%s\")", format);

    }
    return true;
}


static void doGetTraceLevels(psMetadata *out, // Output metadata with the trace levels
                             const p_psComponent* comp, // Component to add
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

    int level = (comp->level == PS_DEFAULT_TRACE_LEVEL) ? defLevel : comp->level; // Level for component
    if (name) {
        psMetadataAddS32(out, PS_LIST_TAIL, name, 0, NULL, level);
    }
    for (int i = 0; i < comp->n; i++) {
        doGetTraceLevels(out, comp->subcomp[i], name, level);
    }

    psFree(name);

    return;
}


psMetadata *psTraceLevels(void)
{
    if (cRoot == NULL) {
        return psMetadataAlloc();
    }

    psMetadata *out = psMetadataAlloc();// Output metadata with the levels
    doGetTraceLevels(out, cRoot, NULL, PS_THE_OTHER_DEFAULT_TRACE_LEVEL);

    return out;
}
