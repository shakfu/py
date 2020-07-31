# ============================================================================
# VARIABLES & CONSTANTS

# constants
COLOR_BOLD_CYAN="\033[1;36m"
COLOR_RESET="\033[m"

# project variables
NAME=py
PROJECT=${NAME}.xcodeproj
TARGETS=py pyjs
EXTENSION=api.pyx

PACKAGE=${HOME}"/Documents/Max 8/Packages/"${NAME}
PKG_DIRS=docs examples externals help init \
         javascript jsextensions media patchers

PYDIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
SRCDIR:=$(shell dirname $(abspath ${PYDIR}))
ROOTDIR:=$(shell dirname $(abspath ${SRCDIR}))


# ifdef MYFLAG
# CFLAGS += -DMYFLAG
# endif

# ============================================================================
# FUNCTIONS

# $(call section,string)
section = @echo ${COLOR_BOLD_CYAN}">>> ${1}"${COLOR_RESET}

# $(call xbuild,name)
define xbuild-targets
$(call section,"build $1")
@for target in ${TARGETS}; do \
		xcodebuild -project targets/$1/py-js.xcodeproj -target $$target ; \
	done
endef

# $(call xbuild,name,flags)
define xbuild-targets-flags
$(call section,"build $1 with flags: $2")
@for target in ${TARGETS}; do \
		xcodebuild -project targets/$1/py-js.xcodeproj -target $$target GCC_PREPROCESSOR_DEFINITIONS='$$GCC_PREPROCESSOR_DEFINITIONS $2 ' ; \
	done
endef


# $call xclean,name)
define xclean-build
$(call section,"cleaning build artifacts from $1 target")
@find **/$1 | grep -E "(build)" | xargs rm -rf
endef

# ============================================================================
# TARGETS

.PHONY: all

all: default

# High-Level
# -----------------------------------------------------------------------
.PHONY: default bin-homebrew-sys \
		bin-homebrew-pkg bin-homebrew-ext \
		bin-framework-pkg bin-framework-ext \
		src-framework-pkg src-framework-ext \
		src-shared-pkg src-shared-ext \
		src-static-pkg src-static-ext \
		bin-resource-ext

default: bin-homebrew-sys

bin-homebrew-sys: clean-bin-homebrew-sys build-bin-homebrew-sys pkg

bin-homebrew-pkg: clean-bin-homebrew-pkg build-bin-homebrew-pkg pkg pkg-support

bin-homebrew-ext: clean-bin-homebrew-ext build-bin-homebrew-ext pkg

bin-framework-pkg: clean-bin-framework-pkg build-bin-framework-pkg pkg pkg-support

bin-framework-ext: clean-bin-framework-ext build-bin-framework-ext pkg

bin-resource-ext: clean-bin-resource-ext build-bin-resource-ext pkg

src-framework-pkg: clean-src-framework-pkg build-src-framework-pkg pkg pkg-support

src-framework-ext: clean-src-framework-ext build-src-framework-ext pkg

src-shared-pkg: clean-src-shared-pkg build-src-shared-pkg pkg pkg-support

src-shared-ext: clean-src-shared-ext build-src-shared-ext pkg

src-static-pkg: clean-src-static-pkg build-src-static-pkg pkg pkg-support

src-static-ext: clean-src-static-ext build-src-static-ext pkg

# DEPLOYING
# -----------------------------------------------------------------------
.PHONY: pkg pkg-support dist


dist:
	$(call section,"preparing for distribution")
	@echo "do it here with git"

pkg:
	$(call section,"deploying to local package")
	@mkdir -p ${PACKAGE}
	@for target in ${PKG_DIRS}; do \
    	rsync -a --delete $(ROOTDIR)/$${target} ${PACKAGE}; \
	done

pkg-support:
	$(call section,"deploying support/ to local package")
	@rsync -a --delete $(ROOTDIR)/support ${PACKAGE}


# Building
# -----------------------------------------------------------------------
.PHONY: build build-extension \
		build-bin-homebrew-sys \
		build-bin-homebrew-pkg build-bin-homebrew-ext \
		build-bin-framework-pkg build-bin-framework-ext \
		build-src-framework-pkg build-src-framework-ext \
		build-src-shared-pkg build-src-shared-ext \
		build-src-static-pkg build-src-static-ext \
		build-bin-resource-ext

build: build-bin-homebrew-sys
	$(call section,"build project")

build-bin-beeware-ext: build-extension
	$(call xbuild-targets-flags,"bin-beeware-ext","PY_STATIC_EXT")

build-bin-homebrew-sys: build-extension
	$(call xbuild-targets,"bin-homebrew-sys")

build-bin-homebrew-pkg: prep-bin-homebrew-pkg
	$(call xbuild-targets,"bin-homebrew-pkg")

build-bin-homebrew-ext: prep-bin-homebrew-ext
	$(call xbuild-targets,"bin-homebrew-ext")

build-bin-framework-pkg: prep-bin-framework-pkg
	$(call xbuild-targets,"bin-framework-pkg")

build-bin-framework-ext: prep-bin-framework-ext
	$(call xbuild-targets,"bin-framework-ext")

build-bin-resource-ext: prep-bin-resource-ext
	$(call xbuild-targets,"bin-resource-ext")

build-src-framework-pkg: prep-src-framework-pkg
	$(call xbuild-targets,"src-framework-pkg")

build-src-framework-ext: prep-src-framework-ext
	$(call xbuild-targets,"src-framework-ext")

build-src-shared-pkg: prep-src-shared-pkg
	$(call xbuild-targets,"src-shared-pkg")

build-src-shared-ext: prep-src-shared-ext
	$(call xbuild-targets,"src-shared-ext")

build-src-static-pkg: prep-src-static-pkg
	$(call xbuild-targets,"src-static-pkg")

build-src-static-ext: prep-src-static-ext
	$(call xbuild-targets,"src-static-ext")

build-extension:
	$(call section,"generate c code from cython extension")
	@cython -3 ${EXTENSION}

# re-compile only (without prep)
# -----------------------------------------------------------------------
.PHONY: compile compile-extension \
		compile-bin-homebrew-sys \
		compile-bin-homebrew-pkg compile-bin-homebrew-ext \
		compile-bin-framework-pkg compile-bin-framework-ext \
		compile-src-framework-pkg compile-src-framework-ext \
		compile-src-shared-pkg compile-src-shared-ext \
		compile-src-static-pkg compile-src-static-ext \
		compile-bin-resource-ext

compile: compile-bin-homebrew-sys
	$(call section,"compile project")

compile-bin-homebrew-sys: compile-extension
	$(call xbuild-targets,"bin-homebrew-sys")

compile-bin-homebrew-pkg:
	$(call xbuild-targets,"bin-homebrew-pkg")

compile-bin-homebrew-ext:
	$(call xbuild-targets,"bin-homebrew-ext")

compile-bin-framework-pkg:
	$(call xbuild-targets,"bin-framework-pkg")

compile-bin-framework-ext:
	$(call xbuild-targets,"bin-framework-ext")

compile-bin-resource-ext:
	$(call xbuild-targets,"bin-resource-ext")

compile-src-framework-pkg:
	$(call xbuild-targets,"src-framework-pkg")

compile-src-framework-ext:
	$(call xbuild-targets,"src-framework-ext")

compile-src-shared-pkg:
	$(call xbuild-targets,"src-shared-pkg")

compile-src-shared-ext:
	$(call xbuild-targets,"src-shared-ext")

compile-src-static-pkg:
	$(call xbuild-targets,"src-static-pkg")

compile-src-static-ext:
	$(call xbuild-targets,"src-static-ext")

compile-extension:
	$(call section,"generate c code from cython extension")
	@cython -3 ${EXTENSION}


# Pre-build prep
# -----------------------------------------------------------------------
.PHONY: prep-bin-homebrew-pkg prep-bin-homebrew-ext \
		prep-bin-framework-pkg prep-bin-framework-ext \
		prep-src-framework-pkg prep-src-framework-ext \
		prep-src-shared-pkg prep-src-shared-ext \
		prep-src-static-pkg prep-src-static-ext \
		prep-bin-resource-ext
		

prep-bin-homebrew-pkg:
	$(call section,"build homebrew python from binary for package")
	@bash scripts/bin-homebrew.sh pkg
	
prep-bin-homebrew-ext:
	$(call section,"build homebrew python from binary for external")
	@bash scripts/bin-homebrew.sh ext

prep-bin-framework-pkg:
	$(call section,"build framework python from binary for package")
	@bash scripts/bin-framework.sh pkg
	
prep-bin-framework-ext:
	$(call section,"build framework python from binary for external")
	@bash scripts/bin-framework.sh ext

prep-bin-resource-ext:
	$(call section,"build resource python from binary for external")
	@bash scripts/bin-resource-ext.sh ext

prep-src-framework-pkg:
	$(call section,"build framework python from source for package")
	@bash scripts/src-framework.sh pkg
	
prep-src-framework-ext:
	$(call section,"build framework python from source for external")
	@bash scripts/src-framework.sh ext

prep-src-shared-pkg:
	$(call section,"prepare shared python from source for package")
	@bash scripts/src-shared.sh pkg

prep-src-shared-ext:
	$(call section,"prepare shared python from source for external")
	@bash scripts/src-shared.sh ext

prep-src-static-pkg:
	$(call section,"build static python from source for package")
	@bash scripts/src-static.sh pkg

prep-src-static-ext:
	$(call section,"build static python from source for external")
	@bash scripts/src-static.sh ext

# Testing
# -----------------------------------------------------------------------
.PHONY: test

test:
	$(call section,"testing planned")
	echo "remember to test!"

# Styling
# -----------------------------------------------------------------------
.PHONY: style clang-format duplo

style: clang-format

clang-format:
	$(call section,"clang-format")
	@clang-format -i -style=file py.c
	@clang-format -i -style=file py.h
	@clang-format -i -style=file pyjs.c

lizard:
	$(call section,"lizard complexity analysis")
	@lizard -o report.html py.c

duplo:
	$(call section,"checking code duplication")
	#@find . -type f \( -iname "*.c" -o -iname "*.h" \) > files.lst
	@find . -type f \( -iname "py.c" \) > files.lst
	@duplo files.lst duplicates.txt
	@rm files.lst


# Cleaning
# -----------------------------------------------------------------------
.PHONY: reset clean clean-build clean-pkg clean-support clean-externals \
		clean-targets-build clean-bin-homebrew-sys \
		clean-bin-homebrew-pkg clean-bin-homebrew-ext \
		clean-bin-framework-pkg clean-bin-framework-ext \
		clean-src-framework-pkg clean-src-framework-ext \
		clean-src-shared-pkg clean-src-shared-ext \
		clean-src-static-pkg clean-src-static-ext \
		clean-bin-resource-ext

reset: clean clean-targets-build

clean: clean-externals clean-build clean-support clean-pkg

clean-build: clean-bin-homebrew-sys  \
			 clean-bin-homebrew-pkg clean-bin-homebrew-ext  \
			 clean-bin-framework-pkg clean-bin-framework-ext \
			 clean-src-framework-pkg clean-src-framework-ext \
			 clean-src-shared-pkg clean-src-shared-ext \
			 clean-src-static-pkg clean-src-static-ext \
			 clean-bin-resource-ext

clean-targets-build:
	$(call section,"cleaning targets/build directory")
	@rm -rf ${PYDIR}/targets/build

clean-externals:
	$(call section,"cleaning externals")
	@for target in ${TARGETS}; do \
		rm -rf ${ROOTDIR}/externals/$$target.mxo  ; \
	done

clean-bin-homebrew-sys:
	$(call xclean-build,"bin-homebrew-sys")

clean-bin-homebrew-pkg:
	$(call xclean-build,"bin-homebrew-pkg")

clean-bin-homebrew-ext:
	$(call xclean-build,"bin-homebrew-ext")

clean-bin-framework-pkg:
	$(call xclean-build,"bin-framework-pkg")

clean-bin-framework-ext:
	$(call xclean-build,"bin-framework-ext")

clean-bin-resource-ext:
	$(call xclean-build,"bin-resource-ext")

clean-src-framework-pkg:
	$(call xclean-build,"src-framework-pkg")

clean-src-framework-ext:
	$(call xclean-build,"src-framework-ext")

clean-src-shared-pkg:
	$(call xclean-build,"src-shared-pkg")

clean-src-shared-ext:
	$(call xclean-build,"src-shared-ext")

clean-src-static-pkg:
	$(call xclean-build,"src-static-pkg")

clean-src-static-ext:
	$(call xclean-build,"src-static-ext")

clean-support:
	$(call section,"cleaning support directory")
	@rm -rf ${ROOTDIR}/support/*

clean-pkg:
	$(call section,"cleaning py pkg")
	@rm -rf ${PACKAGE}

clean-xcode: clean-build
	$(call section,"cleaning xcode detritus")
	@find . | grep -E "(project.xcworkspace|xcuserdata)" | xargs rm -rf

