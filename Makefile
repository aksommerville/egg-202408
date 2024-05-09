all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

ifneq (,$(strip $(filter clean,$(MAKECMDGOALS))))

clean:;rm -rf mid out

else

include etc/config.mk
etc/config.mk:|etc/config.mk.default;$(PRECMD) cp etc/config.mk.default $@ ; echo "Generated $@. Please edit, then rerun make." ; exit 1

SRCFILES:=$(shell find src -type f)

define TARGET_RULES
  $1_MIDDIR:=mid/$1
  $1_OUTDIR:=out/$1
  include etc/target/$1.mk
endef
$(foreach T,$(TARGETS),$(eval $(call TARGET_RULES,$T)))

endif
