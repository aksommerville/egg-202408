#!/bin/sh

if [ "$#" -lt 1 ] ; then
  echo "Usage: $0 OUTPUT [INPUT...]"
  exit 1
fi

DSTPATH="$1"
shift 1
rm -f "$DSTPATH"

cat - >"$DSTPATH" <<EOF
<!DOCTYPE html>
<html>
<head>
<style>
canvas {
INSERT CANVAS STYLE HERE
}
</style>
<egg-rom style="display:none">
INSERT ROM HERE
</egg-rom>
<script>
EOF

while [ "$#" -gt 0 ] ; do
  SRCPATH="$1"
  shift 1
  sed -E 's/^import.*$//;s/^export //;' "$SRCPATH" >>"$DSTPATH"
done

cat - >>"$DSTPATH" <<EOF
</script>
</head>
<body>
</body>
</html>
EOF
