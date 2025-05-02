default: kapa
help:
	@echo "make options: kapa (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/kapa2
LIB	=	$(HOME)/lib
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
INC	=	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS) -fno-strict-aliasing
FULL_LDFLAGS  = -lkapa -ldvo -lFITS -lohana $(BASE_LDFLAGS)

LDDEPS  = $(DESTLIB)/libkapa.a $(DESTLIB)/libdvo.a $(DESTLIB)/libFITS.a $(DESTLIB)/libohana.a
INDEPS  = $(DESTINC)/kapa.h $(DESTINC)/dvo.h $(DESTINC)/gfitsio.h $(DESTINC)/ohana.h

kapa: $(BIN)/kapa.$(ARCH)
install: $(DESTBIN)/kapa

PDF = \
$(SRC)/PDFit.$(ARCH).o                  \
$(SRC)/PDF_Frame.$(ARCH).o              \
$(SRC)/PDF_Objects.$(ARCH).o            \
$(SRC)/PDF_Labels.$(ARCH).o             \
$(SRC)/PDF_Image.$(ARCH).o              \
$(SRC)/PDF_Overlay.$(ARCH).o            \
$(SRC)/PDF_Textlines.$(ARCH).o          \
$(SRC)/PDF_Pixmap.$(ARCH).o             \
$(SRC)/PDF_Alpha.$(ARCH).o              \
$(SRC)/PDF_PrintUtils.$(ARCH).o 

BDRAW = \
$(SRC)/bDrawIt.$(ARCH).o            	\
$(SRC)/bDrawFrame.$(ARCH).o         	\
$(SRC)/bDrawObjects.$(ARCH).o		\
$(SRC)/bDrawLabels.$(ARCH).o            \
$(SRC)/bDrawImage.$(ARCH).o             \
$(SRC)/bDrawOverlay.$(ARCH).o 

PSFILES = \
$(SRC)/PSit.$(ARCH).o                   \
$(SRC)/PSFrame.$(ARCH).o            	\
$(SRC)/PSObjects.$(ARCH).o		\
$(SRC)/PSLabels.$(ARCH).o		\
$(SRC)/PSimage.$(ARCH).o                \
$(SRC)/PSOverlay.$(ARCH).o 	        \
$(SRC)/PSTextlines.$(ARCH).o            \
$(SRC)/PSPixmap.$(ARCH).o

KAPA = \
$(SRC)/kapa.$(ARCH).o                     $(SRC)/args.$(ARCH).o               \
$(SRC)/SetUpGraphic.$(ARCH).o             $(SRC)/NameWindow.$(ARCH).o         \
$(SRC)/CheckColors.$(ARCH).o              $(SRC)/MakeGC.$(ARCH).o             \
$(SRC)/CheckDisplayName.$(ARCH).o  	  $(SRC)/MapWindow.$(ARCH).o          \
$(SRC)/CheckGeometry.$(ARCH).o     	  $(SRC)/OpenDisplay.$(ARCH).o        \
$(SRC)/CloseDisplay.$(ARCH).o      	  $(SRC)/QuitX.$(ARCH).o              \
$(SRC)/CreateWindow.$(ARCH).o      	  $(SRC)/SetNormalHints.$(ARCH).o     \
$(SRC)/GetColor.$(ARCH).o          	  $(SRC)/SetWMHints.$(ARCH).o         \
$(SRC)/LoadFont.$(ARCH).o          	  $(SRC)/TopWindow.$(ARCH).o          \
$(SRC)/MakeCursor.$(ARCH).o		  $(SRC)/FlushDisplay.$(ARCH).o       \
$(SRC)/LoadObject.$(ARCH).o		  $(SRC)/LoadFrame.$(ARCH).o	      \
$(SRC)/LoadTextlines.$(ARCH).o		  $(SRC)/LoadLabels.$(ARCH).o         \
$(SRC)/DrawObjects.$(ARCH).o              $(SRC)/DrawFrame.$(ARCH).o          \
$(SRC)/DrawTextlines.$(ARCH).o		  $(SRC)/DrawLabels.$(ARCH).o         \
$(SRC)/MemoryDump.$(ARCH).o	          $(SRC)/TickMarks.$(ARCH).o	      \
$(SRC)/PNGit.$(ARCH).o                    $(SRC)/PPMit.$(ARCH).o	      \
$(SRC)/CheckPipe.$(ARCH).o                $(SRC)/EventLoop.$(ARCH).o          \
$(SRC)/Reconfig.$(ARCH).o                 $(SRC)/Refresh.$(ARCH).o            \
$(SRC)/Layout.$(ARCH).o		  	  $(SRC)/Sections.$(ARCH).o	      \
$(SRC)/Graphs.$(ARCH).o                   $(SRC)/SetGraphSize.$(ARCH).o       \
$(SRC)/Resize.$(ARCH).o                   $(SRC)/Relocate.$(ARCH).o           \
$(SRC)/ErasePlots.$(ARCH).o               $(SRC)/EraseImage.$(ARCH).o         \
$(SRC)/EraseCurrentPlot.$(ARCH).o         $(SRC)/EraseSections.$(ARCH).o      \
$(SRC)/SetToolbox.$(ARCH).o               $(SRC)/CrossHairs.$(ARCH).o         \
$(SRC)/SetSection.$(ARCH).o		  $(SRC)/DefineSection.$(ARCH).o      \
$(SRC)/SetLimits.$(ARCH).o                $(SRC)/SetFont.$(ARCH).o            \
$(SRC)/Center.$(ARCH).o                   $(SRC)/Remap.$(ARCH).o              \
$(SRC)/Remap8.$(ARCH).o                   $(SRC)/Remap16.$(ARCH).o            \
$(SRC)/Remap24.$(ARCH).o                  $(SRC)/Remap32.$(ARCH).o            \
$(SRC)/LoadPicture.$(ARCH).o              $(SRC)/Image.$(ARCH).o              \
$(SRC)/SetImageSize.$(ARCH).o             $(SRC)/GetPixelCount.$(ARCH).o      \
$(SRC)/StatusBox.$(ARCH).o                $(SRC)/UpdateStatusBox.$(ARCH).o    \
$(SRC)/DrawButton.$(ARCH).o               $(SRC)/DrawBitmap.$(ARCH).o         \
$(SRC)/hh_hms.$(ARCH).o	                  $(SRC)/CreateZoom.$(ARCH).o         \
$(SRC)/CreatePicture.$(ARCH).o            $(SRC)/CreateColorbar.$(ARCH).o     \
$(SRC)/MakeColormap.$(ARCH).o		  $(SRC)/SetColormap.$(ARCH).o        \
$(SRC)/PaintOverlay.$(ARCH).o		  $(SRC)/SetGraphData.$(ARCH).o       \
$(SRC)/LoadOverlay.$(ARCH).o		  $(SRC)/EraseOverlay.$(ARCH).o       \
$(SRC)/SaveOverlay.$(ARCH).o              $(SRC)/SetImageData.$(ARCH).o       \
$(SRC)/CSaveOverlay.$(ARCH).o		  $(SRC)/EraseOverlay.$(ARCH).o       \
$(SRC)/CheckVisual.$(ARCH).o              $(SRC)/CursorOps.$(ARCH).o          \
$(SRC)/InterpretKeys.$(ARCH).o            $(SRC)/InterpretPresses.$(ARCH).o   \
$(SRC)/DragColorbar.$(ARCH).o             $(SRC)/Reorient.$(ARCH).o           \
$(SRC)/InButton.$(ARCH).o                 $(SRC)/InPicture.$(ARCH).o          \
$(SRC)/CheckButtons.$(ARCH).o             $(SRC)/InvertButton.$(ARCH).o       \
$(SRC)/UpdatePointer.$(ARCH).o            $(SRC)/JPEGit24.$(ARCH).o           \
$(SRC)/ButtonFunctions.$(ARCH).o    \
$(SRC)/SetChannel.$(ARCH).o         \
$(SRC)/SetColorScale.$(ARCH).o            $(SRC)/ColorCube.$(ARCH).o          \
$(SRC)/ColorHistogram.$(ARCH).o           $(SRC)/CreateWide.$(ARCH).o

OBJ  =  $(KAPA) $(PDF) $(BDRAW) $(PSFILES)

# dependancy rules for include files ########################
$(OBJ): $(INC)/Ximage.h \
	$(INC)/constants.h \
	$(INC)/structures.h \
	$(INC)/prototypes.h

$(OBJ): $(INDEPS) $(LDDEPS)

$(BIN)/kapa.$(ARCH): $(OBJ)
