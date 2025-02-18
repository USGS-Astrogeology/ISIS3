APPNAME = lrowaccal
FILE=wac0002c120

include $(ISISROOT)/make/isismake.tsts

commands:
	$(APPNAME) from=$(INPUT)/${FILE}.vis.odd.cub \
	  to=$(OUTPUT)/${FILE}.vis.odd.cal.cub > /dev/null;
	$(APPNAME) from=$(INPUT)/${FILE}.vis.even.cub \
          to=$(OUTPUT)/${FILE}.vis.even.cal.cub > /dev/null;
