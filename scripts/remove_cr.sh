#! /bin/sh
THIS_FILE="${BASH_SOURCE[0]}"
RELDIR="$(dirname $THIS_FILE)"
PARENT_DIR="$(cd $RELDIR/.. > /dev/null && pwd)"
find "$PARENT_DIR"/src -name *.c -exec sed ':a;N;$!ba;s/\n/ /g' {}\;
find "$PARENT_DIR"/src -name *.py -exec sed ':a;N;$!ba;s/\n/ /g' {}\;
find "$PARENT_DIR"/scripts -name *.sh -exec sed ':a;N;$!ba;s/\n/ /g' {}\;
