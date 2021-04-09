# Compile with a Mac

roadmap.pdf: roadmap.tex
	pdflatex roadmap.tex > latex.log
	pdflatex roadmap.tex > latex.log

.PHONY: clean

clean:
	-rm -f *.blg *.aux roadmap.pdf *.lot *.log *.lof *.inx *.toc *.log *.bbl *~

