include $(ISISROOT)/make/isismake.os

# ISIS APPLICATION DOCS MAKEFILE
#   Process the XML files deposited by application
#   makefiles
#   dls - deborah lee soltesz - 4/25/2002


DEST_PRES_DIR = $(ISISROOT)/doc/Application/presentation

TABSTYLEDIR = ../presentation/Tabbed/styles
TABDIR = $(DEST_PRES_DIR)/Tabbed

APPLICATIONS = $(filter-out CVS $(wildcard *.*) $(wildcard Makefile*) , $(wildcard *))

help:
	echo "Do not run make in this directory!" 
	echo "Run make from $ISISROOT/src/docsys/Application to remake Application documentation."


APPSDIR = $(ISISROOT)/doc/Application

#------
# DOCS: generate documentation

docs: tabbed toc

#------
# TABBED
# run style sheet for Tabbed presentation
tabbed:

	echo "        Tabbed Interface Version"

	for dirname in $(APPLICATIONS); do \
 	    cd $$dirname ; $(MAKE) -f ../Makefile-application tabbed ; cd .. ; \
	done
	mkdir -p $(TABDIR)/styles ; \
	rsync -lHptr --delete --rsh=ssh $(TABSTYLEDIR)/*.css $(TABDIR)/styles/ ; \


#------
# TOC
# run style sheet for Table of Contents XML
toc: ../build/toc.xml
../build/toc.xml: $(APPSDIR) ../build/IsisApplicationTOCbuild.xsl

	echo "        Table of Contents XML"
	echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><tableofcontents>" >> ../build/toc.xml
	for dirname in $(APPLICATIONS); do \
	  echo "          Adding [$$dirname]" ; \
	  $(XALAN) $(XALAN_INFILE_OPTION) $$dirname/$$dirname.xml $(XALAN_XSL_OPTION) ../build/IsisApplicationTOCbuild.xsl >> ../build/toc.xml ; \
	done
	echo "</tableofcontents>" >> ../build/toc.xml
	mkdir -p $(ISISROOT)/bin/xml/
	cp ../build/toc.xml $(ISISROOT)/bin/xml/applicationTOC.xml

#------
# CLEAN: clean up files

clean:
	rm -rf $(APPLICATIONS) 




