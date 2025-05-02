# include "data.h"
# include "basic.h"
# include "astro.h"

# ifndef MANA_H

void InitMana (void);
void FreeMana (void);
int *findrowpeaks (float *row, int Nrow, float threshold, int *npeaks);

# endif
