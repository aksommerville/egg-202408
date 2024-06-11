# demo.mk
# Builds some demo projects under src/demo, which serve as a guide for building Egg games.

demo_DEMOS:=$(notdir $(wildcard src/demo/*))
define demo_RULES
  demo-$1:all-tools;OUTDIR=../../../$(demo_OUTDIR) MIDDIR=../../../$(demo_MIDDIR)/$1 make --no-print-directory -Csrc/demo/$1
  all-demos:demo-$1
  all:demo-$1
endef
$(foreach D,$(demo_DEMOS),$(eval $(call demo_RULES,$D)))
