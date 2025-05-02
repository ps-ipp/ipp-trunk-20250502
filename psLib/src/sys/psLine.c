#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>

#include "psAssert.h"
#include "psConstants.h"
#include "psLine.h"
#include "psString.h"
#include "psMemory.h"

static void lineFree(psLine *line)
{

    if (!line) {
        return;
    }

    psFree (line->line);
    return;
}

// allocate a psLine structrue
psLine *psLineAlloc(long Nline)
{
    psLine *line = psAlloc(sizeof(psLine));
    psMemSetDeallocator(line, (psFreeFunc)lineFree);
    line->Nline = 0;
    line->NLINE = Nline;
    //    line->line = psAlloc(Nline);
    line->line = psStringAlloc(Nline);

    return line;
}

bool psLineInit(psLine *line)
{
    if (!line) {
        return false;
    }

    line->Nline = 0;
    return true;
}

bool psLineAdd(psLine *line,
               const char *format,
               ...)
{
    if (!line) {
        return false;
    }

    long nMax = line->NLINE - line->Nline;

    va_list ap;
    va_start(ap, format);
    long Nchar = vsnprintf(&line->line[line->Nline], nMax, format, ap);
    line->Nline += PS_MIN(nMax - 1, Nchar);
    va_end(ap);

    if (Nchar >= nMax) {
        return false;
    }
    return true;
}

bool psMemCheckLine(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)lineFree );
}

