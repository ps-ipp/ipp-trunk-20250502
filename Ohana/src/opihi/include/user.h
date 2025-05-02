int abszero         PROTO((int, char **));
int applyfit        PROTO((int, char **));
int applyfit2d      PROTO((int, char **));
int badimages       PROTO((int, char **));
int box             PROTO((int, char **));
int calextract      PROTO((int, char **));
int calmextract     PROTO((int, char **));
int cals            PROTO((int, char **));
int catlog          PROTO((int, char **));
int ccd             PROTO((int, char **));
int ccdextract      PROTO((int, char **));
int center          PROTO((int, char **));
int cgrid           PROTO((int, char **));
int clear           PROTO((int, char **));
int cmatch          PROTO((int, char **));
int cmd             PROTO((int, char **));
int cmdextract      PROTO((int, char **));
int cmpread         PROTO((int, char **));
int concat          PROTO((int, char **));
int contour         PROTO((int, char **));
int cplot           PROTO((int, char **));
int create          PROTO((int, char **));
int csystem         PROTO((int, char **));
int ctimes          PROTO((int, char **));
int cursor          PROTO((int, char **));
int czplot          PROTO((int, char **));
int datafile        PROTO((int, char **));
int date            PROTO((int, char **));
int ddmagextract    PROTO((int, char **));
int ddmags          PROTO((int, char **));
int delete          PROTO((int, char **));
int device          PROTO((int, char **));
int dmagaves        PROTO((int, char **));
int dmagextract     PROTO((int, char **));
int dmagmeas        PROTO((int, char **));
int dmags           PROTO((int, char **));
int dmt             PROTO((int, char **));
int dumpmags        PROTO((int, char **));
int elixir          PROTO((int, char **));
int extract         PROTO((int, char **));
int file            PROTO((int, char **));
int fit             PROTO((int, char **));
int fit2d           PROTO((int, char **));
int gaussjordan     PROTO((int, char **));
int gcat            PROTO((int, char **));
int gimages         PROTO((int, char **));
int grid            PROTO((int, char **));
int gstar           PROTO((int, char **));
int gtypes          PROTO((int, char **));
int histogram       PROTO((int, char **));
int images          PROTO((int, char **));
int imbox           PROTO((int, char **));
int imdata          PROTO((int, char **));
int imdense         PROTO((int, char **)); 
int imextract       PROTO((int, char **));
int imlist          PROTO((int, char **));
int imphot          PROTO((int, char **));
int imrough         PROTO((int, char **));
int imsearch        PROTO((int, char **));
int imstats         PROTO((int, char **));
int interpolate     PROTO((int, char **));
int jpeg            PROTO((int, char **));
int labels          PROTO((int, char **));
int lcat            PROTO((int, char **));
int lcurve          PROTO((int, char **));
int limits          PROTO((int, char **));
int list_buffers    PROTO((int, char **));
int list_vectors    PROTO((int, char **));
int mcreate         PROTO((int, char **));
int mextract        PROTO((int, char **));
int mget            PROTO((int, char **));
int mset            PROTO((int, char **));
int pcat            PROTO((int, char **));
int photcodes       PROTO((int, char **));
int photresid       PROTO((int, char **));
int plot            PROTO((int, char **));
int pmeasure        PROTO((int, char **));
int precess         PROTO((int, char **));
int print           PROTO((int, char **));
int procks          PROTO((int, char **));
int ps              PROTO((int, char **));
int rd              PROTO((int, char **));
int read_vectors    PROTO((int, char **));
int region          PROTO((int, char **));
int resid           PROTO((int, char **));
int resize          PROTO((int, char **));
int section         PROTO((int, char **));
int set             PROTO((int, char **));
int simage          PROTO((int, char **));
int sort_vectors    PROTO((int, char **));
int sprintf_opihi   PROTO((int, char **));
int stats           PROTO((int, char **));
int style           PROTO((int, char **));
int subpix          PROTO((int, char **));
int subraster       PROTO((int, char **));
int subset          PROTO((int, char **));
int textline        PROTO((int, char **));
int tv              PROTO((int, char **));
int uniq            PROTO((int, char **));
int vectobuf        PROTO((int, char **));
int vstat           PROTO((int, char **));
int wd              PROTO((int, char **));
int write_vectors   PROTO((int, char **));
int zap             PROTO((int, char **));
int zeropts         PROTO((int, char **));
int zplot           PROTO((int, char **));

static Command user[] = {  
  {"abszero", 	   abszero,       "find filter zeropts"},
  {"applyfit",     applyfit,      "apply fit to new vector"},
  {"applyfit2d",   applyfit2d,    "apply 2-d fit to new vector"},
  {"badimages",    badimages,     "look for images with anomalous astrometry"},
  {"badimages",    badimages,     "look for images with anomalous astrometry"},
  {"box",     	   box,           "draw a box on the plot"},
  {"calextract",   calextract,    "extract dmags"},
  {"calextract",   calextract,    "extract dmags"},
  {"calmextract",  calmextract,   "extract dmags"},
  {"cals",    	   cals,          "plot calibration data"},
  {"catalog", 	   catlog,        "plot catalog stars"},
  {"catalog", 	   catlog,        "plot catalog stars"},
  {"ccd",     	   ccd,           "plot color-color diagram"},
  {"ccd",     	   ccd,           "plot color-color diagram"},
  {"ccdextract",   ccdextract,    "extract star coords from color-color diagram"},
  {"center",       center,        "center image on coords"},
  {"center",       center,        "center image on coords"},
  {"cgrid",   	   cgrid,         "plot sky coordinate grid"},
  {"cgrid",   	   cgrid,         "plot sky coordinate grid"},
  {"clear",   	   clear,         "erase plot"},
  {"cmatch",  	   cmatch,        "match two catalogs"},
  {"cmatch",  	   cmatch,        "match two catalogs"},
  {"cmd",     	   cmd,           "plot cmd of stars in current region"},
  {"cmd",     	   cmd,           "plot cmd of stars in current region"},
  {"cmdextract",   cmdextract,    "extract stars based on cmd regions"},
  {"cmpread",      cmpread,       "read data from cmp format files"},
  {"cmpread",      cmpread,       "read data from cmp format files"},
  {"concat",  	   concat,        "reduce vector dimension"},
  {"contour", 	   contour,       "create contour from image"},
  {"cplot",   	   cplot,         "plot vectors in sky coordinates"},
  {"cplot",   	   cplot,         "plot vectors in sky coordinates"},
  {"create",  	   create,        "create a new vector"},
  {"csystem", 	   csystem,       "convert between coordinate systems"},
  {"csystem", 	   csystem,       "convert between coordinate systems"},
  {"ctimes",  	   ctimes,        "convert between time formats"},
  {"ctimes",  	   ctimes,        "convert between time formats"},
  {"cursor",  	   cursor,        "get coords from cursor"},
  {"czplot",  	   czplot,        "plot scaled vectors in sky coordinates"},
  {"czplot",  	   czplot,        "plot scaled vectors in sky coordinates"},
  {"datafile",     datafile,      "define file to read vectors"},
  {"date",    	   date,          "get current date"},
  {"ddmagextr",    ddmagextract,  "plot magnitude differences"},
  {"ddmags",       ddmags,        "plot magnitude differences"},
  {"ddmags",       ddmags,        "plot magnitude differences"},
  {"delete",  	   delete,        "delete vectors or matrices"},
  {"device",  	   device,        "set / get current graphics device"},
  {"dmagaves",     dmagaves,      "plot differential magnitudes between filters"},
  {"dmagextract",  dmagextract,   "extract stars based on differential magnitudes between filters"},
  {"dmagmeas",     dmagmeas,      "plot differential magnitudes between filters"},
  {"dmags",   	   dmags,         "plot differential magnitudes between filters"},
  {"dmags",   	   dmags,         "plot differential magnitudes between filters"},
  {"dmt",          dmt,           "plot mag scatter"},
  {"dmt",          dmt,           "plot mag scatter"},
  {"dumpmags",     dumpmags,      "custom dB dumping thingy"},
  {"dumpmags",     dumpmags,      "custom dB dumping thingy"},
  {"elixir",       elixir,        "get status info from elixir"},
  {"elixir",       elixir,        "get status info from elixir"},
  {"extract", 	   extract,       "extract vectors from catalogs"},
  {"file",    	   file,          "test for a file"},
  {"fit",     	   fit,           "fit polynomial to vector pair"},
  {"fit2d",        fit2d,         "fit 2-d polynomial to vector triplet"},
  {"gaussj",  	   gaussjordan,   "solve Ax = B (N-D)"},
  {"gcat",         gcat,          "get catalog at location"},
  {"gcat",         gcat,          "get catalog at location"},
  {"gimages",      gimages,       "get images at location"},
  {"gimages",      gimages,       "get images at location"},
  {"grid",    	   grid,          "plot cartesian grid"},
  {"gstar",   	   gstar,         "get star statistics"},
  {"gstar",   	   gstar,         "get star statistics"},
  {"gtypes",       gtypes,        "get type fractions"},
  {"gtypes",       gtypes,        "get type fractions"},
  {"histogram",    histogram,     "generate histogram from vector"},
  {"images",  	   images,        "plot image boxes"},
  {"images",  	   images,        "plot image boxes"},
  {"imbox",   	   imbox,         "plot expected image box"},
  {"imbox",   	   imbox,         "plot expected image box"},
  {"imdata",  	   imdata,        "extract data for specific images"},
  {"imdata",  	   imdata,        "extract data for specific images"},
  {"imdense", 	   imdense,       "image density plot"},
  {"imdense", 	   imdense,       "image density plot"},
  {"imextract",    imextract,     "extract vectors from catalogs"},
  {"imextract",    imextract,     "extract vectors from catalogs"},
  {"imlist",  	   imlist,        "list image info"},
  {"imlist",  	   imlist,        "list image info"},
  {"imphot",  	   imphot,        "image photometry info"},
  {"imphot",  	   imphot,        "image photometry info"},
  {"imrough",      imrough,       "get info from imruf database"},
  {"imrough",      imrough,       "get info from imruf database"},
  {"imsearch",     imsearch,      "get info from imreg database"},
  {"imsearch",     imsearch,      "get info from imreg database"},
  {"imstats", 	   imstats,       "plot image statistics"},
  {"imstats", 	   imstats,       "plot image statistics"},
  {"interpolate",  interpolate,   "interpolate between vector pairs"},
  {"jpeg",         jpeg,          "write text line on graph"},
  {"labels",  	   labels,        "define labels for plot"},
  {"lcat",    	   lcat,          "list catalogs in region"},
  {"lcat",    	   lcat,          "list catalogs in region"},
  {"lcurve",  	   lcurve,        "plot lightcurve for a star"},
  {"lcurve",  	   lcurve,        "plot lightcurve for a star"},
  {"limits",  	   limits,        "define plot limits"},
  {"buffers",      list_buffers,  "list the currently allocated buffers"},
  {"vectors", 	   list_vectors,  "list vectors"},
  {"local",  	   local,         "define local variables"},
  {"mcreate", 	   mcreate,       "create a matrix"},
  {"mextract",	   mextract,      "extract vectors from catalogs"},
  {"mextract",	   mextract,      "extract vectors from catalogs"},
  {"mget",    	   mget,          "extract a vector from a matrix"},
  {"mset",    	   mset,          "insert a vector in a matrix"},
  {"pcat",    	   pcat,          "plot catalog boundaries"},
  {"pcat",    	   pcat,          "plot catalog boundaries"},
  {"photcodes",    photcodes,     "list photometry codes"},
  {"photcodes",    photcodes,     "list photometry codes"},
  {"photresid",    photresid,     "plot photometry residuals"},
  {"plot",    	   plot,          "plot a pair of vectors"},
  {"pmeasure",	   pmeasure,      "plot individual measurements"},
  {"pmeasure",	   pmeasure,      "plot individual measurements"},
  {"precess", 	   precess,       "precess coordinates"},
  {"precess", 	   precess,       "precess coordinates"},
  {"print",   	   print,         "write vectors to file"},
  {"print",   	   print,         "write vectors to file"},
  {"procks",  	   procks,        "plot rocks"},
  {"procks",  	   procks,        "plot rocks"},
  {"ps",      	   ps,            "define labels for plot"},
  {"rd",           rd,            "load fits image"},
  {"read",         read_vectors,  "read vectors from datafile"},
  {"region",  	   region,        "define sky region for plot"},
  {"region",  	   region,        "define sky region for plot"},
  {"resid",   	   resid,         "plot residuals"},
  {"resize",  	   resize,        "set graphics/image window size"},
  {"section", 	   section,       "define section of graph"},
  {"set",     	   set,           "vector math"},
  {"simage",  	   simage,        "plot stars in an image"},
  {"simage",  	   simage,        "plot stars in an image"},
  {"sort",    	   sort_vectors,  "sort list of vectors"},
  {"sprintf", 	   sprintf_opihi, "formated print to variable"},
  {"stats",   	   stats,         "give statistics on a portion of a buffer"},
  {"style",   	   style,         "set the style for graph plots"},
  {"subpix",  	   subpix,        "get subpixel positions"},
  {"subpix",  	   subpix,        "get subpixel positions"},
  {"subraster",    subraster,     "subraster of fits image"},
  {"subset",  	   subset,        "expand vector dimension"},
  {"textline",     textline,      "write text line on graph"},
  {"tv",      	   tv,            "display an image on the Kii window"},
  {"uniq",    	   uniq,          "create a uniq vector subset from a vector"},
  {"vectobuf",     vectobuf,      "convert vector triplet to buffer"},
  {"vstat",        vstat,         "get info from imreg database"},
  {"wd",      	   wd,            "write an image to a file"},
  {"write",   	   write_vectors, "write vectors to datafile"},
  {"zap",     	   zap,           "delete pixels"},
  {"zeropts", 	   zeropts,       "show filter zeropts"},
  {"zplot",   	   zplot,         "plot x y with size scaled by z"},
}; 
