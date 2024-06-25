# web.mk
# Packs the web runtime and wrapper.

web_JSFILES:=$(filter src/www/%.js,$(SRCFILES))

web_BUNDLE_TEMPLATE:=$(web_OUTDIR)/bundle-template.html
web_BUNDLE_INPUTS:=$(filter-out %Rom.js,$(web_JSFILES))
$(web_BUNDLE_TEMPLATE):$(web_BUNDLE_INPUTS) $(tools_EGGDEV_EXE);$(PRECMD) $(tools_EGGDEV_EXE) webtemplate -o$@ $(web_BUNDLE_INPUTS)
all-tools:$(web_BUNDLE_TEMPLATE)

web-serve:all-tools demo-trial;$(tools_EGGDEV_EXE) serve --htdocs=src/www out/demo/trial.egg

web-edit:all-tools demo-trial;$(tools_EGGDEV_EXE) serve \
  --htdocs=src/editor \
  --override=src/demo/trial/editor \
  --runtime=src/www \
  --data=src/demo/trial/data \
  --types=src/demo/trial/restypes \
  out/demo/trial.egg
