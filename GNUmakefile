lsb_dist     := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -is ; else echo Linux ; fi)
lsb_dist_ver := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -rs | sed 's/[.].*//' ; fi)
uname_m      := $(shell uname -m)

short_dist_lc := $(patsubst CentOS,rh,$(patsubst RedHatEnterprise,rh,\
                   $(patsubst RedHat,rh,\
                     $(patsubst Fedora,fc,$(patsubst Ubuntu,ub,\
                       $(patsubst Debian,deb,$(patsubst SUSE,ss,$(lsb_dist))))))))
short_dist    := $(shell echo $(short_dist_lc) | tr a-z A-Z)
rpm_os        := $(short_dist_lc)$(lsb_dist_ver).$(uname_m)

# this is where the targets are compiled
build_dir ?= $(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)
bind      := $(build_dir)/bin
libd      := $(build_dir)/lib64
objd      := $(build_dir)/obj
dependd   := $(build_dir)/dep

# use 'make port_extra=-g' for debug build
ifeq (-g,$(findstring -g,$(port_extra)))
  DEBUG = true
endif

CC          ?= gcc
CXX         ?= $(CC) -x c++
cc          := $(CC)
cpp         := $(CXX)
arch_cflags := -fno-omit-frame-pointer
gcc_wflags  := -Wall -Wextra -Werror
fpicflags   := -fPIC
soflag      := -shared

ifdef DEBUG
default_cflags := -ggdb
else
default_cflags := -ggdb -O3
endif
# rpmbuild uses RPM_OPT_FLAGS
ifeq ($(RPM_OPT_FLAGS),)
CFLAGS ?= $(default_cflags)
else
CFLAGS ?= $(RPM_OPT_FLAGS)
endif
cflags := $(gcc_wflags) $(CFLAGS) $(arch_cflags)

INCLUDES   ?= -Iinclude
DEFINES    ?=
includes   := $(INCLUDES)
defines    := $(DEFINES)

cppflags   := -fno-rtti -fno-exceptions
#cppflags  := -fno-rtti -fno-exceptions -fsanitize=address
#cpplink   := $(CC) -lasan
cpplink    := $(CC)

rpath      := -Wl,-rpath,$(libd)
cpp_lnk    :=
lnk_lib    := -lpcre2-32

.PHONY: everything
everything: all

# copr/fedora build (with version env vars)
# copr uses this to generate a source rpm with the srpm target
-include .copr/Makefile

# debian build (debuild)
# target for building installable deb: dist_dpkg
-include deb/Makefile

all_exes    :=
all_libs    :=
all_dlls    :=
all_depends :=

liblinecook_files := linecook dispatch history complete prompt \
                     linesave keycook lineio ttycook kewb_utf \
		     xwcwidth9
liblinecook_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(liblinecook_files)))
liblinecook_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(liblinecook_files)))
liblinecook_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(liblinecook_files))) \
                     $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(liblinecook_files)))
liblinecook_dlnk  := $(lnk_lib)
liblinecook_spec  := $(version)-$(build_num)
liblinecook_ver   := $(major_num).$(minor_num)

$(libd)/liblinecook.a: $(liblinecook_objs)
$(libd)/liblinecook.so: $(liblinecook_dbjs)

all_libs    += $(libd)/liblinecook.a
all_dlls    += $(libd)/liblinecook.so
all_depends += $(liblinecook_deps)

lc_example_files := example
lc_example_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(lc_example_files)))
lc_example_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(lc_example_files)))
lc_example_libs  := $(libd)/liblinecook.so
lc_example_lnk   := -L$(libd) -llinecook $(lnk_lib)

$(bind)/lc_example: $(lc_example_objs) $(lc_example_libs)

simple_files := simple
simple_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(simple_files)))
simple_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(simple_files)))
simple_libs  := $(libd)/liblinecook.a
simple_lnk   := $(simple_libs) $(lnk_lib)

$(bind)/simple: $(simple_objs) $(simple_libs)

print_keys_files := print_keys
print_keys_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(print_keys_files)))
print_keys_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(print_keys_files)))
print_keys_libs  := $(libd)/liblinecook.a
print_keys_lnk   := $(print_keys_libs) $(lnk_lib)

$(bind)/print_keys: $(print_keys_objs) $(print_keys_libs)

all_exes    += $(bind)/lc_example $(bind)/simple $(bind)/print_keys
all_depends += $(lc_example_deps) $(simple_deps)

all_dirs := $(bind) $(libd) $(objd) $(dependd)

README.md: $(bind)/print_keys doc/readme.md
	cat doc/readme.md > README.md
	$(bind)/print_keys >> README.md

all: $(all_libs) $(all_dlls) $(all_exes) README.md

# create directories
$(dependd):
	@mkdir -p $(all_dirs)

# remove target bins, objs, depends
.PHONY: clean
clean:
	rm -r -f $(bind) $(libd) $(objd) $(dependd)
	if [ "$(build_dir)" != "." ] ; then rmdir $(build_dir) ; fi

.PHONY: clean_dist
clean_dist:
	rm -rf dpkgbuild rpmbuild

.PHONY: clean_all
clean_all: clean clean_dist

$(dependd)/depend.make: $(dependd) $(all_depends)
	@echo "# depend file" > $(dependd)/depend.make
	@cat $(all_depends) >> $(dependd)/depend.make

# target used by rpmbuild, dpkgbuild
.PHONY: dist_bins
dist_bins: $(all_libs) $(all_dlls) $(bind)/lc_example
	chrpath -d $(libd)/liblinecook.so
	chrpath -d $(bind)/lc_example

# target for building installable rpm
.PHONY: dist_rpm
dist_rpm: srpm
	( cd rpmbuild && rpmbuild --define "-topdir `pwd`" -ba SPECS/linecook.spec )

# force a remake of depend using 'make -B depend'
.PHONY: depend
depend: $(dependd)/depend.make

# dependencies made by 'make depend'
-include $(dependd)/depend.make

ifeq ($(DESTDIR),)
# 'sudo make install' puts things in /usr/local/lib, /usr/local/include
install_prefix = /usr/local
else
# debuild uses DESTDIR to put things into debian/libdecnumber/usr
install_prefix = $(DESTDIR)/usr
endif

install: dist_bins
	install -d $(install_prefix)/lib $(install_prefix)/bin $(install_prefix)/include/linecook
	for f in $(libd)/liblinecook.* ; do \
	if [ -h $$f ] ; then \
	cp -a $$f $(install_prefix)/lib ; \
	else \
	install $$f $(install_prefix)/lib ; \
	fi ; \
	done
	install $(bind)/lc_example $(install_prefix)/bin
	install -m 644 include/linecook/*.h $(install_prefix)/include/linecook

$(objd)/%.o: src/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/%.c
	$(cc) $(cflags) $(fpicflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: test/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: test/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(libd)/%.a:
	ar rc $@ $($(*)_objs)

$(libd)/%.so:
	$(cpplink) $(soflag) $(rpath) $(cflags) -o $@.$($(*)_spec) -Wl,-soname=$(@F).$($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(cpp_dll_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_spec) $(@F).$($(*)_ver) && ln -f -s $(@F).$($(*)_ver) $(@F)

$(bind)/%:
	$(cpplink) $(cflags) $(rpath) -o $@ $($(*)_objs) -L$(libd) $($(*)_lnk) $(cpp_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib)

$(dependd)/%.d: src/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.fpic.d: src/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.d: test/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: test/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

