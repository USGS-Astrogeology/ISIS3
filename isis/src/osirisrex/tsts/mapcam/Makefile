# Category test for OSIRIS-REx mapping cameras.
# The programs chosen for this test were copied from the OSIRIS-REx team's "testIsis" program.
# The points chosen for campt were altered since this is a limb image.
#
# NOTE: This test should be updated when we acquire two overlapping mapcam images.
#       See polycam test for full steps with 2 images.
#
# @history 2016-09-09 Jeannie Backer - Original version.
# @history 2017-08-29 Jeannie Backer - Updated to new test cube that has pre-processing 
#                         flip applied.
#
20190303T100344S990_map_iofL2pan_V001-cam2map_equi.cub.TOLERANCE = 1e-03

include $(ISISROOT)/make/isismake.tsts

# commands:
# 	# import first image
# 	ocams2isis $(TSTARGS)                                                                        \
# 	           from=$(INPUT)/20190303T100344S990_map_iofL2pan_V001.fits                          \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                  \
# 	           > /dev/null;

# 	# spiceinit first image
# 	spiceinit $(TSTARGS)                                                                         \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           > /dev/null;

# 	# phocube first image
# 	# create 7 band backplane with from the following values:
# 	#     dn, phase angle, emission angle, incidence angle, lat, lon, and pixel resolution
# 	phocube $(TSTARGS)                                                                           \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-bp.cub                         \
# 	           dn=t                                                                              \
# 	           phase=t                                                                           \
# 	           emission=t                                                                        \
# 	           incidence=t                                                                       \
# 	           pixelres=t                                                                        \
# 	           > /dev/null;

# 	# explode first image's backplane into 5 cubes
# 	explode $(TSTARGS)                                                                           \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-bp.cub                       \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-bp-explode                     \
# 	           > /dev/null;

# 	# reunite exploded cubes. output should be identical to phocube bp output above
# 	$(LS) $(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-bp-explode*                            \
# 	      > $(OUTPUT)/phocubeBandList.txt;
# 	cubeit $(TSTARGS)                                                                            \
# 	           from=$(OUTPUT)/phocubeBandList.txt                                                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-bp.cub                         \
# 	           > /dev/null;

# 	# for each pixel in first image, multiply by 2 then add 1 
# 	algebra $(TSTARGS)                                                                           \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-algebra.cub                    \
# 	           operator=unary                                                                    \
# 	           a=2                                                                               \
# 	           a=1                                                                               \
# 	           > /dev/null;

# 	# (SKIP) get camera information for first image
#     # caminfo fails for this image using the testIsis code provided by team
# 	#caminfo $(TSTARGS)                                                                           \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-caminfo.pvl                    \
# 	           > /dev/null;

# 	# get camera statistics for first image
# 	camstats $(TSTARGS)                                                                          \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-camstats.pvl                   \
# 	           > /dev/null;

# 	# get DN statistics for first image
# 	stats $(TSTARGS)                                                                             \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-dnstats.pvl                    \
# 	           > /dev/null;

# 	# photrim first image
# 	photrim $(TSTARGS)                                                                           \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-photrim.cub                    \
# 	           minphase=100                                                                      \
# 	           maxphase=101                                                                      \
# 	           > /dev/null;

# 	# photomet first image
# 	photomet $(TSTARGS)                                                                          \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-photomet_lunlam.cub            \
# 	           phtname=lunarlambert                                                              \
# 	           L=0.5                                                                             \
# 	           normname=albedo                                                                   \
# 	           incref=30                                                                         \
# 	           incmat=0.0                                                                        \
# 	           thresh=10e30                                                                      \
# 	           albedo=1.0                                                                        \
# 	           maxemission=80                                                                    \
# 	           maxincidenc=85                                                                    \
# 	           > /dev/null;

# 	# (SKIP) footprint first image
#     # footprintinit fails for this image using the testIsis code provided by team
# 	footprintinit $(TSTARGS)                                                                     \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           > /dev/null;


# 	# campt 4 corners and center of first image
#     # campt fails for some corner pixels, adjust s/l values to known intersections
# 	campt $(TSTARGS)                                                                             \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001_campt_upperleft.pvl            \
# 	           SAMPLE=350                                                                        \
# 	           LINE=1                                                                            \
# 	           ALLOWOUTSIDE=no                                                                   \
# 	           > /dev/null;											                         
# 	campt $(TSTARGS)                                                                             \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001_campt_upperright.pvl           \
# 	           SAMPLE=1024                                                                       \
# 	           LINE=1                                                                            \
# 	           ALLOWOUTSIDE=no                                                                   \
# 	           > /dev/null;											                         
# 	campt $(TSTARGS)                                                                             \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001_campt_lowerright.pvl           \
# 	           SAMPLE=890                                                                        \
# 	           LINE=615                                                                          \
# 	           ALLOWOUTSIDE=no                                                                   \
# 	           > /dev/null;											                         
# 	campt $(TSTARGS)                                                                             \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001_campt_lowerleft.pvl            \
# 	           SAMPLE=256                                                                        \
# 	           LINE=750                                                                          \
# 	           ALLOWOUTSIDE=no                                                                   \
# 	           > /dev/null;											                         
# 	campt $(TSTARGS)                                                                             \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001_campt_imagecenter.pvl          \
# 	           SAMPLE=512                                                                        \
# 	           LINE=512                                                                          \
# 	           ALLOWOUTSIDE=no                                                                   \
# 	           > /dev/null;											                         

# 	# get mapping group containing Equirectangular mosaic range values for the two images
# 	$(LS) $(OUTPUT)/*spiceinit.cub > $(OUTPUT)/unprojectedSpiceinitCubeList.txt;
# 	mosrange $(TSTARGS)                                                                          \
# 	            fromlist=$(OUTPUT)/unprojectedSpiceinitCubeList.txt                              \
# 	            to=$(OUTPUT)/MCAM_equi.map                                                       \
# 	            precision=6                                                                      \
# 	            projection=Equirectangular                                                       \
# 	           > /dev/null;											                         
																		                         
# 	# project the two images using the map from mosrange				                         
# 	cam2map $(TSTARGS)                                                                           \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           to=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-cam2map_equi.cub               \
# 	           map=$(OUTPUT)/MCAM_equi.map                                                       \
# 	           pixres=map                                                                        \
# 	           > /dev/null;											                         
																		                         
# 	# getsn from each image 											                         
# 	echo -e "\nOutput of getsn..." > $(OUTPUT)/MCAM_getsn.txt;  		                         
# 	getsn $(TSTARGS)                                                                             \
# 	           from=$(OUTPUT)/20190303T100344S990_map_iofL2pan_V001-spiceinit.cub                \
# 	           >> $(OUTPUT)/MCAM_getsn.txt;

# 	# change name so that pvldiff can compare
# 	$(MV) $(OUTPUT)/MCAM_equi.map $(OUTPUT)/MCAM_equi.pvl;
