include $(ISISROOT)/make/isismake.os

# ISIS DOCS MAKEFILE
#   Create tables of contents for the Object
#   documentation and kicks off lower makefiles
#   dls - deborah lee soltesz - 4/25/2002
#   history
#     dls 3/4/2005 - modified to use doxygen instead
#                    of IsisOML system


TOCDIR = $(ISISROOT)doc/Object
OBJECTASSETS = $(wildcard $(ISISROOT)/src/*/objs/*/assets/)

help:
	echo "Isis Object Documentation Make System Commands"
	echo "------------------------ "
	echo "make docs      : Build Isis Object Documentation"
	echo "                 SHOWDOCWARNINGS=1 will show doxygen warnings"
	echo " "
	echo "make wwwdocsrc : Sync Source to Public Web Server" 
	echo "make devdocsrc : Sync Source to Development Web Server" 
	echo "make wwwdoc    : Sync Documentation to Public Web Server" 
	echo "make devdoc    : Sync Documentation to Development Web Server" 
	echo " "
	echo "make clean     : Clean up Source Area (docsys/Object)" 


docs: conf assetsDir 
	$(MAKE) documentation
	$(MAKE) render

documentation: 
	echo "      Creating Object Documentation"
	mkdir -p $(ISISROOT)/doc/Object
	mkdir -p $(ISISROOT)/doc/Object/Developer
	mkdir -p $(ISISROOT)/doc/Object/Programmer
	mkdir -p $(ISISROOT)/doc/documents/DocStyle/assets
	rsync -qlHptr --delete --rsh=ssh build/isisDoxyDefs.doxydef $(ISISROOT)/doc/documents/DocStyle/assets/


assetsDir:
	echo "        ...copying assets"
	mkdir -p $(ISISROOT)/doc/Object
	rsync -qlHptr --delete --rsh=ssh assets $(ISISROOT)/doc/Object/ 

conf:
	echo "        ...building Programmer configuration"
	cat build/Programmer.conf                                        >  build/Programmer_temp.conf
	echo "OUTPUT_DIRECTORY = $(ISISROOT)/doc/Object/"                >> build/Programmer_temp.conf
	echo "STRIP_FROM_PATH  = $(ISISROOT)/"                           >> build/Programmer_temp.conf
	echo "INPUT            = build/isisDoxyDefs.doxydef $(ISISROOT)/src/ " >> build/Programmer_temp.conf
	echo "HTML_OUTPUT      = Programmer"                             >> build/Programmer_temp.conf

	if [ "$(findstring LOUD,$(MODE))" = "LOUD" ]; \
	then \
	  echo "QUIET                  = NO"                         >> build/Programmer_temp.conf; \
	fi

	if [[ "$(DOT_PATH)" != "" ]];                                      \
	then                                                               \
	  echo "DOT_PATH  = $(DOT_PATH)" >> build/Programmer_temp.conf;    \
	fi

	if [[ "$(SHOWDOCWARNINGS)" != "" ]];                               \
	then                                                               \
	  echo "WARN_LOGFILE = " >> build/Programmer_temp.conf;            \
	fi

	echo "        ...building Developer configuration"
	cat build/Developer.conf                                    >  build/Developer_temp.conf
	echo "OUTPUT_DIRECTORY = $(ISISROOT)/doc/Object/"           >> build/Developer_temp.conf
	echo "STRIP_FROM_PATH  = $(ISISROOT)/"                      >> build/Developer_temp.conf
	echo "INPUT            = $(ISISROOT)/src/ \\"               >> build/Developer_temp.conf
	echo "                   build/isisDoxyDefs.doxydef"        >> build/Developer_temp.conf
	echo "HTML_OUTPUT      = Developer"                         >> build/Developer_temp.conf
	if [ "$(findstring LOUD,$(MODE))" = "LOUD" ]; \
	then \
	  echo "QUIET                  = NO"                         >> build/Developer_temp.conf; \
	fi

	if [[ "$(DOT_PATH)" != "" ]];                                      \
	then                                                               \
	  echo "DOT_PATH  = $(DOT_PATH)" >> build/Developer_temp.conf;     \
	fi

	if [[ "$(SHOWDOCWARNINGS)" != "" ]];                               \
	then                                                               \
	  echo "WARN_LOGFILE = " >> build/Developer_temp.conf;             \
	fi



render: doProg doDev

doProg:
	echo "        ...building Programmer documentation"
	$(DOXYGEN) build/Programmer_temp.conf

doDev:
	echo "        ...building Developer documentation"
	$(DOXYGEN) build/Developer_temp.conf
	

#----
# CLEAN
clean:
	rm -f  build/Programmer_temp.conf
	rm -f  build/Developer_temp.conf 
	rm -rf docbuild
	rm -f ../documents/DocStyle/assets/isisDoxyDefs.doxydef
	rm -f doxyWarnDeveloper.txt doxyWarnProgrammer.txt

cleandoc:
	rm -r $(ISISROOT)/doc/Object

	


#TODO
# the following are temprary macros for updating the websites with the latest document build.
# these need to be removed and replaced with a more reasonable update script

# public docsrc upload
wwwdocsrc:
	echo " "
	echo "_______________________________________________________________________________"
	echo "BEGIN SYNC: docsys to public server"
	rsync -vlHptr --delete --progress --rsh=ssh $(ISISROOT)/src/docsys/* webmaster@wwwflag:/www/httpd/Astrogeology/Extranet/htdocs/Projects/IsisXML/docsrc/

# dev docsrc upload
devdocsrc:
	echo " "
	echo "_______________________________________________________________________________"
	echo "BEGIN SYNC: docsys to dev server"
	rsync -vlHptr --delete --progress --rsh=ssh $(ISISROOT)/src/docsys/* dsoltesz@wwwdev:/www/httpd/Astrogeology/Extranet/htdocs/Projects/IsisXML/docsrc/

# public doc upload
wwwdoc:
	echo " "
	echo "_______________________________________________________________________________"
	echo "BEGIN SYNC: docs to public server"
	rsync -vlHptr --delete --progress --rsh=ssh $(ISISROOT)/doc/* webmaster@wwwflag:/www/httpd/Astrogeology/Extranet/htdocs/Projects/IsisXML/doc/

# dev doc upload
devdoc:
	echo " "
	echo "_______________________________________________________________________________"
	echo "BEGIN SYNC: docs to dev server"
	rsync -vlHptr --delete --progress --rsh=ssh $(ISISROOT)/doc/* dsoltesz@wwwdev:/www/httpd/Astrogeology/Extranet/htdocs/Projects/IsisXML/doc/

