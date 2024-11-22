IMPORT    = lrowac2isis
SPICEINIT = spiceinit
CROP      = crop
PROJECT   = cam2map
MOSAIC    = automos
INPUTFILE = wac0000a1c4

# These cubes, if there is a problem, will be
#   very significantly different
$(INPUTFILE).uv.lev2.cub.TOLERANCE = 0.0002
$(INPUTFILE).vis.lev2.cub.TOLERANCE = 0.0004

include $(ISISROOT)/make/isismake.tsts

# commands:
# 	$(IMPORT) $(TSTARGS) from=$(INPUT)/$(INPUTFILE).img \
# 	  to=$(OUTPUT)/$(INPUTFILE).cub \
# 	  > /dev/null;
# 	$(SPICEINIT) $(TSTARGS) shape=ellipsoid from=$(OUTPUT)/$(INPUTFILE).vis.even.cub > /dev/null;
# 	$(SPICEINIT) $(TSTARGS) shape=ellipsoid from=$(OUTPUT)/$(INPUTFILE).vis.odd.cub > /dev/null;
# 	$(SPICEINIT) $(TSTARGS) shape=ellipsoid from=$(OUTPUT)/$(INPUTFILE).uv.even.cub > /dev/null;
# 	$(SPICEINIT) $(TSTARGS) shape=ellipsoid from=$(OUTPUT)/$(INPUTFILE).uv.odd.cub > /dev/null;
# 	catlab $(TSTARGS) from=$(OUTPUT)/$(INPUTFILE).vis.even.cub > $(OUTPUT)/vislabel.pvl;
# 	catlab $(TSTARGS) from=$(OUTPUT)/$(INPUTFILE).uv.odd.cub > $(OUTPUT)/uvlabel.pvl;
# 	$(CROP) $(TSTARGS) from=$(OUTPUT)/$(INPUTFILE).vis.even.cub line=3000 nlines=200 \
# 	  sample=100 nsamples=200 \
# 	  TO=$(OUTPUT)/$(INPUTFILE).vis.even.crop.cub > /dev/null;
# 	$(MV) $(OUTPUT)/$(INPUTFILE).vis.even.crop.cub \
# 	  $(OUTPUT)/$(INPUTFILE).vis.even.cub;
# 	$(CROP) $(TSTARGS) from=$(OUTPUT)/$(INPUTFILE).vis.odd.cub line=3000 nlines=200 \
# 	  sample=100 nsamples=200 \
# 	  TO=$(OUTPUT)/$(INPUTFILE).vis.odd.crop.cub > /dev/null;
# 	$(MV) $(OUTPUT)/$(INPUTFILE).vis.odd.crop.cub \
# 	  $(OUTPUT)/$(INPUTFILE).vis.odd.cub;
# 	$(CROP) $(TSTARGS) from=$(OUTPUT)/$(INPUTFILE).uv.even.cub line=900 nlines=100 \
# 	  TO=$(OUTPUT)/$(INPUTFILE).uv.even.crop.cub > /dev/null;
# 	$(MV) $(OUTPUT)/$(INPUTFILE).uv.even.crop.cub \
# 	  $(OUTPUT)/$(INPUTFILE).uv.even.cub;
# 	$(CROP) $(TSTARGS) from=$(OUTPUT)/$(INPUTFILE).uv.odd.cub line=900 nlines=100 \
# 	  TO=$(OUTPUT)/$(INPUTFILE).uv.odd.crop.cub > /dev/null;
# 	$(MV) $(OUTPUT)/$(INPUTFILE).uv.odd.crop.cub \
# 	  $(OUTPUT)/$(INPUTFILE).uv.odd.cub;
# 	$(PROJECT) $(TSTARGS) FROM=$(OUTPUT)/$(INPUTFILE).vis.even.cub \
# 	  TO=$(OUTPUT)/$(INPUTFILE).vis.even.lev2.cub DEFAULTRANGE=CAMERA \
# 	  MINLAT=-74.026771144939 MAXLAT=-73.382294787899 \
# 	  MINLON=242.75004784633 MAXLON=244.11987169894 \
# 	  PIXRES=MPP RESOLUTION=52.8 \
# 	  > /dev/null;
# 	$(PROJECT) $(TSTARGS) FROM=$(OUTPUT)/$(INPUTFILE).vis.odd.cub \
# 	  TO=$(OUTPUT)/$(INPUTFILE).vis.odd.lev2.cub \
# 	  MAP=$(OUTPUT)/$(INPUTFILE).vis.even.lev2.cub MATCHMAP=TRUE \
# 	  > /dev/null;
# 	$(PROJECT) $(TSTARGS) FROM=$(OUTPUT)/$(INPUTFILE).uv.even.cub \
# 	  TO=$(OUTPUT)/$(INPUTFILE).uv.even.lev2.cub DEFAULTRANGE=CAMERA \
# 	  MINLAT=-74.672077453164 MAXLAT=-73.705457723751 \
# 	  MINLON=242.08695858855 MAXLON=247.03644145678 \
# 	  PIXRES=MPP RESOLUTION=266.757 \
# 	  > /dev/null;
# 	$(PROJECT) $(TSTARGS) FROM=$(OUTPUT)/$(INPUTFILE).uv.odd.cub \
# 	  TO=$(OUTPUT)/$(INPUTFILE).uv.odd.lev2.cub \
# 	  MAP=$(OUTPUT)/$(INPUTFILE).uv.even.lev2.cub MATCHMAP=TRUE \
# 	  > /dev/null;
# 	$(LS) $(OUTPUT)/$(INPUTFILE).vis.even.lev2.cub \
# 	  $(OUTPUT)/$(INPUTFILE).vis.odd.lev2.cub > $(OUTPUT)/vis.lis;
# 	$(MOSAIC) $(TSTARGS) FROMLIST=$(OUTPUT)/vis.lis \
# 	  MOSAIC=$(OUTPUT)/$(INPUTFILE).vis.lev2.cub > /dev/null;
# 	$(LS) $(OUTPUT)/$(INPUTFILE).uv.even.lev2.cub \
# 	  $(OUTPUT)/$(INPUTFILE).uv.odd.lev2.cub > $(OUTPUT)/uv.lis;
# 	$(MOSAIC) $(TSTARGS) FROMLIST=$(OUTPUT)/uv.lis \
# 	  MOSAIC=$(OUTPUT)/$(INPUTFILE).uv.lev2.cub > /dev/null;
# 	$(RM) $(OUTPUT)/vis.lis;
# 	$(RM) $(OUTPUT)/uv.lis;
# 	$(RM) $(OUTPUT)/$(INPUTFILE).vis.even.lev2.cub;
# 	$(RM) $(OUTPUT)/$(INPUTFILE).vis.odd.lev2.cub;
# 	$(RM) $(OUTPUT)/$(INPUTFILE).uv.even.lev2.cub;
# 	$(RM) $(OUTPUT)/$(INPUTFILE).uv.odd.lev2.cub;
# 	$(RM) $(OUTPUT)/$(INPUTFILE).vis.odd.cub;
# 	$(RM) $(OUTPUT)/$(INPUTFILE).uv.odd.cub;
