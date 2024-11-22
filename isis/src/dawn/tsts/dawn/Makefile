APPNAME = ckwriter 
APPNAME2 = spkwriter

VIR = VIR_IR_1A_1_39574252_1.cub
FC = FC21B0001010_09049002212F5D.cub

include $(ISISROOT)/make/isismake.tsts

# Test Dawn camera models by spiciniting using default kernels, running campt on a few 
# points then generating new kernels using ckwriter and spkwriter. Re-spiceiniting with those 
# kernels, running campt on the same (s,l) and comparing the output values. 

# This test uses images of Mars for FC and Vesta for VIR

# The two camera models tested and associated images are: 
# VIR = VIR_IR_1B_1_393705064_1.cub
# FC = FC21B0001010_09049002212F5D.cub

# commands:
#         #cp images from input to output for prog15
# 	cp $(INPUT)/$(VIR) $(OUTPUT) > /dev/null;
# 	cp $(INPUT)/$(FC) $(OUTPUT) > /dev/null;

# 	#initial spiceinit 
# 	spiceinit $(TSTARGS) from=$(OUTPUT)/$(VIR) > /dev/null;
# 	spiceinit $(TSTARGS) from=$(OUTPUT)/$(FC) > /dev/null;

# 	#initial campt

# 	#9 points from VIR image
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=7 line=6 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=119 line=5 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=248 line=6 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=246 line=40 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=139 line=42 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=10 line=42 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=27 line=24 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=115 line=25 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn.pvl sample=211 line=27 > /dev/null; 

# 	# 9 points from FC image
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=68 line=60 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=966 line=45 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=976 line=976 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=28 line=976 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=512 line=493 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=237 line=322 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=780 line=276 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=241 line=722 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn.pvl sample=823 line=739 > /dev/null;

# 	#ckwriter from spiceinit'd image to ckernel
# 	# ckwriter will not work for VIR -- non-standard camera model using housekeeping data
# 	#$(APPNAME) FROM=$(OUTPUT)/$(VIR) TO=$(OUTPUT)/ck_vir.bc > /dev/null; 
# 	$(APPNAME) FROM=$(OUTPUT)/$(FC) TO=$(OUTPUT)/ck_fc.bc > /dev/null; 

# 	#spkwriter from spiceinit'd image to spkernel 
# 	$(APPNAME2) $(TSTARGS) FROM=$(OUTPUT)/$(VIR) TO=$(OUTPUT)/spk_vir.bsp > /dev/null;
# 	$(APPNAME2) $(TSTARGS) FROM=$(OUTPUT)/$(FC) TO=$(OUTPUT)/spk_fc.bsp > /dev/null;

# 	#re-spiceinit inputs to generated kernels
# 	spiceinit $(TSTARGS) from=$(OUTPUT)/$(VIR) SPK=$(OUTPUT)/spk_vir.bsp > /dev/null;   
# 	spiceinit $(TSTARGS) from=$(OUTPUT)/$(FC) CK=$(OUTPUT)/ck_fc.bc SPK=$(OUTPUT)/spk_fc.bsp > /dev/null; 
 
# 	#final campt

# 	# 9 points from VIR image
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=7 line=6 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=119 line=5 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=248 line=6 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=246 line=40 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=139 line=42 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=10 line=42 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=27 line=24 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=115 line=25 > /dev/null; 
# 	campt $(TSTARGS) from=$(OUTPUT)/$(VIR) to=$(OUTPUT)/dawn2.pvl sample=211 line=27 > /dev/null; 

# 	# 9 points from FC image
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=68 line=60 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=966 line=45 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=976 line=976 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=28 line=976 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=512 line=493 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=237 line=322 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=780 line=276 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=241 line=722 > /dev/null;
# 	campt $(TSTARGS) from=$(OUTPUT)/$(FC) to=$(OUTPUT)/dawn2.pvl sample=823 line=739 > /dev/null;

# 	#compare initial and final pvl #diff=$(OUTPUT)/tolerances.pvl 
# 	pvldiff $(TSTARGS) from=$(OUTPUT)/dawn.pvl from2=$(OUTPUT)/dawn2.pvl diff=$(INPUT)/tolerances.pvl \
# 	to=$(OUTPUT)/diff.pvl > /dev/null;

# 	#cleanup
# 	$(RM) $(OUTPUT)/$(VIR) > /dev/null;
# 	$(RM) $(OUTPUT)/$(FC) > /dev/null; 
# 	$(RM) $(OUTPUT)/dawn.pvl > /dev/null;
# 	$(RM) $(OUTPUT)/dawn2.pvl > /dev/null;
# 	$(RM) $(OUTPUT)/ck_vir.bc > /dev/null;
# 	$(RM) $(OUTPUT)/ck_fc.bc > /dev/null;
# 	$(RM) $(OUTPUT)/spk_vir.bsp > /dev/null; 
# 	$(RM) $(OUTPUT)/spk_fc.bsp > /dev/null; 

