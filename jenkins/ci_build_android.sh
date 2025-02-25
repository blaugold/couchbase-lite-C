#!/bin/bash -ex

# NOTE: This is for Couchbase internal CI usage.  
# This room is full of dragons, so you *will* get confused.  
# You have been warned.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

ANDROID_NDK_VERSION="21.4.7075529"
ANDROID_CMAKE_VERSION="3.23.0"
ANDROID_NINJA_VERSION="1.10.2"
PKG_TYPE="zip"
PKG_CMD="zip -r"

function usage() {
    echo "Usage: $0 <version> <build num> <edition>"
    echo "Required env: ANDROID_HOME"
    echo "Optional env: WORKSPACE"
    exit 1
}

if [ "$#" -ne 3 ]; then
    usage
fi

if [ -z "$WORKSPACE" ]; then
    WORKSPACE=$(pwd)
fi

if [ -z "$ANDROID_HOME" ]; then
    usage
fi

VERSION="$1"
if [ -z "$VERSION" ]; then
    usage
fi

BLD_NUM="$2"
if [ -z "$BLD_NUM" ]; then
    usage
fi

EDITION="$3"
if [ -z "$EDITION" ]; then
    usage
fi

SDK_MGR="${ANDROID_HOME}/cmdline-tools/latest/bin/sdkmanager"

echo " ======== Installing toolchain with NDK ${ANDROID_NDK_VERSION} (this will accept the licenses!)"
yes | ${SDK_MGR} --licenses > /dev/null 2>&1
${SDK_MGR} --install "ndk;${ANDROID_NDK_VERSION}" > /dev/null

echo " ======== Installing cbdeps ========"
mkdir -p .tools
if [ ! -f .tools/cbdep ]; then 
    curl -o .tools/cbdep http://downloads.build.couchbase.com/cbdep/cbdep.$(uname -s | tr "[:upper:]" "[:lower:]")-$(uname -m)
    chmod +x .tools/cbdep
fi 

CMAKE="$(pwd)/.tools/cmake-${ANDROID_CMAKE_VERSION}/bin/cmake"
NINJA="$(pwd)/.tools/ninja-${ANDROID_NINJA_VERSION}/bin/ninja"
if [ ! -f ${CMAKE} ]; then
    .tools/cbdep install -d .tools cmake ${ANDROID_CMAKE_VERSION}
fi

if [ ! -f ${NINJA} ]; then
    .tools/cbdep install -d .tools ninja ${ANDROID_NINJA_VERSION}
fi

function build_variant {
    ${CMAKE} -G Ninja \
	    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_HOME/ndk/$ANDROID_NDK_VERSION/build/cmake/android.toolchain.cmake \
	    -DCMAKE_MAKE_PROGRAM=${NINJA} \
	    -DANDROID_NATIVE_API_LEVEL=22 \
	    -DANDROID_ABI=$1 \
	    -DCMAKE_INSTALL_PREFIX=$(pwd)/../libcblite-$VERSION \
        -DEDITION=$EDITION \
	    ..

    ${NINJA} install/strip
}

if [ "$EDITION" = "enterprise" ]; then
    ln -sf ${WORKSPACE}/couchbase-lite-c-ee/couchbase-lite-core-EE ${WORKSPACE}/couchbase-lite-c/vendor/couchbase-lite-core-EE
fi

set -x
mkdir -p build_android_x86
cd build_android_x86
build_variant x86

mkdir -p ../build_android_arm
cd ../build_android_arm
build_variant armeabi-v7a

mkdir -p ../build_android_x64
cd ../build_android_x64
build_variant x86_64

mkdir -p ../build_android_arm64
cd ../build_android_arm64
build_variant arm64-v8a

PACKAGE_NAME="couchbase-lite-c-${EDITION}-${VERSION}-${BLD_NUM}-android.${PKG_TYPE}"
echo
echo "=== Creating ${WORKSPACE}/${PACKAGE_NAME}"
echo

cd $(pwd)/..
cp ${WORKSPACE}/product-texts/mobile/couchbase-lite/license/LICENSE_$EDITION.txt libcblite-$VERSION/LICENSE.txt
#notices.txt is produced by blackduck.
#It is not part of source tar, it is download to the workspace by a separate curl command by jenkins job.
if [[ -f ${WORKSPACE}/notices.txt ]]; then
    cp ${WORKSPACE}/notices.txt libcblite-$VERSION/notices.txt
fi
${PKG_CMD} ${WORKSPACE}/${PACKAGE_NAME} libcblite-$VERSION

cd ${WORKSPACE}
PROP_FILE="${WORKSPACE}/publish_android.prop"
echo "PRODUCT=couchbase-lite-c" > ${PROP_FILE}
echo "BLD_NUM=${BLD_NUM}" >> ${PROP_FILE}
echo "VERSION=${VERSION}" >> ${PROP_FILE}
echo "PKG_TYPE=${PKG_TYPE}" >> ${PROP_FILE}
echo "RELEASE_PKG_NAME=${PACKAGE_NAME}" >> ${PROP_FILE}

cat ${PROP_FILE}
