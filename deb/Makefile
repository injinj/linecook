
debdate  := $(shell date -R)
debbuild := dpkgbuild/$(name_ver)
debian   := dpkgbuild/$(name_ver)/debian
origtgz  := $(name)_$(version).orig.tar.gz
debtgz   := $(name)_$(ver_build).debian.tar.gz

.PHONY: debian
debian:
	rm -rf $(debbuild)
	mkdir -p $(debbuild)
	for i in $(srctree) ; do ln -sf ../../$$i $(debbuild) ; done
	( cd dpkgbuild && tar chzf $(origtgz) --exclude=".*.sw*" $(name_ver) && rm -rf $(name_ver) )
	mkdir -p $(debian)
	cp -a deb/copyright deb/rules deb/compat $(debian)
	sed -e "s/999.999/${ver_build}/" \
	    -e "s/__DATE__/${debdate}/" < deb/changelog > $(debian)/changelog
	sed -e "s/99999/${major_num}.${minor_num}/" < deb/control > $(debian)/control
	( cd dpkgbuild && tar chzf $(debtgz) --exclude=".*.sw*" $(name_ver) && rm -rf $(name_ver) )

.PHONY: dist_dpkg
dist_dpkg: debian
	( cd dpkgbuild && tar xzf $(debtgz) && tar xzf $(origtgz) )
	( cd dpkgbuild/$(name_ver) && debuild -i -us -uc -F )
	rm -rf dpkgbuild/$(name_ver)
