O = build

all: ../ndlcom_logo.png ../ndlcom_header.png ../osi_model.png ../bytestream_encoding.png

$(O):
	mkdir -p $@

.SECONDEXPANSION:
$(O)/%.pdf: %.tex | $$(@D)
	pdflatex -halt-on-error -output-directory=$(O) $< 
	pdflatex -halt-on-error -output-directory=$(O) $<

../%.png: $(O)/%.pdf
	pdftoppm -png $< > $@

clean:
	rm -rf $(O)
