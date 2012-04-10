#makefile
	# ITY - Typografie a publikovanu - 1.projekt
	# 7.3.2010
	# Author: Jaroslav Sendler, FIT, xsendl00 AT stud.fit.vutbr.cz


CO=xsendl00
NAME=xsendl00-fit


all: $(CO).pdf

pdf: $(CO).pdf

$(CO).ps: $(CO).dvi
	dvips $(CO)

$(CO).pdf: clean
	pdflatex $(CO)
	pdflatex $(CO)
	pdflatex $(CO)

$(CO).dvi: $(CO).tex 
	latex $(CO)
	latex $(CO)
	latex $(CO)



#clean:
#	rm -f *.dvi *.log $(CO).blg $(CO).bbl $(CO).toc *.aux $(CO).out $(CO).lof
#	rm -f $(CO).pdf
#	rm -f *~

pack:
#	tar czvf bp-xsendl00.tar.gz *.tex *.bib *.bst *.cls ./fig/* ./pic/* Makefile Changelog
	zip bp-xsendl00.zip *.tex *.bib *.bst *.cls ./fig/* ./pic/* Makefile Changelog

#all:	$(FILE)

#$(FILE): $(FILE).tex
#	latex $(FILE).tex
#	pdflatex $(FILE).tex
#	latex $(FILE).tex
#	latex $(FILE).tex
#	dvips $(FILE).dvi
#	#ps2pdf -t=a4 $(FILE).ps
#	ps2pdf -sPAPERSIZE=a4 $(FILE).ps
clean:
	rm -f *.dvi *.ps *.aux *.log *.out

clean-all: clean
	rm -f	*.log

remove: clean-all
	rm -f ${NAME}.tar.gz *.pdf

zip:
	tar -c ${FILE}.tex Makefile > ${NAME}.tar; \
	gzip ${NAME}.tar
