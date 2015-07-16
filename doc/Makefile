all: protocol.pdf

inkscape_%.pdf inkscape_%.pdf_tex: %.svg
	inkscape --file=$< --export-area-drawing --without-gui --export-pdf=$@ --export-latex

standalone_%.tex: %.svg standalone_inscape_export.tex
	cp standalone_inscape_export.tex $@
	sed -i "s/<svgwidth>/`inkscape --query-width $*.svg`px/" $@
	sed -i "s/<textwidth>/`inkscape --query-width $*.svg`px/" $@
	sed -i "s/<textheight>/`inkscape --query-height $*.svg`px/" $@
	sed -i "s/<file>/inkscape_$*.pdf_tex/" $@

%.pdf: standalone_%.tex inkscape_%.pdf
	pdflatex -jobname=protocol standalone_$*.tex

%.png: %.pdf
	pdftoppm -png $< > $@

clean:
	rm -f inkscape_* protocol.pdf
