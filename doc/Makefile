all: protocol.pdf ndlcom_logo.pdf ndlcom_header.pdf

ndlcom_logo.pdf: ndlcom_logo.tex
	pdflatex -halt-on-error $<

ndlcom_header.pdf: ndlcom_header.tex
	pdflatex -halt-on-error $<

inkscape_%.pdf inkscape_%.pdf_tex: %.svg
	inkscape --file=$< --export-area-drawing --without-gui --export-pdf=$@ --export-latex

standalone_%.tex: %.svg standalone_inscape_export.tex
	cp standalone_inscape_export.tex $@
	sed -i "s/<svgwidth>/`inkscape --query-width $*.svg`px/" $@
	sed -i "s/<textwidth>/`inkscape --query-width $*.svg`px/" $@
	sed -i "s/<textheight>/`inkscape --query-height $*.svg`px/" $@
	sed -i "s/<file>/inkscape_$*.pdf_tex/" $@

%.pdf: standalone_%.tex inkscape_%.pdf
	pdflatex -halt-on-error -jobname=protocol standalone_$*.tex

%.png: %.pdf
	pdftoppm -png $< > $@

clean:
	rm -f inkscape_* protocol.pdf ndlcom_logo.pdf ndlcom_header.pdf
