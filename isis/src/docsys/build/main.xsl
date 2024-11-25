<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
    xmlns:fo="http://www.w3.org/1999/XSL/Format" 
    exclude-result-prefixes="xmlns fo">

<!--

This stylesheet will be used to generate the main content frame of the home page

Author
Deborah Lee Soltesz
04/15/2003

-->

<xsl:param name="menuPath"/>

<xsl:include href="menu.xsl"/>
<xsl:include href="header.xsl"/>
<xsl:include href="footer.xsl"/>

<xsl:output 
  media-type="text/html" 
  doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
  doctype-system="http://www.w3.org/TR/html4/loose.dtd"
  indent="yes"
  encoding="utf-8"
  omit-xml-declaration="yes">
  <xsl:template match="/">
     <xsl:apply-templates select="//homePage" />
  </xsl:template>
</xsl:output>

  <xsl:template match="//homePage">
      <html>
      <head>
        <title>USGS Isis: Planetary Image Processing Software</title>
        <meta name="keywords" content="Isis, image processing, software, open source, remote sensing, planetary science, astrogeology"/>
        <!-- need ratings tag -->
        <meta name="description" content="Integrated Software for Imagers and Spectrometers (ISIS), created and managed by the USGS Astrogeology Research Program. ISIS provides a comprehensive, user-friendly, portable tool for processing, analyzing, and displaying remotely sensed image data."/>
        <meta name="publisher" content="USGS - GD - Astrogeology Program"/>
        <meta name="author" content="Deborah Lee Soltesz, webteam@astrogeology.usgs.gov"/>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
        <meta name="country" content="USA"/>
        <meta name="state" content="AZ"/>
        <meta name="county" content="Coconino"/>
        <meta name="city" content="Flagstaff"/>
        <meta name="zip" content="86001"/>

        <!-- ISIS Docs -->
        <link rel="stylesheet" href="assets/styles/IsisStyleCommon.css"/>
        <link rel="stylesheet" media="print" href="assets/styles/print.css"/>

        <!-- USGS -->
        <link rel="stylesheet" href="assets/styles/usgs/common.css" />
        <link rel="stylesheet" href="assets/styles/usgs/custom.css" />

        <!-- Govt -->
        <link rel="stylesheet" href="assets/styles/uswds.css"/>
        <script src="assets/scripts/uswds-init.min.js"></script>

        <style>
          .topnav-container {
            border-top-color: black;
          }
        </style>

      </head>

      <body>

        <script src="assets/scripts/uswds.min.js"></script>
        
        <xsl:call-template name="writeHeader"/>

        <div id="isis-banner">
          <div>
              <div class="isis-logo"></div>
                <h3 class="title-overlay">
                  Integrated Software for <br/>
                  Imagers and Spectrometers
                </h3>
          </div>
            <div class="usa-card__container width-tablet">
              <div class="usa-card__header">
                <h4 class="usa-card__heading">Some ISIS Docs have Moved</h4>
              </div>
              <div class="usa-card__body">
                <p>
                  ISIS documentation, including ISIS tutorials, guides, and concepts,
                  have been moved to <a href="https://astrogeology.usgs.gov/docs/" target="_blank">USGS Astrogeology Software Docs</a>.
                  Manuals for each ISIS Application <a href="{$menuPath}Application/index.html" target="_blank">remain here</a>, 
                  as do docs for previous ISIS versions, accessible via the left panel.
                  
                </p>
              </div>
              <div class="usa-card__footer" style="display: flex;">
                <a href="https://astrogeology.usgs.gov/docs" class="usa-button">Go to USGS Astrogeology Software Docs</a>
                <a href="{$menuPath}Application/index.html" class="usa-button">Go to ISIS App Manuals</a>
              </div>
            </div>
      </div>
      <div class="image-credit-fixed">
        <p>This artist's concept depicts NASA's Europa Clipper spacecraft in orbit around Jupiter.</p>
        <p style="color: #fff">Image Credit: <a style="color: #fff" href="https://photojournal.jpl.nasa.gov/catalog/PIA26068" target="_blank">NASA/JPL-Caltech</a></p>
    </div>

        <div id="page">
          <div class="isisMenu">
            <xsl:call-template  name="writeFrontPageMenu"/>
          </div>
          <main class="isisContent">
            <!--xsl:copy-of select="."/-->
            <xsl:apply-templates select="* | text()" mode="copyContents"/>
          </main>
        </div>
        <xsl:call-template name="writeFooter"/>
      <!-- end of body -->
      </body>
      </html>

  </xsl:template>

  <xsl:template match="*" mode="copyContents">
    <xsl:element name="{name()}" namespace="{namespace-uri()}">
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates mode="copyContents"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="text()" mode="copyContents">
      <xsl:value-of select="."/>
      <xsl:apply-templates mode="copyContents"/>
  </xsl:template>

</xsl:stylesheet>
