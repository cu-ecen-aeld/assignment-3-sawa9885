#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -eu

OUTDIR=/tmp/aeld
KERNEL_REPO=https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath "$(dirname "$0")")
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
JOBS=$(nproc)

if [ "$#" -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

if ! mkdir -p "${OUTDIR}"; then
	echo "ERROR: Could not create output directory ${OUTDIR}" >&2
	exit 1
fi
OUTDIR=$(realpath "${OUTDIR}")
if [ "${OUTDIR}" = "/" ]; then
	echo "ERROR: Refusing to use the filesystem root as the output directory" >&2
	exit 1
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone "${KERNEL_REPO}" --depth 1 --single-branch --branch "${KERNEL_VERSION}" linux-stable
fi
if [ ! -e "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" ]; then
    cd "${OUTDIR}/linux-stable"
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout "${KERNEL_VERSION}"

    make ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" mrproper
    make ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" defconfig
    make -j"${JOBS}" ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" Image
fi

echo "Adding the Image in outdir"
cp "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" "${OUTDIR}/Image"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm -rf "${OUTDIR}/rootfs"
fi

mkdir -p "${OUTDIR}/rootfs"/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr/{bin,lib,lib64,sbin},var/{log,tmp}}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone https://git.busybox.net/busybox
    cd busybox
    git checkout "${BUSYBOX_VERSION}"
else
    cd "${OUTDIR}/busybox"
fi

make distclean
make ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" defconfig
make -j"${JOBS}" ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}"
make ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" CONFIG_PREFIX="${OUTDIR}/rootfs" install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a busybox | grep "Shared library"

SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -aL "${SYSROOT}/lib/." "${OUTDIR}/rootfs/lib/"
if [ -d "${SYSROOT}/lib64" ]; then
    cp -aL "${SYSROOT}/lib64/." "${OUTDIR}/rootfs/lib64/"
fi

sudo mknod -m 666 "${OUTDIR}/rootfs/dev/null" c 1 3
sudo mknod -m 600 "${OUTDIR}/rootfs/dev/console" c 5 1

make -C "${FINDER_APP_DIR}" clean
make -C "${FINDER_APP_DIR}" CROSS_COMPILE="${CROSS_COMPILE}"

cp "${FINDER_APP_DIR}/writer" \
   "${FINDER_APP_DIR}/finder.sh" \
   "${FINDER_APP_DIR}/finder-test.sh" \
   "${FINDER_APP_DIR}/autorun-qemu.sh" \
   "${OUTDIR}/rootfs/home/"
mkdir -p "${OUTDIR}/rootfs/home/conf"
cp "${FINDER_APP_DIR}/../conf/username.txt" \
   "${FINDER_APP_DIR}/../conf/assignment.txt" \
   "${OUTDIR}/rootfs/home/conf/"

sudo chown -R root:root "${OUTDIR}/rootfs"

echo "Creating initramfs.cpio.gz"
cd "${OUTDIR}/rootfs"
find . -print0 | cpio --null -ov --format=newc --owner=root:root | gzip -9 > "${OUTDIR}/initramfs.cpio.gz"
