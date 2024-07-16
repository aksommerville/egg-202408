macos_OPT_ENABLE+=fs rom serial wamr strfmt png render midi sfg hostio synth macos macwm machid 

macos_CC:=$(macos_TOOLCHAIN)gcc -c -MMD -O3 -Isrc -Werror -Wimplicit $(macos_CC_EXTRA) \
  $(patsubst %,-DUSE_%=1,$(macos_OPT_ENABLE)) \
  -I$(WAMR_SDK)/core/iwasm/include -Wno-parentheses -Wno-comment -Wno-pointer-sign -Wno-deprecated-declarations
macos_OBJC:=$(macos_CC) -xobjective-c
macos_LD:=$(macos_TOOLCHAIN)gcc
macos_LDPOST:=$(macos_LD_EXTRA) $(abspath $(WAMR_SDK)/build/libvmlib.a) -lm -lz \
  -framework Cocoa -framework Quartz -framework OpenGL -framework MetalKit -framework Metal -framework CoreGraphics -framework IOKit
macos_AR:=$(macos_TOOLCHAIN)ar rc

$(macos_MIDDIR)/%.o:src/%.c;$(PRECMD) $(macos_CC) -o$@ $<
$(macos_MIDDIR)/%.o:src/%.m;$(PRECMD) $(macos_OBJC) -o$@ $<
$(macos_MIDDIR)/runner/egg_romsrc_external.o:src/runner/egg_romsrc_external.c;$(PRECMD) $(macos_CC) -o$@ $< -DEGG_BUNDLE_ROM=0
$(macos_MIDDIR)/runner/egg_romsrc_bundled.o:src/runner/egg_romsrc_external.c;$(PRECMD) $(macos_CC) -o$@ $< -DEGG_BUNDLE_ROM=1

macos_CFILES_ALL:=$(filter %.c %.m,$(filter \
  $(addprefix src/opt/,$(addsuffix /%,$(macos_OPT_ENABLE))) \
  src/runner/% \
,$(SRCFILES)))

macos_OFILES_ALL:=$(patsubst src/%,$(macos_MIDDIR)/%.o,$(basename $(macos_CFILES_ALL)))
-include $(macos_OFILES_ALL:.o=.d)

macos_ICONS_DIR:=src/opt/macos/appicon.iconset
macos_XIB:=src/opt/macos/Main.xib
macos_PLIST_SRC:=src/opt/macos/Info.plist
macos_ICONS:=$(wildcard $(macos_ICONS_DIR)/*)

macos_BUNDLE:=$(macos_OUTDIR)/Egg.app
macos_PLIST:=$(macos_BUNDLE)/Contents/Info.plist
macos_NIB:=$(macos_BUNDLE)/Contents/Resources/Main.nib
macos_EXE:=$(macos_BUNDLE)/Contents/MacOS/egg
macos_ICON:=$(macos_BUNDLE)/Contents/Resources/appicon.icns

$(macos_NIB):$(macos_XIB);$(PRECMD) ibtool --compile $@ $<
$(macos_PLIST):$(macos_PLIST_SRC);$(PRECMD) cp $< $@
$(macos_ICON):$(macos_ICONS);$(PRECMD) iconutil -c icns -o $@ $(macos_ICONS_DIR)

# The main executable, suitable for running external Egg ROMs.
macos_EXE_OFILES:=$(filter-out $(macos_MIDDIR)/runner/egg_romsrc%,$(macos_OFILES_ALL)) $(macos_MIDDIR)/runner/egg_romsrc_external.o
$(macos_EXE):$(macos_EXE_OFILES);$(PRECMD) $(macos_LD) -o$@ $(macos_EXE_OFILES) $(macos_LDPOST)
macos-bundle-bits:$(macos_EXE) $(macos_ICON) $(macos_NIB) $(macos_PLIST)
all:macos-bundle-bits

# Static library, basically the same thing as EXE, but expects to link against an embedded ROM.
macos_LIBB_OFILES:=$(filter-out $(macos_MIDDIR)/runner/egg_romsrc%,$(macos_OFILES_ALL)) $(macos_MIDDIR)/runner/egg_romsrc_bundled.o
macos_LIBB:=$(macos_OUTDIR)/libegg-bundled.a
$(macos_LIBB):$(macos_LIBB_OFILES);$(PRECMD) $(macos_AR) $@ $(macos_LIBB_OFILES)
all:$(macos_LIBB)
all-tools:$(macos_LIBB)

# Static library, basically the same thing as EXE, but expects to link against an embedded ROM and native hooks for the client code.
macos_LIBN_OFILES:=$(filter-out \
  $(macos_MIDDIR)/runner/egg_romsrc% \
  $(macos_MIDDIR)/opt/wamr/% \
,$(macos_OFILES_ALL)) $(macos_MIDDIR)/runner/egg_romsrc_native.o
macos_LIBN:=$(macos_OUTDIR)/libegg-native.a
$(macos_LIBN):$(macos_LIBN_OFILES);$(PRECMD) $(macos_AR) $@ $(macos_LIBN_OFILES)
all:$(macos_LIBN)
all-tools:$(macos_LIBN)

# Generate scripts that will be invoked by `eggrom bundle`.
# TODO Should we make this bit switchable, in case there's a cross-compile host? Only one target should be doing this.
macos_SCRIPT_BUNDLED:=$(tools_OUTDIR)/egg-bundle.sh
$(macos_SCRIPT_BUNDLED):etc/tool/gen-egg-bundle.sh;$(PRECMD) LD="$(macos_LD)" LDPOST="$(abspath $(macos_LIBB)) $(macos_LDPOST)" etc/tool/gen-egg-bundle.sh $@
linux_SCRIPT_NATIVE:=$(tools_OUTDIR)/egg-native.sh
$(macos_SCRIPT_NATIVE):etc/tool/gen-egg-native.sh;$(PRECMD) LD="$(macos_LD)" LDPOST="$(abspath $(macos_LIBN)) $(macos_LDPOST)" etc/tool/gen-egg-native.sh $@
all:$(macos_SCRIPT_BUNDLED) $(macos_SCRIPT_NATIVE)
all-tools:$(macos_SCRIPT_BUNDLED) $(macos_SCRIPT_NATIVE)

macos-run:macos-bundle-bits;open -W $(macos_BUNDLE) --args rom/lightson.egg --chdir=$(PWD) --reopen-tty=$$(tty)
