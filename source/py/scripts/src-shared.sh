#!/usr/bin/env bash

source "scripts/build.sh"

PREFIX=${SUPPORT}/${PYTHON_NAME}
BIN=${PREFIX}/bin
LIB=${PREFIX}/lib/python${PYTHON_VER}


build_python_shared() {
	cd $BUILD_SRC_PY
	make clean
    write_setup_local setup-shared.local
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


install_python_pkg() {
	install_dependencies
	build_python_shared $PREFIX
	fix_python_dylib_for_pkg
}

install_python_ext() {
	install_dependencies
	build_python_shared $PREFIX
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
	# assumes cp -rf $PREFIX/* -> same directory as py extension in py.mxo
	install_name_tool -id @loader_path/${DYLIB} ${DYLIB}
	cd $ROOT
}

# FIXME: not complete!
fix_python_libintl() {
	#otool -L $PREFIX/lib/libpython${PYTHON_VER}.dylib
	cp /usr/local/opt/gettext/lib/libintl.8.dylib ${PREFIX}/lib
	chmod 777 ${PREFIX}/lib/libintl.8.dylib
	install_name_tool -id @executable_path/libintl.8.dylib ${PREFIX}/lib/libintl.8.dylib
	install_name_tool -change /usr/local/opt/gettext/lib/libintl.8.dylib @executable_path/libintl.8.dylib libpython${PYTHON_VER}.dylib
}

reset() {
	rm -rf $PREFIX
}


if [ "$1" == "pkg" ]; then
	echo "Installing python from source as shared into 'support' folder of package"
	install_python_pkg

elif [ "$1" == "ext" ]; then
	echo "Installing python from source as shared into external"
	install_python_ext
else
	exit
fi
