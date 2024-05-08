all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

WAMR_SDK:=$(abspath ../thirdparty/wasm-micro-runtime)

CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -I$(WAMR_SDK)/core/iwasm/include
LD:=gcc
LDPOST:=$(WAMR_SDK)/build/libvmlib.a
AR:=ar rc

SRCFILES:=$(shell find src -type f)
CFILES:=$(filter %.c,$(SRCFILES))
OFILES:=$(patsubst src/%,mid/%,$(addsuffix .o,$(basename $(CFILES))))
-include $(OFILES:.o=.d)
mid/%.o:src/%.c;$(PRECMD) $(CC) -o$@ $<

EXE:=out/egg
all:$(EXE)
OFILES_EXE:=$(filter-out mid/eggdev/% mid/runner/egg_romsrc%,$(OFILES)) mid/runner/egg_romsrc_external.o
$(EXE):$(OFILES_EXE);$(PRECMD) $(LD) -o$@ $^ $(LDPOST)

LIB_BUNDLED:=out/libegg-bundled.a
all:$(LIB_BUNDLED)
OFILES_LIB_BUNDLED:=$(filter-out mid/eggdev/% mid/runner/egg_romsrc%,$(OFILES)) mid/runner/egg_romsrc_bundled.o
$(LIB_BUNDLED):$(OFILES_LIB_BUNDLED);$(PRECMD) $(AR) $@ $^

SCRIPT_BUNDLED:=out/egg-bundle.sh
all:$(SCRIPT_BUNDLED)
$(SCRIPT_BUNDLED):etc/tool/gen-egg-bundle.sh;$(PRECMD) LD="$(LD)" LDPOST="$(LDPOST) $(abspath $(LIB_BUNDLED))" etc/tool/gen-egg-bundle.sh $@

LIB_NATIVE:=out/libegg-native.a
all:$(LIB_NATIVE)
OFILES_LIB_NATIVE:=$(filter-out mid/eggdev/% mid/runner/egg_romsrc%,$(OFILES)) mid/runner/egg_romsrc_native.o
$(LIB_NATIVE):$(OFILES_LIB_NATIVE);$(PRECMD) $(AR) $@ $^

SCRIPT_NATIVE:=out/egg-native.sh
all:$(SCRIPT_NATIVE)
$(SCRIPT_NATIVE):etc/tool/gen-egg-native.sh;$(PRECMD) LD="$(LD)" LDPOST="$(LDPOST) $(abspath $(LIB_NATIVE))" etc/tool/gen-egg-native.sh $@

EXE_EGGDEV:=out/eggdev
all:$(EXE_EGGDEV)
OFILES_EXE_EGGDEV:=$(filter-out mid/runner/%,$(OFILES))
$(EXE_EGGDEV):$(OFILES_EXE_EGGDEV);$(PRECMD) $(LD) -o$@ $^ $(LDPOST)

#XXX TEMP Generate the "trial" ROM.
ROM_TRIAL:=out/trial.egg
all:$(ROM_TRIAL)
TRIAL_INPUTS:=$(filter src/demo/trial/%,$(SRCFILES))
$(ROM_TRIAL):$(EXE_EGGDEV) $(TRIAL_INPUTS);$(PRECMD) $(EXE_EGGDEV) pack -o$@ --types=src/demo/trial/restypes src/demo/trial
# Also generate it as a bundled executable:
BUNDLE_TRIAL:=out/trial.exe
all:$(BUNDLE_TRIAL)
$(BUNDLE_TRIAL):$(ROM_TRIAL) $(EXE_EGGDEV);$(PRECMD) $(EXE_EGGDEV) bundle -o$@ --rom=$(ROM_TRIAL)
# Could similarly generate a native executable by adding '--code=thegamecode.a' but we're not compiling that yet.

clean:;rm -rf mid out

run:$(EXE) $(ROM_TRIAL);$(EXE) $(ROM_TRIAL)
