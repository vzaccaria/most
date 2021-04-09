
all: tmpm2.txt.pdf tmpm.pdf

tmpm.pdf: 
	sed -f mmd.sed tmp.man > tmpm.txt
	../mmd/bin/txt4pdf tmpm.txt
	putfile tmpm.txt.pdf

tmpm2.txt.pdf: 
	sed -f mmd.sed tmp2.man > tmpm2.txt
	../mmd/bin/txt4pdf tmpm2.txt
	putfile tmpm2.txt.pdf

clean:
	rm -rf *.pdf *.html *.txt *.tex
