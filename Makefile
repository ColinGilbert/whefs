#!/usr/bin/make -f
include config.make

.PHONY: src
app: src
app src: 
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean: clean-app clean-src
distclean: distclean-app distclean-src

all: src app

