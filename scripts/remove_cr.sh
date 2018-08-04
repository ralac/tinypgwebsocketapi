#! /bin/sh
THIS_FILE="${BASH_SOURCE[0]}"
RELDIR="$(dirname $THIS_FILE)"
PARENT_DIR="$(cd $RELDIR/.. > /dev/null && pwd)"

find "$PARENT_DIR"/src -name \*.c -exec sed -i ':a;N;$!ba;s/\r//g' {} \;
find "$PARENT_DIR"/src -name \*.py -exec sed -i ':a;N;$!ba;s/\r//g' {} \;
find "$PARENT_DIR"/src -name \*.sh -exec sed -i ':a;N;$!ba;s/\r//g' {} \;
find "$PARENT_DIR"/src -name \*.html -exec sed -i ':a;N;$!ba;s/\r/\n/g' {} \;
find "$PARENT_DIR"/scripts -name \*.sh -exec sed -i ':a;N;$!ba;s/\r//g' {} \;
find "$PARENT_DIR"/examples -name \*.sql -exec sed -i ':a;N;$!ba;s/\r//g' {} \;
