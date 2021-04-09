all: presentation.pdf

%.pdf: %.md
	pandoc $< -t beamer -o $@
