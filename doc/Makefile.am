include $(top_srcdir)/rules.mk

EXTRA_DIST = \
	$(DOXYGEN_DIST) \
	$(EOL)

phony += doc
doc: html-local

htmllocal =
html-local: $(htmllocal)

distcleanlocal =
distclean-local: $(distcleanlocal)

# === doxygen ===========================================================

phony += doxygen doccheck

if HAVE_DOXYGEN
htmllocal += doxygen

doxygen: Doxyfile
	mkdir -p html
	doxygen

phony += doccheck
doccheck: Doxyfile
	sh -c "doxygen 2>&1 | { ! grep -i -E 'error|warn' ; }"

distcleanlocal += removedoxy
phony += removedoxy
removedoxy:
	-rm -rf html

# Generate docs into another directory, e.g., for gh-pages.  Run as
# `make doxy-to DESTDIR=<whatever>`
phony += doxy-to
doxy-to: Doxyfile
	mkdir -p "$(DESTDIR)"
	@: # see https://www.doxygen.nl/manual/faq.html#faq_cmdline
	(cat Doxyfile ; echo 'OUTPUT_DIRECTORY=$(DESTDIR)') | doxygen -

else
phony += doxygen doccheck
doxygen:
	echo "Doxygen not installed"
doccheck:
	echo "Doxygen not installed"
endif
