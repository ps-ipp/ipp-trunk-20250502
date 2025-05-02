SOURCE  = PSDC-940-005

FIGURES = figures/exposure_ota.eps \
	figures/ext_theta.eps \
	figures/ipp_workflow.eps

RM=/bin/rm -f

figures/%.eps: figures/%.fig
	fig2dev -L eps $< $@

all: figures
	latex $(SOURCE).tex
	bibtex $(SOURCE)
	latex $(SOURCE).tex
	latex $(SOURCE).tex
	dvipdf PSDC-940-005.dvi
	@echo
	@echo
	@echo "IPP-MOPS documentation in pdf format has been succesfully built"
	@echo 

figures: $(FIGURES)

clean:
	$(RM) *.aux *.dvi *.out *.pdf *.toc *.log *~ *.blg *.bbl
	$(RM) $(FIGURES)
