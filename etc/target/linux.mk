linux_OPT_ENABLE+=fs rom serial wamr strfmt png render midi sfg

linux_CC:=$(linux_TOOLCHAIN)gcc -c -MMD -O3 -Isrc -Werror -Wimplicit $(linux_CC_EXTRA) \
  $(patsubst %,-DUSE_%=1,$(tools_OPT_ENABLE)) \
  -I$(WAMR_SDK)/core/iwasm/include
linux_LD:=$(linux_TOOLCHAIN)gcc
linux_LDPOST:=$(linux_LD_EXTRA) $(abspath $(WAMR_SDK)/build/libvmlib.a) -lm -lz -lGL -lEGL
linux_AR:=$(linux_TOOLCHAIN)ar rc

$(linux_MIDDIR)/%.o:src/%.c;$(PRECMD) $(linux_CC) -o$@ $<
$(linux_MIDDIR)/runner/egg_romsrc_external.o:src/runner/egg_romsrc_external.c;$(PRECMD) $(linux_CC) -o$@ $< -DEGG_BUNDLE_ROM=0
$(linux_MIDDIR)/runner/egg_romsrc_bundled.o:src/runner/egg_romsrc_external.c;$(PRECMD) $(linux_CC) -o$@ $< -DEGG_BUNDLE_ROM=1

linux_CFILES_ALL:=$(filter \
  $(addprefix src/opt/,$(addsuffix /%.c,$(linux_OPT_ENABLE))) \
  src/runner/%.c \
,$(SRCFILES))

linux_OFILES_ALL:=$(patsubst src/%.c,$(linux_MIDDIR)/%.o,$(linux_CFILES_ALL))
-include $(linux_OFILES_ALL:.o=.d)

# The main executable, suitable for running external Egg ROMs.
linux_EXE_OFILES:=$(filter-out $(linux_MIDDIR)/runner/egg_romsrc%,$(linux_OFILES_ALL)) $(linux_MIDDIR)/runner/egg_romsrc_external.o
linux_EXE:=$(linux_OUTDIR)/egg
$(linux_EXE):$(linux_EXE_OFILES);$(PRECMD) $(linux_LD) -o$@ $(linux_EXE_OFILES) $(linux_LDPOST)
all:$(linux_EXE)

# Static library, basically the same thing as EXE, but expects to link against an embedded ROM.
linux_LIBB_OFILES:=$(filter-out $(linux_MIDDIR)/runner/egg_romsrc%,$(linux_OFILES_ALL)) $(linux_MIDDIR)/runner/egg_romsrc_bundled.o
linux_LIBB:=$(linux_OUTDIR)/libegg-bundled.a
$(linux_LIBB):$(linux_LIBB_OFILES);$(PRECMD) $(linux_AR) $@ $(linux_LIBB_OFILES)
all:$(linux_LIBB)
all-tools:$(linux_LIBB)

# Static library, basically the same thing as EXE, but expects to link against an embedded ROM and native hooks for the client code.
linux_LIBN_OFILES:=$(filter-out \
  $(linux_MIDDIR)/runner/egg_romsrc% \
  $(linux_MIDDIR)/opt/wamr/% \
,$(linux_OFILES_ALL)) $(linux_MIDDIR)/runner/egg_romsrc_native.o
linux_LIBN:=$(linux_OUTDIR)/libegg-native.a
$(linux_LIBN):$(linux_LIBN_OFILES);$(PRECMD) $(linux_AR) $@ $(linux_LIBN_OFILES)
all:$(linux_LIBN)
all-tools:$(linux_LIBN)

# Generate scripts that will be invoked by `eggrom bundle`.
# TODO Should we make this bit switchable, in case there's a cross-compile host? Only one target should be doing this.
linux_SCRIPT_BUNDLED:=$(tools_OUTDIR)/egg-bundle.sh
$(linux_SCRIPT_BUNDLED):etc/tool/gen-egg-bundle.sh;$(PRECMD) LD="$(linux_LD)" LDPOST="$(abspath $(linux_LIBB)) $(linux_LDPOST)" etc/tool/gen-egg-bundle.sh $@
linux_SCRIPT_NATIVE:=$(tools_OUTDIR)/egg-native.sh
$(linux_SCRIPT_NATIVE):etc/tool/gen-egg-native.sh;$(PRECMD) LD="$(linux_LD)" LDPOST="$(abspath $(linux_LIBN)) $(linux_LDPOST)" etc/tool/gen-egg-native.sh $@
all:$(linux_SCRIPT_BUNDLED) $(linux_SCRIPT_NATIVE)
all-tools:$(linux_SCRIPT_BUNDLED) $(linux_SCRIPT_NATIVE)

linux-run:$(linux_EXE) demo-trial;$(linux_EXE) out/demo/trial.egg
