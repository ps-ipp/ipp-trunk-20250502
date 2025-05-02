# include "data.h"
// XXX adding a comment

int accum            PROTO((int, char **));
int antialias        PROTO((int, char **));
int applyfit         PROTO((int, char **));
int applyfit1d       PROTO((int, char **));
int applyfit2d       PROTO((int, char **));
int applyfit2d_full  PROTO((int, char **));
int applyfit3d       PROTO((int, char **));
int box              PROTO((int, char **));
int book_command     PROTO((int, char **));
int bisection        PROTO((int, char **));
int center           PROTO((int, char **));
int cast             PROTO((int, char **));
int chebyshev_command PROTO((int, char **));
int circstats        PROTO((int, char **));
int clear            PROTO((int, char **));
int clip             PROTO((int, char **));
int close_device     PROTO((int, char **));
int concat           PROTO((int, char **));
int contour          PROTO((int, char **));
int create           PROTO((int, char **));
int cumulative       PROTO((int, char **));
int cursor           PROTO((int, char **));
int cut              PROTO((int, char **));
int datafile         PROTO((int, char **));
int dbconnect        PROTO((int, char **));
int dbselect         PROTO((int, char **));
int dbinsert         PROTO((int, char **));
int dbadmin          PROTO((int, char **));
int dbupdate         PROTO((int, char **));
int delete           PROTO((int, char **));
int densify          PROTO((int, char **));
int device           PROTO((int, char **));
int dft2d            PROTO((int, char **));
int dimendown        PROTO((int, char **));
int dimenup          PROTO((int, char **));
int distribute       PROTO((int, char **));
int erase            PROTO((int, char **));
int extract          PROTO((int, char **));
int fft1d            PROTO((int, char **));
int fft2d            PROTO((int, char **));
int fit              PROTO((int, char **));
int fit1d            PROTO((int, char **));
int fit1d_irls       PROTO((int, char **));
int fit2d            PROTO((int, char **));
int fit2d_full       PROTO((int, char **));
int fit3d            PROTO((int, char **));
int gaussjordan      PROTO((int, char **));
int gaussdeviate     PROTO((int, char **));
int gaussintegral    PROTO((int, char **));
int grid             PROTO((int, char **));
int gridify          PROTO((int, char **));
int grow             PROTO((int, char **));
int ungridify        PROTO((int, char **));
int histogram        PROTO((int, char **));
int tdhistogram      PROTO((int, char **));
int hermitian1d      PROTO((int, char **));
int hermitian2d      PROTO((int, char **));
int idxread          PROTO((int, char **));
int imclip           PROTO((int, char **));
int imcut            PROTO((int, char **));
int imhist           PROTO((int, char **));
int impeaks          PROTO((int, char **));
int imsmooth         PROTO((int, char **));
int imsmooth_generic PROTO((int, char **));
int imsmooth_2d      PROTO((int, char **));
int imconvolve       PROTO((int, char **));
int imresample       PROTO((int, char **));
int imcollapse       PROTO((int, char **));
int integrate        PROTO((int, char **));
int interpolate      PROTO((int, char **));
int interpolate_presort PROTO((int, char **));
int join             PROTO((int, char **));
int jpeg             PROTO((int, char **));
int kapamemory       PROTO((int, char **));
int kern             PROTO((int, char **));
int keyword          PROTO((int, char **));
int labels           PROTO((int, char **));
int limits           PROTO((int, char **));
int line             PROTO((int, char **));
int list_buffers     PROTO((int, char **));
int header           PROTO((int, char **));
int list_vectors     PROTO((int, char **));
int vtype            PROTO((int, char **));
int load             PROTO((int, char **));
int lookup           PROTO((int, char **));
int matrix           PROTO((int, char **));
int match1d          PROTO((int, char **));
int match2d          PROTO((int, char **));
int mkrgb            PROTO((int, char **));
int mcreate          PROTO((int, char **));
int medacc           PROTO((int, char **));
int mgaussdev        PROTO((int, char **));
int mget             PROTO((int, char **));
int mget3d           PROTO((int, char **));
int mslice           PROTO((int, char **));
int mlayer           PROTO((int, char **));
int minterp          PROTO((int, char **));
int medimage_command PROTO((int, char **));
int mset             PROTO((int, char **));
int mprint           PROTO((int, char **));
int needles          PROTO((int, char **));
int nnet_command     PROTO((int, char **));
int peak             PROTO((int, char **));
int periodogram      PROTO((int, char **));
int periodogram_fm   PROTO((int, char **));
int plot             PROTO((int, char **));
int dot              PROTO((int, char **));
int parity           PROTO((int, char **));
int point            PROTO((int, char **));
int pdf              PROTO((int, char **));
int ps               PROTO((int, char **));
int vprint           PROTO((int, char **));
int queuelist        PROTO((int, char **));
int queueload        PROTO((int, char **));
int queueinit        PROTO((int, char **));
int queuedelete      PROTO((int, char **));
int queuedrop        PROTO((int, char **));
int queuepop         PROTO((int, char **));
int queueprint       PROTO((int, char **));
int queuepush        PROTO((int, char **));
int queuesave        PROTO((int, char **));
int queuesubstr      PROTO((int, char **));
int queuesize        PROTO((int, char **));
int queue2book       PROTO((int, char **));
int rd               PROTO((int, char **));
int rdjpg            PROTO((int, char **));
int rdseg            PROTO((int, char **));
int read_vectors     PROTO((int, char **));
int rebin            PROTO((int, char **));
int resize           PROTO((int, char **));
int reindex          PROTO((int, char **));
int relocate         PROTO((int, char **));
int rndseed          PROTO((int, char **));
int roll             PROTO((int, char **));
int rotate           PROTO((int, char **));
int save             PROTO((int, char **));
int section          PROTO((int, char **));
int set              PROTO((int, char **));
int shift            PROTO((int, char **));
int sort_vectors     PROTO((int, char **));
int spline_command   PROTO((int, char **));
int imspline_apply   PROTO((int, char **));
int imspline_construct PROTO((int, char **));
int squash3d         PROTO((int, char **));
int stats            PROTO((int, char **));
int imstats          PROTO((int, char **));
int style            PROTO((int, char **));
int subraster        PROTO((int, char **));
int subset           PROTO((int, char **));
int svd              PROTO((int, char **));
int swapbytes        PROTO((int, char **));
int textline         PROTO((int, char **));
int threshold        PROTO((int, char **));
int triangle         PROTO((int, char **));
int tv               PROTO((int, char **));
int tvchannel        PROTO((int, char **));
int tvcolors         PROTO((int, char **));
int tvcontour        PROTO((int, char **));
int tvgrid           PROTO((int, char **));
int opihi_type       PROTO((int, char **));
int opihi_size       PROTO((int, char **));
int uniq             PROTO((int, char **));
int uniqpair         PROTO((int, char **));
int unsign           PROTO((int, char **));
int vbin             PROTO((int, char **));
int vgroup           PROTO((int, char **));
int vclip            PROTO((int, char **));
int vect_select      PROTO((int, char **));
int vgrid            PROTO((int, char **));
int vgauss           PROTO((int, char **));
int vlorentz         PROTO((int, char **));
int vsigmoid         PROTO((int, char **));
int vellipse         PROTO((int, char **));
int vmaxwell         PROTO((int, char **));
int vload            PROTO((int, char **));
int vlist            PROTO((int, char **));
int vmedfilt         PROTO((int, char **));
int vzload           PROTO((int, char **));
int vstats           PROTO((int, char **));
int vstats           PROTO((int, char **));
int virls            PROTO((int, char **));
int vwtmean          PROTO((int, char **));
int vroll            PROTO((int, char **));
int vshift           PROTO((int, char **));
int vpeaks           PROTO((int, char **));
int vtransitions     PROTO((int, char **));
int vpop             PROTO((int, char **));
int vsmooth          PROTO((int, char **));
int vsh              PROTO((int, char **));
int vshfit           PROTO((int, char **));
int shterms          PROTO((int, char **));
int shfit            PROTO((int, char **));
int shdot            PROTO((int, char **));
int shapply          PROTO((int, char **));
int wd               PROTO((int, char **));
int write_vectors    PROTO((int, char **));
int xsection         PROTO((int, char **));
int zap              PROTO((int, char **));
int zplot            PROTO((int, char **));
int zcplot            PROTO((int, char **));

// ???
// int mtype            PROTO((int, char **));
//  {1, "mtype",        mtype,            "return the type of the defined buffer"},

static Command cmds[] = {  
  {1, "accum",        accum,            "accumulate vector values in another vector"},
  {1, "antialias",    antialias,        "set anti-alias sigma value for display"},
  {1, "applyfit",     applyfit1d,       "apply 1-d fit to new vector"},
  {1, "applyfit1d",   applyfit1d,       "apply 1-d fit to new vector"},
  {1, "applyfit2d",   applyfit2d,       "apply 2-d fit to new vector"},
  {1, "applyfit2d_full", applyfit2d_full, "apply 2-d fit to new vector"},
  {1, "applyfit3d",   applyfit3d,       "apply 3-d fit to new vector"},
  {1, "bisection",    bisection,        "use bisection to find threshold in vector"},
  {1, "book",         book_command,     "commands to manipulate book/page/word data"},
  {1, "box",          box,              "draw a box on the plot"},
  {1, "buffers",      list_buffers,     "list the currently allocated buffers (images)"},
  {1, "center",       center,           "center image on coords"},
  {1, "cast",         cast,             "cast input vector to specified type"},
  {1, "chebyshev",    chebyshev_command, "Chebyshev polynomial commands"},
  {1, "circstats",    circstats,        "circular statistics"},
  {1, "clear",        clear,            "erase plot"},
  {1, "clip",         imclip,           "clip values in an image to be within a range"},
  {1, "close",        close_device,     "close the current display device"},
  {1, "concat",       concat,           "append values to the end of a vector"},
  {1, "contour",      contour,          "create contour from image"},
  {1, "create",       create,           "create a new vector"},
  {1, "vcreate",      create,           "create a new vector"},
  {1, "cumulative",   cumulative,       "build a cumulative histogram from a specific histogram"},
  {1, "cursor",       cursor,           "get coords from cursor"},
  {1, "cut",          cut,              "extract a cut across an image"},
  {1, "datafile",     datafile,         "define file to read vectors"},
  {1, "dbconnect",    dbconnect,        "setup mysql db connection"},
  {1, "dbselect",     dbselect,         "extract vectors from mysql database table"},
  {1, "dbinsert",     dbinsert,         "sql insert command"},
  {1, "dbadmin",      dbadmin,          "sql admin command (create, drop)"},
  {1, "dbupdate",     dbupdate,         "sql update command"},
  {1, "delete",       delete,           "delete vectors or images"},
  {1, "densify",      densify,          "create an image histogram from a set of vectors"},
  {1, "device",       device,           "set / get current graphics device"},
  {1, "dft2d",        dft2d,            "2D discrete fourier transform"},
  {1, "dimendown",    dimendown,        "convert image to vector"},
  {1, "dimenup",      dimenup,          "convert vector to image"},
  {1, "distribute",   distribute,       "distribute vector values to a collection of vectors by index"},
  {1, "dot",          dot,              "plot a single point"},
  {1, "erase",        erase,            "erase objects on an image overlay"},
  {1, "extract",      extract,          "extract a portion of a image into another image"},
  {1, "fft1d",        fft1d,            "fft on a vector"},
  {1, "fft2d",        fft2d,            "fft on an image"},
  {1, "fit",          fit1d,            "fit polynomial to vector pair"},
  {1, "fit1d",        fit1d,            "fit polynomial to vector pair"},
  {1, "fit1d_irls",   fit1d_irls,       "fit polynomial to vector pair"},
  {1, "fit2d",        fit2d,            "fit 2-d polynomial to vector triplet"},
  {1, "fit2d_full",   fit2d_full,       "fit 2-d polynomial to vector triplet (all X^n Y^m coeffs)"},
  {1, "fit3d",        fit3d,            "fit 3-d polynomial to vector quad"},
  {1, "gaussdev",     gaussdeviate,     "generate a gaussian deviate vector"},
  {1, "gaussint",     gaussintegral,    "return the integrated gaussian vector"},
  {1, "gaussj",       gaussjordan,      "solve Ax = B (N-Dimensional)"},
  {1, "grid",         grid,             "plot cartesian grid on graph"},
  {1, "gridify",      gridify,          "convert vector triplet to image (same as vgrid?)"},
  {1, "grow",         grow,             "grow a mask"},
  {1, "header",       header,           "print image header"},
  {1, "histogram",    histogram,        "generate histogram from vector"},
  {1, "tdhistogram",  tdhistogram,      "generate 2D histogram image from vector set"},
  {1, "hermitian1d",  hermitian1d,      "generate 1-D Hermitian Polynomial"},
  {1, "hermitian2d",  hermitian2d,      "generate 2-D Hermitian Polynomial"},
  {1, "idxread",      idxread,          "read vector or image data from an IDX file"},
  {1, "imbin",        rebin,            "rebin image data by factor of N"},
  {1, "imclip",       imclip,           "clip values in an image to be within a range"},
  {1, "imcut",        imcut,            "linear image cut between arbitrary coords"},
  {1, "imresample",   imresample,       "extract arbitrary window from image"},
  {1, "imcollapse",   imcollapse,       "collapse to histogram along axis"},
  {1, "imhistogram",  imhist,           "histogram of an image region"},
  {1, "impeaks",      impeaks,          "find peaks in an image (return vectors)"},
  {1, "imsmooth",     imsmooth,         "circular gaussian smoothing"},
  {1, "imsmooth.generic", imsmooth_generic, "circular non-gaussian smoothing"},
  {1, "imsmooth.2d",  imsmooth_2d,      "circular non-gaussian smoothing"},
  {1, "imconvolve",   imconvolve,       "full 2D real-space convolution"},
  {1, "imstats",      imstats,          "statistics on a portion of an image"},
  {1, "integrate",    integrate,        "integrate a vector"},
  {1, "interpolate_presort",  interpolate_presort,      "interpolate between vector pairs"},
  {1, "join",         join,             "find the join of two ID vectors"},
  {1, "jpeg",         jpeg,             "convert display image to JPEG"},
  {1, "kapamemory",   kapamemory,       "manage kapa memory dump options"},
  {1, "kern",         kern,             "convolve with 3x3 kernel"},
  {1, "keyword",      keyword,          "extract a FITS keyword from image header"},
  {1, "labels",       labels,           "define labels for plot"},
  {1, "limits",       limits,           "define plot limits"},
  {1, "line",         line,             "plot a line"},
  {1, "load",         load,             "load an SAOimage/DS9 style overlay"},
  {1, "lookup",       lookup,           "convert vector via lookup table (vector pair)"},
  {1, "mcreate",      mcreate,          "create an image"},
  {1, "imcreate",     mcreate,          "create an image"},
  {1, "medacc",       medacc,           "accumulate vector values in another vector"},
  {1, "mgaussdev",    mgaussdev,        "generate a gaussian deviate image"},
  {1, "mget",         mget,             "extract a vector from an image"},
  {1, "mget3d",       mget3d,           "extract a vector from a 3D image"},
  {1, "mslice",       mslice,           "extract an image plane from a 3D image"},
  {1, "mlayer",       mlayer,           "insert an image plane into a 3D image"},
  {1, "imget",        mget,             "extract a vector from an image"},
  {1, "minterp",      minterp,          "interpolate image pixels"},
  {1, "iminterp",     minterp,          "interpolate image pixels"},
  {1, "medimage",     medimage_command, "median image manipulation"},
  {1, "matrix",       matrix,           "matrix math operations"},
  {1, "match1d",      match1d,          "match 2 vectors and return matched indexes"},
  {1, "match2d",      match2d,          "match 2 pairs of X,Y vectors and return matched indexes"},
  {1, "mkrgb",        mkrgb,            "convert 3 images to rgb jpeg (use Kapa for better control)"},
  {1, "mset",         mset,             "insert a vector in an image"},
  {1, "imset",        mset,             "insert a vector in an image"},
  {1, "mprint",       mprint,           "print a region of an image"},
  {1, "needles",      needles,          "plot vectors needles"},
  {1, "nnet",         nnet_command,     "Neural Network commands"},
  {1, "parity",       parity,           "set image parity"},
  {1, "peak",         peak,             "find vector peak in range"},
  {1, "periodogram",    periodogram,    "measure periods in unevenly sampled data (Lomb-Scargle)"},
  {1, "periodogram_fm", periodogram_fm, "measure periods in unevenly sampled data (generalized Lomb-Scargle; floating mean)"},
  {1, "plot",         plot,             "plot a pair of vectors"},
  {1, "png",          jpeg,             "convert display graphic to PNG"},
  {1, "point",        point,            "load image overlay with a single point"},
  {1, "ppm",          jpeg,             "convert display graphic to PPM"},
  {1, "ps",           ps,               "convert display to PostScript"},
  {1, "pdf",          pdf,              "convert display to PDF"},
  {1, "print_vectors", vprint,          "print a set of vectors"},
  {1, "vprint",       vprint,           "print a set of vectors"},
  {1, "queuedelete",  queuedelete,      "delete a queue"},
  {1, "queuedrop",    queuedrop,        "drop values from queue matching a key"},
  {1, "queueinit",    queueinit,        "create an empty queue"},
  {1, "queuelist",    queuelist,        "list defined queues"},
  {1, "queueload",    queueload,        "load queue from command"},
  {1, "queuepop",     queuepop,         "pop value from queue to variable"},
  {1, "queueprint",   queueprint,       "print the contents of a queue"},
  {1, "queuepush",    queuepush,        "push value onto queue"},
  {1, "queuesave",    queuesave,        "save the contents of a queue in a filex"},
  {1, "queuesize",    queuesize,        "show queue size"},
  {1, "queuesubstr",  queuesubstr,      "bulk replace strings in queue"},
  {1, "queue2book",   queue2book,       "convert queue with ipptool output to book"},
  {1, "ipptool2book", queue2book,       "convert queue with ipptool output to book"},
  {1, "rd",           rd,               "load fits image"},
  {1, "rdjpg",        rdjpg,            "load jpeg image"},
  {1, "rdseg",        rdseg,            "read a segment of an image from a file"},
  {1, "read",         read_vectors,     "read vectors from datafile"},
  {1, "rebin",        rebin,            "rebin image data by factor of N"},
  {1, "reindex",      reindex,          "create new vector from old vector based on index vector"},
  {1, "resize",       resize,           "set graphics/image window size"},
  {1, "relocate",     relocate,         "set graphics/image window position"},
  {1, "roll",         roll,             "roll image to new start point"},
  {1, "rndseed",      rndseed,          "set the pseudo-random seed"},
  {1, "rotate",       rotate,           "rotate image"},
  {1, "save",         save,             "save an SAOimage style image overlay"},
  {1, "section",      section,          "define section of graph"},
  {1, "select",       vect_select,      "selective vector assignment"},
  {1, "set",          set,              "image and vector math"},
  {1, "shift",        shift,            "shift data in an image"},
  {1, "size",         opihi_size,       "get vector/matrix dimension/size information"},
  {1, "sort",         sort_vectors,     "sort list of vectors"},
  {1, "spline",       spline_command,   "shift data in an image"},
  {1, "imspline.apply", imspline_apply, "apply spline fit to generate an image"},
  {1, "imspline.const", imspline_construct, "create spline 2nd deriv. terms"},
  {1, "squash3d",     squash3d,         "squash 3d buffer to 2d"},
  {1, "stats",        imstats,          "statistics on a portion of an image"},
  {1, "style",        style,            "set the style for graph plots"},
  {1, "subraster",    subraster,        "subraster of fits image"},
  {1, "subset",       subset,           "generate a vector from a portion of another vector"},
  {1, "svd",          svd,              "singular value decomposition of a matrix (image)"},
  {1, "swapbytes",    swapbytes,        "byte swap thing"},
  {1, "textline",     textline,         "write text line on graph"},
  {1, "threshold",    threshold,        "find (interpolate) location of transition"},
  {1, "triangle",     triangle,         "fill a triangular region with a value"},
  {1, "tv",           tv,               "display an image on the Kii window"},
  {1, "tvchannel",    tvchannel,        "set the current tv channel"},
  {1, "tvcolors",     tvcolors,         "set the tv colormap"},
  {1, "tvcontour",    tvcontour,        "send contour to image overlay"},
  {1, "tvgrid",       tvgrid,           "RA/DEC grid on an image"},
  {1, "type",         opihi_type,       "get scalar/vector/matrix type information"},
  {1, "ungridify",    ungridify,        "convert image region to vector triplet"},
  {1, "uniq",         uniq,             "create a uniq vector subset from a vector"},
  {1, "uniqpair",     uniqpair,         "create a uniq vector subset from a pair of vectors, saving duplicates if desired"},
  {1, "unsign",       unsign,           "toggle the UNSIGN status"},
  {1, "vbin",         vbin,             "rebin vector data by a factor of N"},
  {1, "vgroup",       vgroup,           "group y vector into bins defined by x vector values"},
  {1, "vclip",        vclip,            "clip values in a vector to be within a range"},
  {1, "vectors",      list_vectors,     "list vectors"},
  {1, "vtype",        vtype,            "return the vector type (FLT or INT)"},
  {1, "vgauss",       vgauss,           "fit a Gaussian to a vector"},
  {1, "vlorentz",     vlorentz,         "fit a Lorentzian to a vector"},
  {1, "vsigmoid",     vsigmoid,         "fit a Sigmoid to a vector"},
  {1, "vellipse",     vellipse,         "fit a Ellipse to a vector pair"},
  {1, "vgrid",        vgrid,            "generate an image from a triplet of vectors"},
  {1, "vhistogram",   histogram,        "generate histogram from vector"},
  {1, "vlist",        vlist,            "append values to a vector from command line"},
  {1, "vmedfilt",     vmedfilt,         "median filter for a vector"},
  {1, "vload",        vload,            "load vectors as overlay on image display"},
  {1, "vmaxwell",     vmaxwell,         "fit a Maxwellian to a vector"},
  {1, "vpeaks",       vpeaks,           "find coord and flux of peaks in vector"},
  {1, "vtransitions", vtransitions,     "find points in vector that cross the transition value"},
  {1, "vpop",         vpop,             "remove first element of a vector"},
  {1, "vroll",        vroll,            "roll vector elements by 1 entry"},
  {1, "vshift",       vshift,           "shift vector elements by arbitrary amount"},
  {1, "vsmooth",      vsmooth,          "Gaussian smooth of a vector"},
  {1, "vstats",       vstats,           "statistics on a vector"},
  {1, "vwtmean",      vwtmean,          "weighted mean of a vector"},
  {1, "virls",        virls,            "IRLS mean of a vector"},
  {1, "vzload",       vzload,           "load vectors as overlay on image display (scaled points)"},
  {1, "vsh",          vsh,              "Vector Spherical Harmonics"},
  {1, "vshfit",       vshfit,           "Vector Spherical Harmonics fits"},
  {1, "shterms",      shterms,          "Spherical Harmonics terms"},
  {1, "shfit",        shfit,            "Spherical Harmonics fits"},
  {1, "shdot",        shdot,            "Spherical Harmonics dot product"},
  {1, "shapply",      shapply,          "Spherical Harmonics fit application"},
  {1, "wd",           wd,               "write an image to a file"},
  {1, "write",        write_vectors,    "write vectors to datafile"},
  {1, "xsection",     xsection,         "generate cross-section histogram for an image (or region)"},
  {1, "zap",          zap,              "assign values to pixel regions"},
  {1, "zplot",        zplot,            "plot x y with size scaled by z"},
  {1, "zcplot",       zcplot,           "plot x y with color scaled by z"},
}; 

void InitData () {
  
  int i;

  InitKapa ();
  InitQueues ();
  InitBooks ();

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }
}

void FreeData () {

  FreeKapa ();
  FreeQueues ();
  FreeBooks ();
}
