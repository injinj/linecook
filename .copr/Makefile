outdir ?= rpmbuild/SRPMS
name   := linecook
spec   := rpm/$(name).spec
date   := $(shell date +"%a %b %d %Y")

major_num   := 1
minor_num   := 1
patch_num   := 0
build_num   := 13
version     := $(major_num).$(minor_num).$(patch_num)
ver_build   := $(version)-$(build_num)

.PHONY: srpm
srpm: $(outdir)/$(name)-$(version)-$(release).src.rpm

$(outdir)/$(name)-$(version)-$(release).src.rpm: rpmbuild/SOURCES/$(name)-$(ver_build).tar.gz
	@mkdir -p $(outdir)
	rpmbuild -D'_srcrpmdir $(outdir)' -D'_sourcedir rpmbuild/SOURCES' -bs rpmbuild/SPECS/$(name).spec

rpmbuild/SOURCES/$(name)-$(ver_build).tar.gz:
	mkdir -p rpmbuild/{RPMS,SRPMS,BUILD,SOURCES,SPECS}
	sed -e "s/99999/${build_num}/" \
	    -e "s/999.999/${version}/" \
	    -e "s/__DATE__/${date}/" < rpm/$(name).spec > rpmbuild/SPECS/$(name).spec
	mkdir -p rpmbuild/SOURCES/$(name)-${version}
	ln -sf ../../../{src,test,include,GNUmakefile,.copr} rpmbuild/SOURCES/$(name)-${version}/
	( cd rpmbuild/SOURCES && tar chzf $(name)-${ver_build}.tar.gz --exclude=".*.sw*" $(name)-${version} && rm -r -f $(name)-${version} )

