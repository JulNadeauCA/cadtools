#!/bin/sh
# Public domain

PROJ=cadtools
PROJNAME=CADTools
MAILLIST_EN=${PROJ}-announce@lists.csoft.org
MAILLIST_FR=${PROJ}-announce-fr@lists.csoft.org
VER=`perl mk/get-version.pl`
REL=`perl mk/get-release.pl`
DISTFILE=${PROJ}-${VER}
HOST=resin.csoft.net
RUSER=vedge
MAILER="sendmail -t"
HOMEPAGE=http://hypertriton.com/cadtools

if [ "$1" = "snapshot" ]; then
	PHASE=beta
else
	PHASE=stable
fi

echo "Packaging ${PROJ}-${VER} (${REL})"

cd ..
rm -fr ${PROJ}-${VER}
cp -fRp ${PROJ} ${PROJ}-${VER}
rm -fR `find ${PROJ}-${VER} \( -name .svn \
    -or -name \*~ \
    -or -name \*.o \
    -or -name \*.a \
    -or -name \*.core \
    -or -name .\*.swp \
    -or -name .depend \
    -or -name .xvpics \)`

tar -f ${DISTFILE}.tar -c ${PROJ}-${VER}
gzip -f ${DISTFILE}.tar
zip -r ${DISTFILE}.zip ${PROJ}-${VER}

openssl md5 ${DISTFILE}.tar.gz > ${DISTFILE}.tar.gz.md5
openssl rmd160 ${DISTFILE}.tar.gz >> ${DISTFILE}.tar.gz.md5
openssl sha1 ${DISTFILE}.tar.gz >> ${DISTFILE}.tar.gz.md5
openssl md5 ${DISTFILE}.zip > ${DISTFILE}.zip.md5
openssl rmd160 ${DISTFILE}.zip >> ${DISTFILE}.zip.md5
openssl sha1 ${DISTFILE}.zip >> ${DISTFILE}.zip.md5
gpg -ab ${DISTFILE}.tar.gz
gpg -ab ${DISTFILE}.zip

if [ "$1" = "commit" -o "$1" = "snapshot" ]; then
	echo "uploading"
	scp -C ${DISTFILE}.{tar.gz,tar.gz.md5,tar.gz.asc,zip,zip.md5,zip.asc} ${RUSER}@${HOST}:www/$PHASE.csoft.org/${PROJ}

	echo "notifying ${PROJ}-announce@"
	TMP=`mktemp /tmp/${PROJ}announceXXXXXXXX`
	cat > $TMP << EOF
From: Julien Nadeau <vedge@hypertriton.com>
To: ${MAILLIST_EN}
Subject: ${PROJNAME}-${VER} (${REL}) released
X-Mailer: dist.sh
X-PGP-Key: 206C63E6

We are pleased to announce the official release of ${PROJNAME} ${VER}
(${REL}).

It is now available for download from ${PHASE}.csoft.org.

	http://${PHASE}.csoft.org/${PROJ}/${PROJ}-${VER}.tar.gz
	http://${PHASE}.csoft.org/${PROJ}/${PROJ}-${VER}.tar.gz.asc
	http://${PHASE}.csoft.org/${PROJ}/${PROJ}-${VER}.tar.gz.md5
	http://${PHASE}.csoft.org/${PROJ}/${PROJ}-${VER}.zip
	http://${PHASE}.csoft.org/${PROJ}/${PROJ}-${VER}.zip.asc
	http://${PHASE}.csoft.org/${PROJ}/${PROJ}-${VER}.zip.md5

Binary packages are also available from the ${PROJNAME} website:

	${HOMEPAGE}/download.html

Your comments, suggestions and bug reports are most welcome.

EOF
	if [ -e "Release-${VER}.txt" ]; then
		echo "appending release notes"
		cat Release-${VER}.txt >> $TMP;
	fi
	cat $TMP | ${MAILER}

	echo "notifying ${PROJ}-announce-fr@"
	TMP=`mktemp /tmp/${PROJ}announceXXXXXXXX`
	cat > $TMP << EOF
From: Julien Nadeau <vedge@hypertriton.com>
To: ${MAILLIST_FR}
Subject: Nouvelle version: ${PROJNAME} ${VER} (${REL})
X-Mailer: dist.sh
X-PGP-Key: 206C63E6

Il me fait plaisir d'annoncer la sortie officielle de ${PROJNAME} ${VER}
(${REL}).

La distribution source est téléchargable à partir de ${PHASE}.csoft.org:

	http://$PHASE.csoft.org/${PROJ}/${PROJ}-${VER}.tar.gz
	http://$PHASE.csoft.org/${PROJ}/${PROJ}-${VER}.tar.gz.asc
	http://$PHASE.csoft.org/${PROJ}/${PROJ}-${VER}.tar.gz.md5
	http://$PHASE.csoft.org/${PROJ}/${PROJ}-${VER}.zip
	http://$PHASE.csoft.org/${PROJ}/${PROJ}-${VER}.zip.asc
	http://$PHASE.csoft.org/${PROJ}/${PROJ}-${VER}.zip.md5

Des paquets binaires sont également disponibles sur le site de ${PROJNAME}:

	${HOMEPAGE}/download.html.fr

Vos commentaires, suggestions et signalements de bogues sont, comme
toujours, fortement appréciés.
EOF
	if [ -e "Release-${VER}.txt" ]; then
		echo "appending release notes"
		cat Release-${VER}.txt >> $TMP;
	fi
	cat $TMP | ${MAILER}
	rm -f $TMP
fi

echo "**************************************************"
echo "TODO: Update en.wikipedia.org/wiki/cadtools (software)"
echo "TODO: Update fr.wikipedia.org/wiki/cadtools (logiciel)"
echo "TODO: Update SourceForge, Freshmeat"
echo "**************************************************"

