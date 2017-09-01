default: all

.DEFAULT:
	cd src && $(MAKE) $@
	cd alimq && $(MAKE) $@

deps:
	cd libs && $(MAKE) $@
	
clean:
	cd src && $(MAKE) $@
	cd alimq && $(MAKE) $@

distclean:
	cd libs && $(MAKE) $@
	
.PHONY: clean distclean
