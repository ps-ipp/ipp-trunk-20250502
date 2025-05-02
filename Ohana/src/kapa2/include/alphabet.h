# include "../rotfont/times8.h"
# include "../rotfont/times12.h"
# include "../rotfont/times14.h"
# include "../rotfont/times18.h"
# include "../rotfont/times24.h"

# include "../rotfont/courier8.h"
# include "../rotfont/courier12.h"
# include "../rotfont/courier14.h"
# include "../rotfont/courier18.h"
# include "../rotfont/courier24.h"

# include "../rotfont/helvetica8.h"
# include "../rotfont/helvetica12.h"
# include "../rotfont/helvetica14.h"
# include "../rotfont/helvetica18.h"
# include "../rotfont/helvetica24.h"

# include "../rotfont/symbol8.h"
# include "../rotfont/symbol12.h"
# include "../rotfont/symbol14.h"
# include "../rotfont/symbol18.h"
# include "../rotfont/symbol24.h"

# define DEFFONT 1
static FontSet HardwiredFonts[] = {
  {times8font,  "times", 8},
  {times12font, "times", 12},
  {times14font, "times", 14},
  {times18font, "times", 18},
  {times24font, "times", 24},
  
  {courier8font,  "courier", 8},
  {courier12font, "courier", 12},
  {courier14font, "courier", 14},
  {courier18font, "courier", 18},
  {courier24font, "courier", 24},
  
  {helvetica8font,  "helvetica", 8},
  {helvetica12font, "helvetica", 12},
  {helvetica14font, "helvetica", 14},
  {helvetica18font, "helvetica", 18},
  {helvetica24font, "helvetica", 24},
  
  {symbol8font,  "symbol", 8},
  {symbol12font, "symbol", 12},
  {symbol14font, "symbol", 14},
  {symbol18font, "symbol", 18},
  {symbol24font, "symbol", 24}
};

/* put these as static in RotFont.c with accessor functions */
# define NROT 256
