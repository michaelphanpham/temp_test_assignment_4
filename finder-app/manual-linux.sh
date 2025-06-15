#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    # make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    # make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
cp -r ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log
#

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- distclean
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    # sed -i 's/# CONFIG_BASH is not set/CONFIG_BASH=y/' .config
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- CONFIG_PREFIX=${OUTDIR}/rootfs install
cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
INTERPRETER=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | awk '{print $4}' | tr -d '[]')
LIBS=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | awk '{print $5}' | tr -d '[]')

# Copy interpreter
echo "Copying interpreter: ${SYSROOT}${INTERPRETER} to rootfs"
cp ${SYSROOT}${INTERPRETER} ${OUTDIR}/rootfs/lib/

# Copy shared libraries
for lib in $LIBS; do
    echo "Copying library: ${SYSROOT}/lib64/$lib to rootfs"
    cp ${SYSROOT}/lib64/$lib ${OUTDIR}/rootfs/lib64/
done

# Make device nodes
cd ${OUTDIR}/rootfs
sudo mknod -m 622 dev/console c 5 1
sudo mknod -m 622 dev/null c 1 3
sudo mknod -m 622 dev/zero c 1 5
sudo mknod -m 622 dev/ptmx c 5 2
sudo mknod -m 622 dev/tty c 5 0
sudo mknod -m 444 dev/random c 1 8
sudo mknod -m 444 dev/urandom c 1 9
sudo mknod -m 666 dev/tty0 c 4 0
sudo mknod -m 666 dev/tty1 c 4 1
sudo mknod -m 666 dev/tty2 c 4 2
sudo mknod -m 666 dev/tty3 c 4 3
sudo mknod -m 666 dev/tty4 c 4 4

# TODO: Clean and build the writer utility
echo "Building the writer utility"
cd ${FINDER_APP_DIR}
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- clean
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
cp writer ${OUTDIR}/rootfs/home/

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copying the finder related scripts and executables to the /home directory"
cp finder.sh ${OUTDIR}/rootfs/home/
cp finder-test.sh ${OUTDIR}/rootfs/home/
cp -r ../conf ${OUTDIR}/rootfs/home/conf
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/

# TODO: Chown the root directory
echo "Chowning the root directory"
sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
echo "Creating initramfs.cpio.gz"
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
