#!/bin/sh

if [ "$#" -ne 1 ] ; then
  echo "Usage: $0 OUTPUT"
  exit 1
fi

DSTPATH="$1"

cat - >"$DSTPATH" <<EOF
#!/bin/sh
if [ "\$#" -ne 2 ] ; then
  echo "Usage: \$0 OUTPUT ROMFILE"
  exit 1
fi
TMPASM=tmp-egg-rom.s
cat - >"\$TMPASM" <<EOFASM
.globl egg_rom_bundled
.globl egg_rom_bundled_length
egg_rom_bundled: .incbin "\$2"
egg_rom_bundled_length: .int (egg_rom_bundled_length-egg_rom_bundled)
EOFASM
$LD -o\$1 \$TMPASM $LDPOST || (
  rm \$TMPASM
  exit 1
)
rm \$TMPASM
EOF

chmod +x "$DSTPATH"
