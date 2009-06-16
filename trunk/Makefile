#!/usr/bin/make -f
include config.make

.PHONY: src
app: src
app src: 
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean: clean-app clean-src
distclean: distclean-app distclean-src

all: src app

.PHONY:
AMAL_GEN := $(TOP_SRCDIR)/createAmalgamation.sh
amal amalgamation: 
	bash $(AMAL_GEN)

