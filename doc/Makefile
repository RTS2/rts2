all: diplomka.pdf

diplomka.pdf: diplomka.dvi
	dvipdf $?

diplomka.dvi: *.tex 
	$(MAKE) -C dia
	cslatex diplomka.tex
	makeindex diplomka.idx
	cslatex diplomka.tex

clean:
	$(RM) *.aux *.idx *.dvi *.pdf *~ *.log *.ind *.ilg
	$(MAKE) -C dia clean

vlna:
	vlna *.tex
