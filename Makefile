ROOTDIR := $(shell pwd)
SRCDIR := $(ROOTDIR)/source
PROJECTS := $(SRCDIR)/projects
PYDIR := $(PROJECTS)/py
API_MODULE = $(PYDIR)/api.pyx
PKG_NAME = py
MAX_VERSIONS := 8 9

CYTHON_OPTIONS := --timestamps -E INCLUDE_NUMPY=$(ENABLE_NUMPY) -X emit_code_comments=False
# the interpreter Cython is installed in: gen_api_stub.py imports Cython
CYTHON_PYTHON = $(shell head -1 "$$(command -v cython)" | cut -c3-)

# Relocatable builds: buildpy.py builds Python into build/install (or support/),
# then py is configured against it. See README.md.
VARIANTS := shared-ext static-ext framework-ext framework-pkg
PYTHON ?= python3
TARGETS ?= py
# minimum macOS for the bundled Python; py.mxo follows it (default: buildpy.py's)
ifdef MACOSX_DEPLOYMENT_TARGET
export MACOSX_DEPLOYMENT_TARGET
endif


.PHONY: all build api stub clean setup update-submodules link test local $(VARIANTS)

all: build


build: clean api
	@mkdir -p build && cd build && \
		cmake .. && \
		cmake --build . --config Release

source/projects/py/api.c: source/projects/py/api.pyx
	@echo "generating source/projects/py/api.c"
	@cython -3 $(CYTHON_OPTIONS) $(API_MODULE)

api: source/projects/py/api.c stub

stub:
	@$(CYTHON_PYTHON) $(PYDIR)/scripts/gen_api_stub.py

$(VARIANTS):
	@$(PYTHON) $(SRCDIR)/scripts/buildpy.py -t $@
	@# start from an empty bundle: another variant's Python would stay in it
	@rm -rf $(TARGETS:%=externals/%.mxo)
	@cmake -S . -B build --fresh -DBUILD_VARIANT=$@ && \
		cmake --build build --config Release --target $(TARGETS)

local:
	@rm -rf $(TARGETS:%=externals/%.mxo)
	@cmake -S . -B build --fresh -DBUILD_VARIANT=local && \
		cmake --build build --config Release --target $(TARGETS)

test:
	@cmake -S . -B build && \
		cmake --build build --config Release --target py_code_tests py_code_lsp_integration && \
		cd build && ctest -C Release --output-on-failure

clean:
	@rm -rf exterals build


setup: update-submodules link
	$(call section,"setup complete")

update-submodules:
	$(call section,"updating git submodules")
	@git submodule init && git submodule update

link:
	$(call section,"symlink to Max 'Packages' Directories")
	@for MAX_VERSION in $(MAX_VERSIONS); do \
		MAX_DIR="Max $${MAX_VERSION}" ; \
		PACKAGES="$(HOME)/Documents/$${MAX_DIR}/Packages" ; \
		PY_PACKAGE="$${PACKAGES}/$(PKG_NAME)" ; \
		if [ -d "$${PACKAGES}" ]; then \
			echo "symlinking to $${PY_PACKAGE}" ; \
			if ! [ -L "$${PY_PACKAGE}" ]; then \
				ln -s "$(ROOTDIR)" "$${PY_PACKAGE}" ; \
				echo "... symlink created" ; \
			else \
				echo "... symlink already exists" ; \
			fi \
		fi \
	done