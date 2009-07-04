#!/bin/bash
########################################################################
# A quick hack to generate a source dist file for QBoard.

VERSION=$(date +%Y%m%d)

DEST=libwhefs-${VERSION}
echo "Creating $DEST ..."


test -d ${DEST} && rm -fr ${DEST}
mkdir ${DEST}
test -d ${DEST} || {
    echo "Destination dir [${DEST}] could not be created!"
    exit 1
}
find . -name '*~' | xargs rm -f

sdirs="src include doc app"

find doc -type d -name 'libwhefs-API*' | xargs rm -fr

echo "Copying files..."
for d in $sdirs; do
    echo -e "\t... $d"
    find $d \
	-type f \
	-name '*.h' \
	-o -name '*.c' \
	-o -name '*.[hc]pp' \
	-o -name '*.sh' \
	-o -name 'Makefile' \
	-o -name 'Doxyfile' \
	-o -name 'Doxyfile-internal' \
	| sed -e '/\/bak/d' \
	-e '/kdbg/d' \
	-e '/\/nono/d' \
	-e '/\.#/d' \
	-e '/\.svn/d' \
	> filelist
    cp --parents $(cat filelist) $DEST || {
	err=$?
	echo "Copy failed!"
	rm filelist
	exit $err
    }
    rm filelist

    #echo cp --parents $(set -x; find $d $nameargs)
done

cp LICENSE \
    README \
    Makefile \
    createAmalgamation.sh \
    config.make \
    common.make \
    $DEST

echo "Kludging in static version number..."

find $DEST -name nono -o -name release -o -name debug | xargs rm -fr
find $DEST -name  debug -o -name release | xargs rm -fr

echo "Tarring...";
TF=$DEST.tar.bz2
tar cjf $TF $DEST
echo "Cleaning up..."
rm -fr $DEST

echo "Done:"
ls -l $TF
