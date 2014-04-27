all::
	@echo "This is the Makefile only for committing to GitHub. Use make -f not_makefile"

clean::
	rm -f consolidated_bibtex_file.bib

vi:
	make readme

include common.mk

