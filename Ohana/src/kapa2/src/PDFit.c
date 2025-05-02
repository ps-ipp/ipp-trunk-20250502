# include "Ximage.h"
# define XOFFSET 0
# define YOFFSET 0

int PDFcommand (int sock) {

  int status, scaleMode, pageMode;
  char filename[1024], pagename[1024];

  /* expect a line telling the number of bytes and a filename */
  KiiScanMessage (sock, "%s %s %d %d", filename, pagename, &scaleMode, &pageMode);
  status = PDFit (filename, pagename, scaleMode, pageMode);
  return (status);
}

int PDFit (char *filename, char *pagename, int scaleMode, int pageMode) {

  double scale;
  IOBuffer buffer;

  Graphic *graphic = GetGraphic();

  // PDF files can be extended, but require effort to re-rewrite the cross-ref table
  // Disable this for now
  if (pageMode != KAPA_PS_NEWPLOT) {
    fprintf (stderr, "cannot extend PDF files yet\n");
    return (TRUE);  /* true because otherwise it quits kapa! */    
  }

  /* two scaling options: expand to fit page / keep absolute size */ 
  if (scaleMode) {
    scale = MIN (fabs(500.0 / graphic->dx), fabs (700.0 / graphic->dy));
  } else {
    scale = 72.0 / 96.0; /* ratio of screen pixels to points */
  }

  PDF_FILE *obj = PDF_Open (filename);

  // we are using a very minimal file structure:
      
  // Header
  PDF_Print (obj, 0, "%%PDF-1.4\n");
  PDF_Print (obj, 0, "%%%c%c%c%c\n", 0xff, 0xfe, 0xfd, 0xfc);

  // Root : File Catalog:
  PDF_Print (obj, 1, "1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n");

  // Pages
  PDF_Print (obj, 2, "2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n");

  // Page Container:
  PDF_Print (obj, 3, "3 0 obj << /Type /Page /Parent 2 0 R\n");
  PDF_Print (obj, 0, " /MediaBox [%d %d %.0f %.0f]\n", 
	   XOFFSET, YOFFSET,
	   XOFFSET + scale*graphic->dx,
	   YOFFSET + scale*graphic->dy);

  PDF_Print (obj, 0, " /Resources <<\n");
  PDF_Print (obj, 0, "   /Font <<\n");
  PDF_Print (obj, 0, "     /Ft  7 0 R\n");
  PDF_Print (obj, 0, "     /Fh  8 0 R\n");
  PDF_Print (obj, 0, "     /Fc  9 0 R\n");
  PDF_Print (obj, 0, "     /Fs 10 0 R\n");
  PDF_Print (obj, 0, "   >>\n");
  PDF_Print (obj, 0, "   /XObject 4 0 R\n");
  PDF_Print (obj, 0, "   /ExtGState 6 0 R\n");
  PDF_Print (obj, 0, " >>\n");
  PDF_Print (obj, 0, " /Contents 5 0 R >> endobj\n"); 

  // /XObject points to a deferred object which lists the images (if any) (must use object 4)
  // /Contents points to a deferred object which lists all content streams (must use object 5)
  // /ExtGState points to a deferred object which lists all opacity levels (must use object 6)

  // Font Dictionaries
  PDF_Print (obj,  7, " 7 0 obj << /Type /Font /Subtype /Type1 /Name /Ft /BaseFont /Times-Roman >>\n");
  PDF_Print (obj,  8, " 8 0 obj << /Type /Font /Subtype /Type1 /Name /Fh /BaseFont /Helvetica >>\n");
  PDF_Print (obj,  9, " 9 0 obj << /Type /Font /Subtype /Type1 /Name /Fc /BaseFont /Courier >>\n");
  PDF_Print (obj, 10, "10 0 obj << /Type /Font /Subtype /Type1 /Name /Fs /BaseFont /Symbol >>\n");

  PDF_AlphaInit();

  // NOTE: The PDF_WriteStream and PDF_WriteImage functions use the next available object,
  // but deferred objects have to have a fixed value.  Reserve them before the last object above

  // create stream here (include scale operations)
  PDF_CreateStream (&buffer, scale, XOFFSET, YOFFSET);

  // create streams for each of the sections (one for image, one for graph)
  int Nsection = GetNumberOfSections ();
  for (int i = 0; i < Nsection; i++) {
    Section *section = GetSectionByNumber (i);
    if (section->image) {
      PDF_Image (obj, section->image, &buffer);
      // flushes buffer when done
    }
    if (section->graph) {
      PDF_Frame (section->graph, &buffer); 
      PDF_Objects (section->graph, &buffer);
      PDF_Labels (section->graph, &buffer);
      PDF_Textlines (section->graph, &buffer);
      PDF_WriteStream (obj, &buffer);
    }
  }
  PDF_WriteStream (obj, &buffer);
  FreeIOBuffer (&buffer);

  PDF_Close (obj); // frees the PDF_PrintObject
  return (TRUE);
}

/* 

PDF creation notes:

* numbers are all either float or int in decimal
* ascii strings are in ()
* hex data strings are in <>
  * always represents a sequence of bytes (if an odd number are used, LAST nibble is 0)

cross-reference table:

xref
Nstart Nobj
0000000000 65535 f \n <- this line 
nnnnnnnnnn ggggg n \n
 nnnnnnnnnn - byte offset to start of obejct (zero padded) 
 ggggg - generation number (can be zero for Kapa outputs if we are not re-writing PDFs)

trailer section:

trailer\n
<< /Size NNN\n    <- number of entries in the cross-ref table above
   /Root 1 0 R\n  <- REQUIRED (catalog dictionary for PDF doc)
   /Info 2 0 R\n  <- OPTIONAL (information dictionary)
   /ID [<3DD227F735946325424B0AD32F7FE72D><3DD227F735946325424B0AD32F7FE72D>]
>>
startxref
2724 <- start byte of xref
%%EOF

*/
