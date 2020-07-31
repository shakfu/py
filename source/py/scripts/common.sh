#!/usr/bin/env bash

COLOR_BOLD_YELLOW="\033[1;33m"
COLOR_BOLD_BLUE="\033[1;34m"
COLOR_BOLD_MAGENTA="\033[1;35m"
COLOR_BOLD_CYAN="\033[1;36m"
COLOR_RESET="\033[m"

function section {
	echo -e "${COLOR_BOLD_CYAN}>>> ${1} ${COLOR_RESET}"
}

function warn {
    echo -e $COLOR_BOLD_YELLOW"WARNING: "$COLOR_RESET$1
}

VERSION_MAJOR=${PY_MAJ_VER:=3.8}
VERSION_MINOR=${PY_MIN_VER:=4}
SSL_VERSION=${SSL_VER:=1.1.1g}
MAC_DEP_TARGET=${MAC_DEP:=10.13}

# --- need not modify below except to change excluded modules

SEMVER=${VERSION_MAJOR}.${VERSION_MINOR}
VERSION=${VERSION_MAJOR}
VER="${VERSION//./}"
NAME=python${VERSION}

URL_PYTHON=https://www.python.org/ftp/python/${SEMVER}/Python-${SEMVER}.tgz
URL_OPENSSL=https://www.openssl.org/source/openssl-${SSL_VERSION}.tar.gz
URL_GETPIP=https://bootstrap.pypa.io/get-pip.py

ROOT=$(pwd)
PY=${ROOT}
SUPPORT=${ROOT}/../../support
EXTERNALS=${ROOT}/../../externals
SOURCE=${ROOT}/../source
FRAMEWORKS=${SUPPORT}/Frameworks
SCRIPTS=${ROOT}/scripts
BUILD=${ROOT}/targets/build
PYTHON=${BUILD}/Python-${SEMVER}
PREFIX=${SUPPORT}/${NAME}
BIN=${SUPPORT}/${NAME}/bin
LIB=${PREFIX}/lib/python${VERSION}
SSL_SRC=${BUILD}/openssl-${SSL_VERSION}
SSL=${BUILD}/ssl
TMP=${BUILD}/_tmp
DYLIB_NAME=libpython${VERSION}
DYLIB=${DYLIB_NAME}.dylib
PY_EXTERNAL=${EXTERNALS}/py.mxo
PYJS_EXTERNAL=${EXTERNALS}/pyjs.mxo

debug() {

	echo "NAME: $NAME"

	echo "VERSION_MAJOR: $VERSION_MAJOR"
	echo "VERSION_MINOR: $VERSION_MINOR"
	echo "SEMVER: $SEMVER"
	echo "VERSION: $VERSION"
	echo "VER: $VER"

	echo "SSL_VERSION: $SSL_VERSION"
	echo "MAC_DEP_TARGET: $MAC_DEP_TARGET"
	
	echo "URL_SRCPYTHON: $URL_SRCPYTHON"
	echo "URL_OPENSSL: $URL_OPENSSL"
	echo "URL_GETPIP: $URL_GETPIP"
	
	echo "ROOT: $ROOT"
	echo "SUPPORT: $SUPPORT"
	echo "SOURCE: $SOURCE"
	echo "PY: $PY"
	echo "TARGETS: $TARGETS"
	echo "TARGET: $TARGET"
	echo "BUILD: $BUILD"
	echo "PYTHON: $PYTHON"
	echo "PREFIX: $PREFIX"
	echo "BIN: $BIN"
	echo "LIB: $LIB"
	echo "SSL_SRC: $SSL_SRC"
	echo "SSL: $SSL"
	echo "TMP: $TMP"
}


get_url() {
	mkdir -p $TMP
	fname=$(basename $1)
	curl $1 -o $TMP/$fname
	tar -C $BUILD -xvf $TMP/$fname
	rm -rf $TMP
}

get_python() {
	get_url $URL_PYTHON
}


get_ssl() {
	get_url $URL_OPENSSL
}

reset_python() {
	remove $PYTHON
}

reset_support() {
	remove $PREFIX
}

reset_ssl() {
	remove $SSL_SRC
	remove $SSL
}

reset() {
	reset_python
	reset_support
	reset_ssl
}

remove() {
	echo "removing $1"
	rm -rf $1
}

rm_lib() {
	echo "removing $1"
	rm -rf ${LIB}/$1
}


rm_ext() {
	echo "removing $LIB/lib-dynload/$1.cpython-${VER}m-darwin.so"
	rm -rf $LIB/lib-dynload/$1.cpython-${VER}-darwin.so
}

rm_bin() {
	echo "removing $PREFIX/bin/$1"
	rm -rf $PREFIX/bin/$1
}


clean_python_cruft() {
	find . | grep -E "(__pycache__|\.pyc|\.pyo$)" | xargs rm -rf
	find . | grep -E "(tests|test)" | xargs rm -rf
}

clean_python_pyc() {
	echo "removing __pycache__ .pyc/o from $1"
	find $1 | grep -E "(__pycache__|\.pyc|\.pyo$)" | xargs rm -rf
}

clean_python_tests() {
	echo "removing 'test' dirs from $1"
	find $1 | grep -E "(tests|test)" | xargs rm -rf
}

clean_python_site_packages() {
	echo "removing everything in $LIB/site-packages"
	rm -rf $LIB/site-packages/*
}

clean_python() {
	clean_python_pyc $PREFIX
	clean_python_tests $LIB
	clean_python_site_packages

	remove $LIB/distutils/command/*.exe
	remove $PREFIX/lib/pkgconfig
	remove $PREFIX/share

	rm_lib config-${VERSION}m-darwin
	rm_lib idlelib
	rm_lib lib2to3
	rm_lib tkinter
	rm_lib turtledemo
	rm_lib turtle.py
	rm_lib ctypes
	rm_lib curses
	rm_lib ensurepip
	rm_lib venv

	rm_ext _tkinter
	rm_ext _ctypes

	rm_ext _multibytecodec
	rm_ext _codecs_jp
	rm_ext _codecs_hk
	rm_ext _codecs_cn
	rm_ext _codecs_kr
	rm_ext _codecs_tw
	rm_ext _codecs_iso2022
	rm_ext _curses
	rm_ext _curses_panel

	rm_bin 2to3-${VERSION}
	rm_bin idle${VERSION}
	rm_bin easy_install-${VERSION}
	rm_bin pip${VERSION}
	rm_bin python${VERSION}m
	rm_bin pyvenv-${VERSION}
	rm_bin pydoc${VERSION}
}

zip_python_library() {
	remove ${LIB}/site-packages
	mv $LIB/lib-dynload $PREFIX
	cp $LIB/os.py $PREFIX
	python -m zipfile -c $PREFIX/lib/python${VER}.zip $LIB/*
	remove $LIB
	mkdir -p $LIB
	mv $PREFIX/lib-dynload $LIB
	 # Need to have a copy of os.py to remain in site-packages
	 # or it will fail to pick up library.zip
	mv $PREFIX/os.py $LIB
	mkdir $LIB/site-packages
}


build_ssl() {
	cd $SSL_SRC
	make clean
	# ./config no-shared --prefix=$SSL
	./config shared --prefix=$SSL
	make install_sw
	cd $ROOT
}


write_python_minim_setup_local() {

FILE="${PYTHON}/Modules/Setup.local"

/bin/cat <<EOM >$FILE

*disabled*
_ctypes
#_sqlite3
_tkinter 
_curses
_curses_panel
#pyexpat
#_elementtree
xxlimited
xxsubtype
#unicodedata
_multibytecodec
_codecs_jp 
_codecs_kr 
_codecs_tw 
_codecs_cn
_codecs_hk
_codecs_iso2022
EOM

}

write_python_getpip() {
FILE="${PREFIX}/bin/get_pip.sh"
/bin/cat <<EOM >$FILE
curl ${URL_GETPIP} -s -o get-pip.py 
./bin/python3.7 get-pip.py
rm get-pip.py
EOM
chmod +x ${PREFIX}/bin/get_pip.sh
}

reset_prefix() {
	remove $PREFIX
}

compile_python() {
	cd $PYTHON
	write_python_minim_setup_local
	make clean
	configure_python
	make altinstall
	cd $ROOT
}


build_python() {
	compile_python
	clean_python
	write_python_getpip
}

build_python_zipped() {
	build_python
	zip_python_library
}


fix_python_dylib_for_pkg() {
	cd $PREFIX/lib
	chmod 777 ${DYLIB}
	# assumes python in installed in $PREFIX
	# ../../../../support/python3.7/lib/libpython3.7m.dylib
	install_name_tool -id @loader_path/../../../../support/${NAME}/lib/${DYLIB} ${DYLIB}
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


# FIXME: not complete!
fix_python_libintl() {
	#otool -L $PREFIX/lib/libpython${VERSION}.dylib
	cp /usr/local/opt/gettext/lib/libintl.8.dylib ${PREFIX}/lib
	chmod 777 ${PREFIX}/lib/libintl.8.dylib
	install_name_tool -id @executable_path/libintl.8.dylib ${PREFIX}/lib/libintl.8.dylib
	install_name_tool -change /usr/local/opt/gettext/lib/libintl.8.dylib @executable_path/libintl.8.dylib libpython${VERSION}.dylib
}


install_python() {
	echo "checking if previous build exists"
	mkdir -p $BUILD
	if [ ! -d $PYTHON ] ; then
		echo "retrieving $PYTHON from $URL_PYTHON"
		get_python
	fi
	if [ ! -d $SSL ] ; then
		echo "retrieving $SSL from $URL_OPENSSL"
		get_ssl
		build_ssl
	fi
	build_python_zipped
}

install_python_pkg() {
	install_python
	fix_python_dylib_for_pkg
}

install_python_ext() {
	install_python
	fix_python_dylib_for_ext
}

usage() {
    echo "No argument given. Can be 'pkg' or 'ext'"
    echo "for package or external installation respectively"
}
