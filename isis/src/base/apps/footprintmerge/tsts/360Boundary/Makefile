APPNAME = footprintmerge

include $(ISISROOT)/make/isismake.tsts

commands:
	$(CP) $(INPUT)/*.cub $(OUTPUT)/;
	$(LS) $(OUTPUT)/*.cub > $(OUTPUT)/fromList.lis;
	$(APPNAME) fromlist=$(OUTPUT)/fromList.lis \
	to=$(OUTPUT)/islands.pvl mode=FULL > /dev/null;
	$(RM) $(OUTPUT)/fromList.lis $(OUTPUT)/*.cub;
