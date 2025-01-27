APPNAME = isisminer

include $(ISISROOT)/make/isismake.tsts

commands:
	$(LS) $(INPUT)/*.pvl >& $(OUTPUT)/badgeomfix_data.lis;
	$(APPNAME) config=$(INPUT)/badgeomfix.conf \
	  parameters="fromlist:$(OUTPUT)/badgeomfix_data.lis@tocsv:$(OUTPUT)/badgeomfix.csv" > /dev/null
	$(RM) $(OUTPUT)/badgeomfix_data.lis;

