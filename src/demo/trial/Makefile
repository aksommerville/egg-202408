all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

# When building from the Egg root, these get provided for us.
ifndef MIDDIR
  MIDDIR:=mid
endif
ifndef OUTDIR
  OUTDIR:=out
endif

EGGINC:=../..
EGGDEV:=../../../out/tools/eggdev
EGGEXE:=../../../out/linux/egg
WASMLD:=$(firstword $(shell which wasm-ld) $(shell which wasm-ld-11))

CC:=clang --target=wasm32 -c -O3 -MMD -nostdlib -I$(EGGINC) -Wno-comment
LD:=$(WASMLD) --no-entry -z stack-size=4194304 --no-gc-sections --allow-undefined --export-table \
  --export=egg_client_init --export=egg_client_quit --export=egg_client_update --export=egg_client_render

$(MIDDIR)/%.o:src/%.c;$(PRECMD) $(CC) -o$@ $<

DATAFILES:=$(shell find data -type f)
CFILES:=$(shell find src -name '*.c')
OFILES:=$(patsubst src/%.c,$(MIDDIR)/%.o,$(CFILES))
-include $(OFILES:.o=.d)

WASM1:=$(MIDDIR)/data/wasm/1
$(WASM1):$(OFILES);$(PRECMD) $(LD) -o$@ $^

ROM:=$(OUTDIR)/trial.egg
$(ROM):$(WASM1) $(DATAFILES) $(EGGDEV);$(PRECMD) $(EGGDEV) pack -o$@ $(WASM1) data --types=restypes
all:$(ROM)

run:$(ROM);$(EGGEXE) $(ROM)
clean:;rm -rf $(MIDDIR) $(OUTDIR)

#------------------------------------
# Optionally bundle as a fake-native executable and a single-page web app.

FAKENATIVE:=$(OUTDIR)/trial-fake.exe
$(FAKENATIVE):$(ROM);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$<
all:$(FAKENATIVE)

# Note that WEBAPP is exactly the same command as FAKENATIVE: The key thing is its output path ends ".html"
# I'm actively developing bundle-template.html as I write this... In real life, once egg is stable you shouldn't need that prereq.
WEBAPP:=$(OUTDIR)/trial.html
$(WEBAPP):$(ROM);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$<
all:$(WEBAPP)

#-----------------------------------------
# Even optionallier, build the code natively and bundle as a true-native executable.

CC_NATIVE:=gcc -c -MMD -O3 -I$(EGGINC)
AR_NATIVE:=ar rc

$(MIDDIR)/native/%.o:src/%.c;$(PRECMD) $(CC_NATIVE) -o$@ $<

TRUENATIVE:=$(OUTDIR)/trial-true.exe
OFILES_NATIVE:=$(patsubst src/%.c,$(MIDDIR)/native/%.o,$(CFILES))
-include $(OFILES_NATIVE:.o=.d)
NATIVE_LIB:=$(MIDDIR)/native.a
$(NATIVE_LIB):$(OFILES_NATIVE);$(PRECMD) $(AR_NATIVE) $@ $^
$(TRUENATIVE):$(ROM) $(NATIVE_LIB);$(PRECMD) $(EGGDEV) bundle -o$@ --rom=$(ROM) --code=$(NATIVE_LIB)
all:$(TRUENATIVE)
