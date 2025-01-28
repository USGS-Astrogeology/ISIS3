<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:fo="http://www.w3.org/1999/XSL/Format"
    exclude-result-prefixes="xmlns fo">

<!--

This stylesheet will be used to generate the alphabetical TOC for applications

Author
Deborah Lee Soltesz
4/2002

-->


  <xsl:output
    media-type="text/html"
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
    doctype-system="http://www.w3.org/TR/html4/loose.dtd"
    indent="yes"
    encoding="utf-8"
    omit-xml-declaration="yes"
  />

  <xsl:param name="menuPath"/>

  <xsl:include href="../../build/menu.xsl"/>
  <xsl:include href="../../build/header.xsl"/>
  <xsl:include href="../../build/footer.xsl"/>


  <xsl:key name="categoryMatch" match="/tableofcontents/application" use="category/categoryItem"/>


  <xsl:template match="/">
     <xsl:apply-templates select="tableofcontents" />
  </xsl:template>


  <xsl:template match="tableofcontents">
    <html>
      <head>
        <title>
            USGS: ISIS Application Table of Contents (Alphabetical Listing)
        </title>

        <!-- ISIS Docs -->
        <link rel="stylesheet" href="{$menuPath}assets/styles/IsisStyleCommon.css"/>
        <link rel="stylesheet" media="print" href="{$menuPath}assets/styles/print.css"/>

        <!-- USGS -->
        <link rel="stylesheet" href="{$menuPath}assets/styles/usgs/common.css" />
        <link rel="stylesheet" href="{$menuPath}assets/styles/usgs/custom.css" />

        <!-- Govt -->
        <link rel="stylesheet" href="{$menuPath}assets/styles/uswds.css"/>
        <script src="{$menuPath}assets/scripts/uswds-init.min.js"></script>

        <meta name="keywords" content="Isis, applications, table of contents, image processing"/>

        <meta name="description" content="Isis Applications Table of Contents listed in alphabetical order"/>
        <meta name="publisher" content="USGS - GD - Astrogeology Research Program"/>
        <meta name="author" content="Deborah Lee Soltesz, webteam@astrogeology.usgs.gov"/>

        <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
        <meta name="country" content="USA"/>
        <meta name="state" content="AZ"/>
        <meta name="county" content="Coconino"/>
        <meta name="city" content="Flagstaff"/>
        <meta name="zip" content="86001"/>

       </head>

      <body>

        <script src="{$menuPath}assets/scripts/uswds.min.js"></script>
        
        <xsl:call-template name="writeHeader"/>

        <div id="page">

          <div class="isisMenu">
            <xsl:call-template  name="writeMenu"/>
          </div>

          <main class="isisContent">

            <h1>
              Alphabetical Listing of Applications
            </h1>

            <!-- Alphabetical tables of links to documentation -->
            <table>
            <xsl:for-each select="/tableofcontents/application">
              <xsl:sort order="ascending" select="name"/>
                <tr>
                  <th>
                    <xsl:variable name="appName" select="normalize-space(name)"/>
                    <a href="presentation/Tabbed/{$appName}/{$appName}.html">
                    <xsl:value-of select="$appName"/></a>
                  </th>
                  <td>
                    <xsl:if test="brief">
                    <xsl:value-of select="brief"/><br/>
                    </xsl:if>
                  </td>
                </tr>
            </xsl:for-each>
            </table>
          </main>
        </div>
        <xsl:call-template name="writeFooter"/>
      </body>
    </html>

  </xsl:template>

</xsl:stylesheet>
