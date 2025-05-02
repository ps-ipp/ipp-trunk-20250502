# include "fakeastro.h"

int InitStar (Stars *star) {


    dvo_measure_init (&star[0].measure);
    dvo_average_init (&star[0].average);
    star[0].found = -1; // found == -1 -> not yet found (use enums?)
    return TRUE;
}
