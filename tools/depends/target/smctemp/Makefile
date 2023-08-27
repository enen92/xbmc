include ../../Makefile.include SMCTEMP-VERSION ../../download-files.include
DEPS = ../../Makefile.include SMCTEMP-VERSION  Makefile ../../download-files.include \

LIBDYLIB=$(PLATFORM)/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) staticlib

.installed-$(PLATFORM): $(LIBDYLIB)
	PREFIX=$(PREFIX) $(MAKE) -C $(PLATFORM) installstaticlib
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
