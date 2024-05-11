# tools.mk
# Should always be enabled.
# We build the 'eggdev' executable.
# For now, that's it. eggdev is a swiss army knife, it does really all of our "tool" things.

tools_OPT_ENABLE+=fs http rom serial png midi sfg

tools_CC:=$(tools_TOOLCHAIN)gcc -c -MMD -O3 -Isrc -Werror -Wimplicit $(tools_CC_EXTRA) \
  $(patsubst %,-DUSE_%=1,$(tools_OPT_ENABLE))
tools_LD:=$(tools_TOOLCHAIN)gcc
tools_LDPOST:=$(tools_LD_EXTRA) -lz -lm

ifneq (,$(strip $(WABT_SDK)))
  tools_CC+=-DWABT_SDK=\"$(abspath $(WABT_SDK))\"
endif

$(tools_MIDDIR)/%.o:src/%.c;$(PRECMD) $(tools_CC) -o$@ $<

tools_OPT_CFILES:=$(filter $(addprefix src/opt/,$(addsuffix /%.c,$(tools_OPT_ENABLE))),$(SRCFILES))
tools_OPT_OFILES:=$(patsubst src/%.c,$(tools_MIDDIR)/%.o,$(tools_OPT_CFILES))
-include $(tools_OPT_OFILES:.o=.d)

tools_EGGDEV_CFILES:=$(filter src/eggdev/%.c,$(SRCFILES))
tools_EGGDEV_OFILES:=$(patsubst src/%.c,$(tools_MIDDIR)/%.o,$(tools_EGGDEV_CFILES))
-include $(tools_EGGDEV_OFILES:.o=.d)
tools_EGGDEV_EXE:=$(tools_OUTDIR)/eggdev$(tools_EXE_SFX)
$(tools_EGGDEV_EXE):$(tools_EGGDEV_OFILES) $(tools_OPT_OFILES);$(PRECMD) $(tools_LD) -o$@ $(tools_EGGDEV_OFILES) $(tools_OPT_OFILES) $(tools_LDPOST)
all:$(tools_EGGDEV_EXE)
all-tools:$(tools_EGGDEV_EXE)
eggdev:$(tools_EGGDEV_EXE)

