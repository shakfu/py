#!/usr/bin/env bash

source "scripts/build.sh"

PREFIX=${SUPPORT}/${PYTHON_NAME}
BIN=${PREFIX}/bin
LIB=${PREFIX}/lib/python${PYTHON_VER}


install_python_pkg() {
	install_dependencies
	build_python_static $PREFIX
	fix_python_dylib_for_pkg
}

install_python_ext() {
	install_dependencies
	build_python_static $PREFIX
	fix_python_dylib_for_ext
}

fix_python_dylib_for_pkg() {
	cd $PREFIX/lib
	chmod 777 ${DYLIB}
	install_name_tool -id @loader_path/../../../../support/${PYTHON_NAME}/lib/${DYLIB} ${DYLIB}
	cd $ROOT
}

fix_python_dylib_for_ext() {
	cd $PREFIX/lib
	chmod 777 ${DYLIB}
	install_name_tool -id @loader_path/${DYLIB} ${DYLIB}
	cd $ROOT
}

reset() {
	rm -rf $PREFIX
}


if [ "$1" == "pkg" ]; then
	echo "Installing python from source as static lib into 'support' folder of package"
	install_python_pkg

elif [ "$1" == "ext" ]; then
	echo "Installing python from source as static lib into external"
	install_python_ext
else
	exit
fi