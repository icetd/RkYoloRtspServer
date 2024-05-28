#! /usr/bin/bash
CURRENT_DIR=$(pwd)
MPP_VERSION=1.0.5
MPP_SRC=mpp-${MPP_VERSION}
MPP_SRC_DIR=${CURRENT_DIR}/thirdparty/${MPP_SRC}
MPP_URL=https://github.com/rockchip-linux/mpp/archive/refs/tags/${MPP_VERSION}.tar.gz
MPP_PREFIX=${CURRENT_DIR}/thirdparty/mpp

cd ${CURRENT_DIR}/thirdparty

if [ ! -d "${MPP_PREFIX}" ]
then
	(wget -O "${MPP_SRC}.tar" ${MPP_URL} \
		&& tar -xf "${MPP_SRC}.tar") || exit
	rm -f "${MPP_SRC}.tar"
else
	echo "already build mpp"
fi

if [ ! -d "${MPP_PREFIX}" ]
then
	cd ${MPP_SRC_DIR}
	(cmake -DCMAKE_INSTALL_PREFIX=${MPP_PREFIX} .) || exit
	(make -j4 && make install) || exit
	cd ${CURRENT_DIR}
	(rm -rf "${MPP_SRC_DIR}") || exit
else
	echo "already build mpp"
fi
