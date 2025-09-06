ROOTDIR := $(shell pwd)
SRCDIR := $(ROOTDIR)/source
PROJECTS := $(SRCDIR)/projects
PYDIR := $(PROJECTS)/py
API_MODULE = $(PYDIR)/api.pyx
PKG_NAME = py
MAX_VERSIONS := 8 9

CYTHON_OPTIONS := --timestamps -E INCLUDE_NUMPY=$(ENABLE_NUMPY) -X emit_code_comments=False


.phony: all build api clean setup update-submodules link

all: build


build: clean api
	@mkdir -p build && cd build && \
		cmake .. && \
		cmake --build . --config Release

source/projects/py/api.c: source/projects/py/api.pyx
	@echo "generating source/projects/py/api.c"
	@cython -3 $(CYTHON_OPTIONS) $(API_MODULE)

api: source/projects/py/api.c

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