# web.mk
# Packs the web runtime and wrapper.

web_JSFILES:=$(filter src/www/%.js,$(SRCFILES))

#TODO Minify JS first.
web_BUNDLE_TEMPLATE:=$(web_OUTDIR)/bundle-template.html
$(web_BUNDLE_TEMPLATE):$(filter-out %Rom.js,$(web_JSFILES));$(PRECMD) etc/tool/gen-bundle-template.sh $@ $^
all-tools:$(web_BUNDLE_TEMPLATE)

web-serve:all-tools demo-trial;$(tools_EGGDEV_EXE) serve --htdocs=src/www out/demo/trial.egg

web-edit:all-tools demo-trial;$(tools_EGGDEV_EXE) serve \
  --htdocs=src/editor \
  --override=src/demo/trial/editor \
  --runtime=src/www \
  --data=src/demo/trial/data \
  --types=src/demo/trial/restypes \
  out/demo/trial.egg
