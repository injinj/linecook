# linecook makefile
lsb_dist     := $(shell if [ -f /etc/os-release ] ; then \
                  grep '^NAME=' /etc/os-release | sed 's/.*=[\"]*//' | sed 's/[ \"].*//' ; \
                  elif [ -x /usr/bin/lsb_release ] ; then \
                  lsb_release -is ; else echo Linux ; fi)
lsb_dist_ver := $(shell if [ -f /etc/os-release ] ; then \
		  grep '^VERSION=' /etc/os-release | sed 's/.*=[\"]*//' | sed 's/[ \"].*//' ; \
                  elif [ -x /usr/bin/lsb_release ] ; then \
                  lsb_release -rs | sed 's/[.].*//' ; else uname -r | sed 's/[-].*//' ; fi)
#lsb_dist     := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -is ; else uname -s ; fi)
#lsb_dist_ver := $(shell if [ -x /usr/bin/lsb_release ] ; then lsb_release -rs | sed 's/[.].*//' ; else uname -r | sed 's/[-].*//' ; fi)
uname_m      := $(shell uname -m)

short_dist_lc := $(patsubst CentOS,rh,$(patsubst RedHatEnterprise,rh,\
                   $(patsubst RedHat,rh,\
                     $(patsubst Fedora,fc,$(patsubst Ubuntu,ub,\
                       $(patsubst Debian,deb,$(patsubst SUSE,ss,$(lsb_dist))))))))
short_dist    := $(shell echo $(short_dist_lc) | tr a-z A-Z)
pwd           := $(shell pwd)
rpm_os        := $(short_dist_lc)$(lsb_dist_ver).$(uname_m)

# this is where the targets are compiled
build_dir ?= $(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)
bind      := $(build_dir)/bin
libd      := $(build_dir)/lib64
objd      := $(build_dir)/obj
dependd   := $(build_dir)/dep

default_cflags := -ggdb -O3
# use 'make port_extra=-g' for debug build
ifeq (-g,$(findstring -g,$(port_extra)))
  default_cflags := -ggdb
endif
ifeq (-a,$(findstring -a,$(port_extra)))
  default_cflags := -fsanitize=address -ggdb -O3
endif
ifeq (-mingw,$(findstring -mingw,$(port_extra)))
  CC    := /usr/bin/x86_64-w64-mingw32-gcc
  CXX   := /usr/bin/x86_64-w64-mingw32-g++
  mingw := true
endif
ifeq (,$(port_extra))
  build_cflags := $(shell if [ -x /bin/rpm ]; then /bin/rpm --eval '%{optflags}' ; \
                          elif [ -x /bin/dpkg-buildflags ] ; then /bin/dpkg-buildflags --get CFLAGS ; fi)
endif
# msys2 using ucrt64
ifeq (MSYS2,$(lsb_dist))
  mingw := true
endif
CC          ?= gcc
CXX         ?= g++
cc          := $(CC) -std=c11
cpp         := $(CXX)
arch_cflags := -fno-omit-frame-pointer
gcc_wflags  := -Wall -Wextra -Werror

# if windows cross compile
ifeq (true,$(mingw))
dll         := dll
exe         := .exe
soflag      := -shared -Wl,--subsystem,windows
fpicflags   := -fPIC -DLC_SHARED
sock_lib    := -lws2_32
dynlink_lib := -lpcre2-32
NO_STL      := 1
else
dll         := so
exe         :=
soflag      := -shared
fpicflags   := -fPIC
dynlink_lib := -lpcre2-32
endif
# make apple shared lib
ifeq (Darwin,$(lsb_dist))
dll       := dylib
endif
# rpmbuild uses RPM_OPT_FLAGS
#ifeq ($(RPM_OPT_FLAGS),)
CFLAGS ?= $(build_cflags) $(default_cflags)
#else
#CFLAGS ?= $(RPM_OPT_FLAGS)
#endif
cflags := $(gcc_wflags) $(CFLAGS) $(arch_cflags)

INCLUDES   ?= -Iinclude
DEFINES    ?=
includes   := $(INCLUDES)
defines    := $(DEFINES)
# if not linking libstdc++
ifdef NO_STL
cppflags   := -std=c++11 -fno-rtti -fno-exceptions
cpplink    := $(CC)
else
cppflags   := -std=c++11
cpplink    := $(CXX)
endif
cclink     := $(CC)

rpath      := -Wl,-rpath,$(pwd)/$(libd)
cpp_lnk    :=
lnk_lib    :=

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

liblinecook_files1:= linecook dispatch history complete prompt \
                     linesave keycook lineio ttycook kewb_utf \
                     xwcwidth9 console_vt
liblinecook_files2:= keytable hashaction
liblinecook_files := $(liblinecook_files1) $(liblinecook_files2)
liblinecook_cfile := $(addprefix src/, $(addsuffix .cpp, $(liblinecook_files1))) \
                     $(addprefix src/, $(addsuffix .c, $(liblinecook_files2)))
liblinecook_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(liblinecook_files)))
liblinecook_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(liblinecook_files)))
liblinecook_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(liblinecook_files))) \
                     $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(liblinecook_files)))
liblinecook_dlnk  := $(lnk_lib)
liblinecook_spec  := $(version)-$(build_num)_$(git_hash)
liblinecook_dylib := $(version).$(build_num)
liblinecook_ver   := $(major_num).$(minor_num)

$(libd)/liblinecook.a: $(liblinecook_objs)
$(libd)/liblinecook.$(dll): $(liblinecook_dbjs)

all_libs    += $(libd)/liblinecook.a
all_dlls    += $(libd)/liblinecook.$(dll)
all_depends += $(liblinecook_deps)

lc_example_files := example
lc_example_cfile := test/example.c
lc_example_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(lc_example_files)))
lc_example_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(lc_example_files)))
lc_example_libs  := $(libd)/liblinecook.$(dll)
lc_example_lnk   := -L$(libd) -llinecook $(lnk_lib)

$(bind)/lc_example$(exe): $(lc_example_objs) $(lc_example_libs)

all_exes    += $(bind)/lc_example$(exe)
all_depends += $(lc_example_deps)

simple_files := simple
simple_cfile := test/simple.c
simple_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(simple_files)))
simple_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(simple_files)))
simple_libs  := $(libd)/liblinecook.$(dll)
simple_lnk   := -L$(libd) -llinecook $(lnk_lib)

$(bind)/simple$(exe): $(simple_objs) $(simple_libs)

all_exes    += $(bind)/simple$(exe)
all_depends += $(simmple_deps)

lc_hist_cat_files := hist_cat
lc_hist_cat_cfile := test/hist_cat.c
lc_hist_cat_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(lc_hist_cat_files)))
lc_hist_cat_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(lc_hist_cat_files)))
lc_hist_cat_libs  := $(libd)/liblinecook.$(dll)
lc_hist_cat_lnk   := -L$(libd) -llinecook $(lnk_lib)

$(bind)/lc_hist_cat$(exe): $(lc_hist_cat_objs) $(lc_hist_cat_libs)

all_exes    += $(bind)/lc_hist_cat$(exe)
all_depends += $(lc_hist_cat_deps)

print_keys_files := print_keys keytable
print_keys_cfile := test/print_keys.c src/keytable.c
print_keys_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(print_keys_files)))
print_keys_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(print_keys_files)))
print_keys_lnk   := $(print_keys_libs)

$(bind)/print_keys$(exe): $(print_keys_objs) $(print_keys_libs)

all_exes    += $(bind)/print_keys$(exe)
all_depends += $(print_keys_deps)

net_test_files := net_test
net_test_cfile := test/net_test.cpp
net_test_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(net_test_files)))
net_test_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(net_test_files)))
net_test_libs  := $(libd)/liblinecook.$(dll)
net_test_lnk   := -L$(libd) -llinecook $(lnk_lib)

$(bind)/net_test$(exe): $(net_test_objs) $(net_test_libs)

all_exes    += $(bind)/net_test$(exe)
all_depends += $(net_test_deps)

all_dirs := $(bind) $(libd) $(objd) $(dependd)

console_test_cfile := test/console_test.cpp

README.md: $(bind)/print_keys$(exe) doc/readme.md
	  cat doc/readme.md > README.md
	  $(bind)/print_keys$(exe) >> README.md

src/hashaction.c: $(bind)/print_keys$(exe) include/linecook/keycook.h
	$(bind)/print_keys$(exe) hash > src/hashaction.c.1
	if [ $$? != 0 ] || cmp -s src/hashaction.c src/hashaction.c.1 ; then \
	  echo no change ; \
	  rm -f src/hashaction.c.1 ; \
	  touch src/hashaction.c ; \
	else \
	  mv src/hashaction.c.1 src/hashaction.c ; \
	  cat doc/readme.md > README.md ; \
	  $(bind)/print_keys$(exe) >> README.md ; \
	fi

all: $(all_libs) $(all_dlls) $(all_exes) cmake

.PHONY: cmake
cmake: CMakeLists.txt

.ONESHELL: CMakeLists.txt
CMakeLists.txt: .copr/Makefile
	@cat <<'EOF' > $@
	cmake_minimum_required (VERSION 3.9.0)
	if (POLICY CMP0111)
	  cmake_policy(SET CMP0111 OLD)
	endif ()
	project (linecook)
	include_directories (
	  include
	)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	  add_definitions(/DPCRE2_STATIC)
	  if ($$<CONFIG:Release>)
	    add_compile_options (/arch:AVX2 /GL /std:c11 /wd5105)
	  else ()
	    add_compile_options (/arch:AVX2 /std:c11 /wd5105)
	  endif ()
	  if (NOT TARGET pcre2-32-static)
	    add_library (pcre2-32-static STATIC IMPORTED)
	    set_property (TARGET pcre2-32-static PROPERTY IMPORTED_LOCATION_DEBUG ../pcre2/build/Debug/pcre2-32-staticd.lib)
	    set_property (TARGET pcre2-32-static PROPERTY IMPORTED_LOCATION_RELEASE ../pcre2/build/Release/pcre2-32-static.lib)
	    include_directories (../pcre2/build)
	  else ()
	    include_directories ($${CMAKE_BINARY_DIR}/pcre2)
	  endif ()
	else ()
	  add_compile_options ($(cflags))
	  if (TARGET pcre2-32-static)
	    include_directories ($${CMAKE_BINARY_DIR}/pcre2)
	  endif ()
	endif ()
	add_library (linecook STATIC $(liblinecook_cfile))
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	  set (ws_lib ws2_32)
	endif ()
	if (TARGET pcre2-32-static)
	  link_libraries (linecook pcre2-32-static $${ws_lib})
	else ()
	  link_libraries (linecook -lpcre2-32)
	endif ()
	if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
	  add_executable (lc_example $(lc_example_cfile))
	else ()
	  add_executable (console_test $(console_test_cfile))
	  add_executable (net_test $(net_test_cfile))
	  target_link_libraries (net_test ws2_32)
	endif ()
	add_executable (simple $(simple_cfile))
	add_executable (lc_hist_cat $(lc_hist_cat_cfile))
	add_executable (print_keys $(print_keys_cfile))
	EOF

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

ifeq (SunOS,$(lsb_dist))
remove_rpath = rpath -r
else
ifeq (Darwin,$(lsb_dist))
remove_rpath = true
else
remove_rpath = chrpath -d
endif
endif
# target used by rpmbuild, dpkgbuild
.PHONY: dist_bins
dist_bins: $(all_libs) $(all_dlls) $(bind)/lc_example$(exe) $(bind)/lc_hist_cat$(exe)
	$(remove_rpath) $(libd)/liblinecook.$(dll)
	$(remove_rpath) $(bind)/lc_example
	$(remove_rpath) $(bind)/lc_hist_cat
	$(remove_rpath) $(bind)/print_keys

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
install_prefix ?= /usr/local
else
# debuild uses DESTDIR to put things into debian/libdecnumber/usr
install_prefix = $(DESTDIR)/usr
endif
# this should be 64 for rpm based, /64 for SunOS
install_lib_suffix ?=

install: dist_bins
	install -d $(install_prefix)/lib$(install_lib_suffix)
	install -d $(install_prefix)/bin $(install_prefix)/include/linecook
	for f in $(libd)/liblinecook.* ; do \
	if [ -h $$f ] ; then \
	cp -a $$f $(install_prefix)/lib$(install_lib_suffix) ; \
	else \
	install $$f $(install_prefix)/lib$(install_lib_suffix) ; \
	fi ; \
	done
	install $(bind)/lc_example $(install_prefix)/bin
	install $(bind)/lc_hist_cat $(install_prefix)/bin
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

ifeq (Darwin,$(lsb_dist))
$(libd)/%.dylib:
	$(cpplink) -dynamiclib $(cflags) -o $@.$($(*)_dylib).dylib -current_version $($(*)_dylib) -compatibility_version $($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_dylib).dylib $(@F).$($(*)_ver).dylib && ln -f -s $(@F).$($(*)_ver).dylib $(@F)
else
$(libd)/%.$(dll):
	$(cpplink) $(soflag) $(rpath) $(cflags) -o $@.$($(*)_spec) -Wl,-soname=$(@F).$($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_spec) $(@F).$($(*)_ver) && ln -f -s $(@F).$($(*)_ver) $(@F)
endif

$(bind)/%$(exe):
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

