#!/usr/bin/env bash

#PYTHON_VERSION=3.8.5
PYTHON_VERSION=`python --version | sed s/Python[[:space:]]//`
OPENSSL_VERSION=1.1.1g
BZIP2_VERSION=1.0.8
XZ_VERSION=5.2.5
GETTEXT_VERSION=0.20.2

MAC_DEP_TARGET=10.13

# --- need not modify below except to change excluded modules

PYTHON_VER=${PYTHON_VERSION%.*} 		# 3.8
PYTHON_VER_NODOT="${PYTHON_VER//./}"	# 38 
PYTHON_NAME=python${PYTHON_VER}			# python3.8

URL_PY=https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz
URL_SSL=https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
URL_BZ2=https://sourceware.org/pub/bzip2/bzip2-${BZIP2_VERSION}.tar.gz
URL_XZ=http://tukaani.org/xz/xz-${XZ_VERSION}.tar.gz
# URL_GETTEXT=https://ftp.gnu.org/pub/gnu/gettext/gettext-${GETTEXT_VERSION}.tar.gz

URLS="$URL_PY $URL_SSL $URL_BZ2 $URL_XZ"

# GETTEXT dependency
LIBINTL=/usr/local/opt/gettext/lib/libintl.a


ROOT=$(pwd)
SUPPORT=${ROOT}/../../support
EXTERNALS=${ROOT}/../../externals
SOURCE=${ROOT}/../source
FRAMEWORKS=${SUPPORT}/Frameworks
SCRIPTS=${ROOT}/scripts
PATCH=${ROOT}/patch

BUILD=${ROOT}/targets/build
BUILD_DOWNLOADS=${BUILD}/downloads

BUILD_SRC=${BUILD}/src
BUILD_SRC_PY=${BUILD_SRC}/Python-${PYTHON_VERSION}
BUILD_SRC_SSL=${BUILD_SRC}/openssl-${OPENSSL_VERSION}
BUILD_SRC_BZ2=${BUILD_SRC}/bzip2-${BZIP2_VERSION}
BUILD_SRC_XZ=${BUILD_SRC}/xz-${XZ_VERSION}

BUILD_LIB=${BUILD}/lib
BUILD_LIB_SSL=${BUILD_LIB}/openssl
BUILD_LIB_BZ2=${BUILD_LIB}/bzip2
BUILD_LIB_XZ=${BUILD_LIB}/xz
BUILD_LIB_PY_STATIC=${BUILD_LIB}/python-static
BUILD_LIB_PY_SHARED=${BUILD_LIB}/python-shared
BUILD_LIB_PY_FRAMEWORK=${BUILD_LIB}/Python.framework

PREFIX=${SUPPORT}/${PYTHON_NAME}
BIN=${PREFIX}/bin
LIB=${PREFIX}/lib/python${PYTHON_VER}

DYLIB_NAME=libpython${PYTHON_VER}
DYLIB=${DYLIB_NAME}.dylib
PY_EXTERNAL=${EXTERNALS}/py.mxo
PYJS_EXTERNAL=${EXTERNALS}/pyjs.mxo

COLOR_BOLD_YELLOW="\033[1;33m"
COLOR_BOLD_BLUE="\033[1;34m"
COLOR_BOLD_GREEN="\033[1;32m"
COLOR_BOLD_MAGENTA="\033[1;35m"
COLOR_BOLD_CYAN="\033[1;36m"
COLOR_RESET="\033[m"

section() {
	echo -e "${COLOR_BOLD_CYAN}>>> ${1} ${COLOR_RESET}"
}

info() {
    echo -e $COLOR_BOLD_YELLOW"INFO: "$COLOR_RESET$1
}

warn() {
    echo -e $COLOR_BOLD_MAGENTA"WARNING: "$COLOR_RESET$1
}

success() {
    echo -e $COLOR_BOLD_GREEN"SUCCESS: "$COLOR_RESET$1
}

fail() {
    echo -e $COLOR_BOLD_MAGENTA"FAILURE: "$COLOR_RESET$1
}

remove() {
	info "removing $1"
	rm -rf $1
}

write_setup_local() {
	cp $PATCH/$1 $BUILD_SRC_PY/Modules/Setup.local
}

apply_patch() {
	patch -p1 < $PATCH/$1
}

reverse_patch() {
	patch -p1 -R < $PATCH/$1
}

get_url() {
	section "retrieving: $1"
	mkdir -p $BUILD_DOWNLOADS
	mkdir -p $BUILD_SRC
	archive=$(basename $1)
	if [[ $archive == *.tar.gz ]]; then
		name=$(basename -s .tar.gz $archive)
	elif [[ $archive == *.tgz ]]; then
		name=$(basename -s .tgz $archive)
	fi

	if [ ! -f $BUILD_DOWNLOADS/${name}.tgz ]; then
		curl --fail -L $1 -o $BUILD_DOWNLOADS/${name}.tgz
	else
		info "skipping existing src archive: $archive"
	fi

	if [ ! -d $BUILD_SRC/$name ]; then
		tar -C $BUILD_SRC -xvf $BUILD_DOWNLOADS/${name}.tgz
	else
		info "skipping existing src dir: $name"
	fi
}

get_all() {
	for url in $URLS; do
		get_url $url
	done;
}

get_python() {
	get_url $URL_PY
}

build_ssl() {
	cd $BUILD_SRC_SSL
	make clean
	./config no-shared no-tests --prefix=$BUILD_LIB_SSL
	make install_sw
	cd $ROOT
}

build_bz2() {
	cd $BUILD_SRC_BZ2
	make clean
	make install PREFIX=$BUILD_LIB_BZ2
	cd $ROOT
}

build_xz() {
	cd $BUILD_SRC_XZ
	./configure \
    	MACOSX_DEPLOYMENT_TARGET=${MAC_DEP_TARGET} \
        --disable-shared --enable-static \
        --prefix=$BUILD_LIB_XZ
    make install
	cd $ROOT
}

test_python_build() {
	DIR=$1
	cd $DIR
	RESULT=`./bin/python3.8 -c "import os; print(os.__file__)"`
	EXPECTED=`pwd`/lib/python38.zip/os.py
	if [[ $RESULT == $EXPECTED ]]; then
		success "$DIR"
	else
		fail "$DIR"
	fi
	cd $ROOT
}


# takes $PREFIX as as $1 
clean_python() {
	DIR=$1
	BIN=$1/bin
	LIB=$1/lib/python${PYTHON_VER}

	find $DIR | grep -E "(__pycache__|\.pyc|\.pyo$)" | xargs rm -rf
	find $LIB | grep -E "(tests|test)" | xargs rm -rf
	rm -rf $LIB/site-packages/*

	rm -rf $LIB/distutils/command/*.exe
	rm -rf $DIR/lib/pkgconfig
	rm -rf $DIR/share

	MODS="\
		cgi.py
		cgitb.py
		config-${PYTHON_VER}-darwin
		ctypes
		curses
		distutils
		ensurepip
		gettext.py
		idlelib
		lib2to3
		pydoc.py
		pydoc_data
		sqlite3
		sunau.py
		turtle.py
		turtledemo
		venv
	"
	for mod in $MODS; do
		rm -rf $LIB/${mod}
	done

	EXTS="\
		_tkinter
		_ctypes
		_multibytecodec
		_codecs_jp
		_codecs_hk
		_codecs_cn
		_codecs_kr
		_codecs_tw
		_codecs_iso2022
		_curses
		_curses_panel
	"
	for ext in $EXTS; do
		rm -rf $LIB/lib-dynload/${ext}.cpython-${PYTHON_VER_NODOT}-darwin.so
	done

	BINS="\
		2to3-${PYTHON_VER}
		idle${PYTHON_VER}
		2to3-${PYTHON_VER}
		idle${PYTHON_VER}
		easy_install-${PYTHON_VER}
		pip${PYTHON_VER}
		python${PYTHON_VER}m
		pyvenv-${PYTHON_VER}
		pydoc${PYTHON_VER}
	"
	for executable in $BINS; do
		rm -rf $BIN/{$executable}
	done
}

zip_python_library() {
	DIR=$1
	LIB=$1/lib/python${PYTHON_VER}
	rm -rf ${LIB}/site-packages
	mv $LIB/lib-dynload $DIR
	cp $LIB/os.py $DIR
	python -m zipfile -c $DIR/lib/python${PYTHON_VER_NODOT}.zip $LIB/*
	rm -rf $LIB
	mkdir -p $LIB
	mv $DIR/lib-dynload $LIB
	 # Need to have a copy of os.py to remain in site-packages
	 # or it will fail to pick up library.zip
	mv $DIR/os.py $LIB
	mkdir $LIB/site-packages
}

build_python_shared() {
	local PREFIX=${1:-$BUILD_LIB_PY_SHARED}
	cd $BUILD_SRC_PY
	make clean
    write_setup_local setup-shared.local
    #apply_patch
	./configure \
		MACOSX_DEPLOYMENT_TARGET=$MAC_DEP_TARGET \
	 	--prefix=$PREFIX \
	 	--enable-shared \
	 	--with-openssl=$BUILD_LIB_SSL \
        --without-doc-strings \
        --enable-ipv6 \
        --without-ensurepip \
	 	--with-lto \
	 	--enable-optimizations
	make altinstall
	cd $ROOT
	clean_python $PREFIX
	zip_python_library $PREFIX
	test_python_build $PREFIX
}

build_python_static() {
	local PREFIX=${1:-$BUILD_LIB_PY_STATIC}
	cd $BUILD_SRC_PY
	make clean
    write_setup_local setup-static-min2.local
    apply_patch makesetup.patch
	./configure \
		MACOSX_DEPLOYMENT_TARGET=$MAC_DEP_TARGET \
        --prefix=$PREFIX \
        --without-doc-strings \
        --enable-ipv6 \
        --without-ensurepip \
        --with-lto \
        --enable-optimizations
	make altinstall
	reverse_patch makesetup.patch
	cd $ROOT
	clean_python $PREFIX
	zip_python_library $PREFIX
	test_python_build $PREFIX
}

build_python_framework() {
	local PREFIX=${1:-$BUILD_LIB}
	# assuming PREFIX=${SUPPORT}/Frameworks
	local FWK_PREFIX=${PREFIX}/Python.framework/Versions/${PYTHON_VER}
	cd $BUILD_SRC_PY
	make clean
    write_setup_local setup-shared.local
    apply_patch makesetup.patch
	./configure \
		MACOSX_DEPLOYMENT_TARGET=${MAC_DEP_TARGET} \
        --enable-framework=${PREFIX} \
        --with-openssl=${BUILD_LIB_SSL} \
        --without-doc-strings \
        --enable-ipv6 \
        --without-ensurepip \
        --with-lto \
        --enable-optimizations
	make altinstall
	reverse_patch makesetup.patch
	cd $ROOT
	clean_python $FWK_PREFIX
	zip_python_library $FWK_PREFIX
	test_python_build $FWK_PREFIX
}


reset_build_downloads() {
	rm -rf $BUILD_DOWNLOADS
}

reset_build_src() {
	rm -rf $BUILD_DOWNLOADS
}

# reset() {
# 	rm -rf $BUILD_LIB
# }

reset_all() {
	rm -rf $BUILD_DOWNLOADS
	rm -rf $BUILD_SRC
	rm -rf $BUILD_LIB
}


fix_python_dylib_for_pkg() {
	cd $PREFIX/lib
	chmod 777 ${DYLIB}
	# assumes python in installed in $PREFIX
	# ../../../../support/python3.7/lib/libpython3.7m.dylib
	install_name_tool -id @loader_path/../../../../support/${PYTHON_NAME}/lib/${DYLIB} ${DYLIB}
	echo "fix_python_dylib_for_pkg done"
	# otool -L ${DYLIB}
	cd $ROOT
}

fix_python_dylib_for_ext() {
	cd $PREFIX/lib
	chmod 777 ${DYLIB}
	# assumes cp -rf $PREFIX/* -> same directory as py extension in py.mxo
	install_name_tool -id @loader_path/${DYLIB} ${DYLIB}
	echo "fix_python_dylib_for_ext done"
	cd $ROOT
}


install_dependencies() {
	echo "checking if previous build exists"
	mkdir -p $BUILD
	if [ ! -d $BUILD_DOWNLOADS ] ; then
		echo "retrieving downloads"
		get_all
	fi
	if [ ! -d $BUILD_LIB_SSL ] ; then
		echo "building ssl"
		build_ssl
	fi
	if [ ! -d $BUILD_LIB_BZ2 ] ; then
		echo "building bz2"
		build_bz2
	fi
	if [ ! -d $BUILD_LIB_xz ] ; then
		echo "building xz"
		build_xz
	fi
}

install_python_pkg() {
	install_dependencies
	install_python
	fix_python_dylib_for_pkg
}

install_python_ext() {
	install_dependencies
	install_python
	fix_python_dylib_for_ext
}

