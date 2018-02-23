#!/bin/sh

TOOLDIR="$(dirname "$0")"
DESTDIR="$2"
[ -d "$2" ] || mkdir -p "$2"
rm -f "$2"/*

for md in "$1"/*.md; do
    md="$(basename "${md}")"
    TITLE="${md%%.md}"
    OUTHTML="${DESTDIR}/${TITLE}.html"
    echo "${md}"

    cat > "${OUTHTML}" <<FFXX
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
    <title>CLRadeonExtender Manual - ${TITLE}</title>
    <link rel="stylesheet" type="text/css" href="styles.css"/>
</head>
<body style="background: white;">
FFXX
    cat "$1/${md}" | python "${TOOLDIR}"/PrepareOffline.py | \
    markdown_py -x tables >> "${OUTHTML}"
    cat >> "${OUTHTML}" <<FFXX
</body>
</html>
FFXX
done

mv "${DESTDIR}/ClrxToc.html" "${DESTDIR}/index.html"
cp "$1/styles.css" "${DESTDIR}/styles.css"
