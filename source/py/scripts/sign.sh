COLOR_BOLD_YELLOW="\033[1;33m"
COLOR_BOLD_BLUE="\033[1;34m"
COLOR_BOLD_MAGENTA="\033[1;35m"
COLOR_BOLD_CYAN="\033[1;36m"
COLOR_RESET="\033[m"

function section {
	echo -e "${COLOR_BOLD_CYAN}>>> ${1} ${COLOR_RESET}"
}

VERSION=3.7
PY=${HOME}/Downloads/src/py
PY_EXT=${PY}/externals/py.mxo
PY_EXT_VERSION=${PY_EXT}/Contents/Resources/Python.Framework/Versions/${VERSION}
PYJS_EXT=${PY}/externals/pyjs.mxo
PYJS_EXT_VERSION=${PYJS_EXT}/Contents/Resources/Python.Framework/Versions/${VERSION}

function sign {
	section "codesign version $1"
	/usr/bin/codesign -s $DEV_ID --force --sign - --timestamp=none \
		--deep \
		--preserve-metadata=identifier,entitlements,flags $1/Contents/Resources/Python.Framework/Versions/${VERSION}

	/usr/bin/codesign -s $DEV_ID --force --deep $1

	section "codesign verify $1"
	codesign --verify --verbose $1
}

# section "codesign version ${PY_EXT_VERSIONS}/${VERSION}"
# /usr/bin/codesign -s $DEV_ID --force --sign - --timestamp=none \
# 	--deep \
# 	--preserve-metadata=identifier,entitlements,flags ${PY_EXT_VERSIONS}/${VERSION}


# /usr/bin/codesign -s $DEV_ID --force --deep ${PY_EXT}

# section "codesign verify ${PY_EXT}"
# codesign --verify --verbose ${PY_EXT}

sign ${PY_EXT}

sign ${PYJS_EXT}