/* @file pmVisualUtils.h
 * @brief functions to create visual diagnostics with the help of 'kapa'
 * @author Chris Beaumont, IfA
 *
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_VISUAL_UTILS_H
#define PM_VISUAL_UTILS_H

typedef struct pmVisualComponent
{
    const char *name;			///< last part of name of component
    psS32 level;			///< visual level for this component
    bool specified;			///< whether the component is specified
    psS32 n;				///< number of subcomponents
    struct pmVisualComponent* *subcomp;	///< next level of subcomponents
}
pmVisualComponent;

psMetadata *pmVisualLevels(void);
void pmVisualPrintLevels(FILE *output);
int pmVisualSetLevel(const char *comp, int level);
void pmVisualCleanup();
void pmVisualReset(pmVisualComponent* currentNode);

int p_pmVisualGetLevel(const char *func, const char *name);
# define pmVisualGetLevel(facil) p_pmVisualGetLevel(__func__, facil)

bool p_pmVisualTestLevel(const char *func, const char *name, int level);
# define pmVisualTestLevel(facil, level) p_pmVisualTestLevel(__func__, facil, level)

#endif // PM_VISUAL_UTILS_H
