# Test spiceinit on a file which uses a shapemodel with the Renderer, etc specified in an 
# IsisPreferences file. 

APPNAME = spiceinit

include $(ISISROOT)/make/isismake.tsts

commands:
# # Test with IsisPreferences set for Embree
# 	cp $(INPUT)/st_2458542208_v.cub $(OUTPUT)/ > /dev/null;
# 	$(APPNAME) from=$(OUTPUT)/st_2458542208_v.cub shape=user \
# 	model='$$hayabusa/kernels/dsk/hay_a_amica_5_itokawashape_v1_0_64q.bds' \
# 	-pref=$(INPUT)/IsisPreferences_embree > /dev/null;
# 	catlab from=$(OUTPUT)/st_2458542208_v.cub to=$(OUTPUT)/label_embree.pvl > /dev/null;

# # Test with IsisPreferences set for Bullet
# 	$(APPNAME) from=$(OUTPUT)/st_2458542208_v.cub shape=user \
# 	model='$$hayabusa/kernels/dsk/hay_a_amica_5_itokawashape_v1_0_64q.bds' \
# 	-pref=$(INPUT)/IsisPreferences_bullet > /dev/null; 
# 	catlab from=$(OUTPUT)/st_2458542208_v.cub to=$(OUTPUT)/label_bullet.pvl > /dev/null;

# # Test for resetting back to the default (no Embree or Bullet) if nothing is specified
# 	$(APPNAME) from=$(OUTPUT)/st_2458542208_v.cub -pref=$(INPUT)/IsisPreferences_default > /dev/null;
# 	catlab from=$(OUTPUT)/st_2458542208_v.cub to=$(OUTPUT)/label_default.pvl > /dev/null;
# 	rm $(OUTPUT)/st_2458542208_v.cub; 
