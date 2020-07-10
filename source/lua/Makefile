
# input variables
NAME=luna
VERSION=5.4.0
MACOSX_DEPLOYMENT_TARGET=10.9
LUAJIT_VERSION=2.0

# misc variables
URL_LUA=https://www.lua.org/ftp/lua-${VERSION}.tar.gz
ARCHIVE=$(shell basename ${URL_LUA})
SRC_LUA=$(shell basename -s .tar.gz ${ARCHIVE})

ROOT=$(shell pwd)
EXTERNALS=${ROOT}/../../externals
EXTERNAL=${EXTERNALS}/${NAME}.mxo
EXTERNAL_LUNAR=${EXTERNALS}/lunar.mxo
PACKAGE=${HOME}"/Documents/Max 8/Packages/py"
BUILD=${ROOT}/build
VERSION_LUAJIT=${BUILD}/luajit-${LUAJIT_VERSION}
LUAJIT=${BUILD}/luajit
LUA=${BUILD}/lua

CFLAGS_LUA=-I${LUA}/include
LDFLAGS_LUA=-L${LUA}/lib -llua -lm

CFLAGS_LUAJIT=-I${LUAJIT}/include/luajit-2.0
LDFLAGS_LUAJIT=-L${LUAJIT}/lib -lluajit -lm ${LDFLAGS_EXTRA_LUAJIT}
LDFLAGS_EXTRA_LUAJIT=-pagezero_size 10000 -image_base 100000000


COLOR_BOLD_CYAN="\033[1;36m"
COLOR_RESET="\033[m"

# ============================================================================
# FUNCTIONS

# $(call section,string)
section = @echo ${COLOR_BOLD_CYAN}">>> ${1}"${COLOR_RESET}


# ============================================================================
# TARGETS

all: build-externals deploy

.PHONY: build-externals show prep-luajit test-luajit prep-lua test-lua clean

show:
	@echo "ROOT: ${ROOT}"
	@echo "BUILD: ${BUILD}"
	@echo "LUAJIT_SRC: ${VERSION_LUAJIT}"
	@echo "PREFIX=${LUAJIT}"

prep-luajit:	
	@make -C ${VERSION_LUAJIT} \
		install PREFIX=${LUAJIT} \
		MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET}
	@rm ${LUAJIT}/lib/*.dylib
	@ln -s ${LUAJIT}/lib/libluajit-5.1.a ${LUAJIT}/lib/libluajit.a

prep-lua:
	$(call section,"building ${SRC_LUA} installation from source")
	@mkdir -p ${BUILD}
	@mkdir -p ${LUA}
	@curl ${URL_LUA} -o ${BUILD}/${ARCHIVE}
	@tar -C ${BUILD} -xvf ${BUILD}/${ARCHIVE}
	@make -C ${BUILD}/${SRC_LUA}
	@make -C ${BUILD}/${SRC_LUA} install INSTALL_TOP=${LUA}

build-externals:
	$(call section,"building externals")
	@xcodebuild -project ${NAME}.xcodeproj -target luna
	@xcodebuild -project ${NAME}.xcodeproj -target lunar

deploy:
	$(call section,"deploying external to local package")
	@rm -rf ${PACKAGE}/externals/${NAME}.mxo
	@cp -rf ${EXTERNAL} ${PACKAGE}/externals
	@rm -rf ${PACKAGE}/externals/lunar.mxo
	@cp -rf ${EXTERNAL_LUNAR} ${PACKAGE}/externals

test-lua:
	@cc -o test_lua test_lua.c ${CFLAGS_LUA} ${LDFLAGS_LUA}

test-luajit:
	@cc -o test_lua test_luajit.c ${CFLAGS_LUAJIT} ${LDFLAGS_LUAJIT}

clean:
	@rm test_lua

