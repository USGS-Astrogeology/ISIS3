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
          <p><em>Integrated Software for Imagers and Spectrometers</em></p>
        </div>

        <div id="page">
          <div class="isisMenu">
            <xsl:call-template  name="writeMenu"/>
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

