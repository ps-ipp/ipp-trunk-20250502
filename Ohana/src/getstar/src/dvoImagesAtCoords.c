# include "dvoImagesAtCoords.h"

// We'd like to do this but since pstamp gets built later this won't work
// so just redefine the macro
// # include "pstamp.h"
#define PSTAMP_NO_OVERLAP 28

static int readPoints(char *filename, Point **pointsOut);
static int makePoint(double ra, double dec, Point **pointsOut);
static int ListImagesAtCoords (Image *dbImages, Point *points, int Npoints);
static int readPointFromFile(FILE *f, Point *pt);

int main (int argc, char **argv) {

  off_t NdbImages;
  int status;
  Image *dbImages;
  FITS_DB db;

  SetSignals ();
  ConfigInit_coords (&argc, argv);
  args_coords (argc, argv);
  
  Point *points = NULL;
  int readStdin = 0;
  int Npoints = 0;
  if (coordsFile) {
      if (strcmp(coordsFile, "-") == 0) {
        readStdin = 1;
      } else {
        Npoints = readPoints(coordsFile, &points);
      }
  } else {
      Npoints = makePoint(cmd_line_ra, cmd_line_dec, &points);
  }
  if (!Npoints && !readStdin) {
    exit(1);
  }

  if (astromFile) {
      dbImages = ReadImageFiles(astromFile, &NdbImages);
  } else {
    /*** update the image table ***/
    /* setup image table format and lock */
    gfits_db_init (&db);
    db.mode   = dvo_catalog_catmode (CATMODE);
    db.format = dvo_catalog_catformat (CATFORMAT);
    status    = dvo_image_lock (&db, ImageCat, 3600.0, LCK_SOFT);  // shorter timeout?
    if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

    /* load or create the image table */
    if (db.dbstate == LCK_EMPTY) {
      fprintf (stderr, "no images in database (%s)\n", ImageCat);
      exit (1);
    } else {
      if (!dvo_image_load (&db, VERBOSE, FALSE)) {
        Shutdown ("can't read image catalog %s", db.filename);
      }
    }
    dvo_image_unlock (&db);

    // convert database table to internal structure
    dbImages = gfits_table_get_Image (&db.ftable, &NdbImages, &db.scaledValue, &db.nativeOrder);
    if (!dbImages) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
    }
  }
  
  if (readStdin) {
    Point pt;
    memset(&pt, 0, sizeof(pt));
    pt.Nmatches = 0;
    ALLOCATE(pt.matches, Match, 20);
    pt.arrayLength = 20;

    int status;
    while ((status = readPointFromFile(stdin, &pt))) {
      if (status < 0) {
        fprintf(stdout, "ERROR malformed input line\n");
        exit(1);
      }
      if (MatchCoords (dbImages, NdbImages, &pt, 1)) {
        ListImagesAtCoords(dbImages, &pt, 1);
      }
      fprintf(stdout, "DONE\n");
      fflush(stdout);
      pt.Nmatches = 0;
    }
  } else {
    if (MatchCoords (dbImages, NdbImages, points, Npoints)) {
      ListImagesAtCoords(dbImages, points, Npoints);
    } else {
      exit(PSTAMP_NO_OVERLAP);
    }
  }
  exit(0);
}

static int readPointFromFile(FILE *f, Point *pt) {
    char buf[80];
    char *good = fgets(buf, 80, f);
    if (!good) {
        return 0;
    }
    // fprintf(stderr, "READ: %s\n", buf);
    int Nread = sscanf(buf, "%d %lf %lf\n", &pt->id, &pt->ra, &pt->dec);
    if (Nread == 3) {
        // got one
        return 1;
    } else if (Nread == -1) {
        // all done time to go
        return 0;
    } else {
        // fprintf(stderr, "malformed input line: %d\n", Nread);
        return -1;
    }
}

static int readPoints(char *filename, Point **pointsOut)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
      fprintf (stderr, "failed to open coordinate file %s", filename);
      exit(1);
    }

    int LEN = 20;
    Point *pts;
    ALLOCATE(pts, Point, LEN);
    int Npoints = 0;

    int Nread;
    // File Format is simple text file.
    // Each Line has 3 values separated by spaces that represent the coordinates to match.
    //
    // 'unique integer id' 'ra in degrees' 'declination in degrees'
    // we don't check the id for uniqueness (garbage in garbage out).

    while ((Nread = fscanf(f, "%d %lf %lf\n", &pts[Npoints].id, &pts[Npoints].ra, &pts[Npoints].dec)) == 3) {
        pts[Npoints].Nmatches = 0;
        ALLOCATE(pts[Npoints].matches, Match, 20);
        pts[Npoints].arrayLength = 20;

        if (++Npoints >= LEN) {
            LEN += 20;
            REALLOCATE(pts, Point, LEN);
        }
    }
    if (Nread != -1) {
      fprintf (stderr, "unexpected data on line %d of points file %s", Npoints, filename);
      exit(1);
    }

    *pointsOut = pts;

    return Npoints;
}

static int makePoint(double ra, double dec, Point **pointsOut)
{
    Point *pts;
    ALLOCATE(pts, Point, 1);

    pts[0].id = 1;
    pts[0].ra = ra;
    pts[0].dec = dec;
    pts[0].Nmatches = 0;
    ALLOCATE(pts[0].matches, Match, 20);
    pts[0].arrayLength = 20;

    *pointsOut = pts;

    return 1;
}

static int ListImagesAtCoords (Image *dbImages, Point *points, int Npoints) 
{
  int j;
  int i;
  for (j = 0; j < Npoints; j++) {
      Point *pt = points + j;
      for (i = 0; i < pt->Nmatches; i++) {
        int N = pt->matches[i].n;
        double x = pt->matches[i].x;
        double y = pt->matches[i].y;

        char *name;
        char *copy = NULL;
        // output of lookup is filename[class_id.hdr] for astrometry files
        char *left_bracket = rindex(dbImages[N].name, '[');
        if (!fullNames && left_bracket) {
            copy = strcreate(left_bracket + 1);
            name = copy;
            // zap the .hdr]
            char *dot = index(name, '.');
            if (dot) {
                *dot = 0;
            }
        } else {
            name = dbImages[N].name;
        }
        if (LISTCHIPCOORDS) {
            fprintf (stdout, "%d %lf %lf %s %8.2lf %8.2lf\n", pt->id, pt->ra, pt->dec, name, x, y);
        } else {
            fprintf (stdout, "%d %lf %lf %s\n", pt->id, pt->ra, pt->dec, name);
        }
        if  (copy) {
	  free(copy);
        }
    }
  }
  
  return (TRUE);
}

