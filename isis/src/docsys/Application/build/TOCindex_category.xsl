<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:fo="http://www.w3.org/1999/XSL/Format"
    exclude-result-prefixes="xmlns fo">

<!--

This stylesheet will be used to generate the main categorical TOC for applications

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
            USGS: ISIS Application Table of Contents (Categorical)
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

        <meta name="description" content="Isis Applications Table of Contents listed by category"/>
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
              Applications by Category
            </h1>

            <!-- links to categories -->
            <h3>Core Programs</h3>
            <ul class="card-list-sm">
                <xsl:for-each select="//application/category/categoryItem[not(normalize-space(.)=preceding::application/category/categoryItem)]">
                  <xsl:sort order="ascending" select="normalize-space(.)"/>
                    <li>
                      <a href="#{translate(normalize-space(.), ' ', '_')}">
                      <xsl:value-of select="." /></a>
                    </li>
                </xsl:for-each>
            </ul>

            <h3>Mission Specific Programs</h3>
            <ul class="card-list-sm">
                <xsl:for-each select="//application/category/missionItem[not(normalize-space(.)=preceding::application/category/missionItem)]">
                  <xsl:sort order="ascending" select="normalize-space(.)"/>
                    <li>
                      <a href="#{translate(normalize-space(.), ' ', '_')}">
                      <xsl:value-of select="." /></a>
                    </li>
                </xsl:for-each>
            </ul>

            <!-- tables of links to documentation -->

            <a name="Core"></a>
            <h2>Core Programs</h2>
            <xsl:for-each select="//application/category/categoryItem[not(normalize-space(.)=preceding::application/category/categoryItem)]">
              <xsl:sort order="ascending" select="normalize-space(.)"/>
              <h3>
                <a name="{translate(normalize-space(.), ' ', '_')}">
                  <xsl:value-of select="."/>
                </a>
              </h3>
              <xsl:variable name="categoryName" select="normalize-space(.)"/>

              <table>
                <xsl:for-each select="/tableofcontents/application/category/categoryItem">
                  <xsl:sort select="normalize-space(../../name)"/>
                  <xsl:if test="normalize-space(.) = $categoryName">
                    <tr>
                      <th class="tableCellLevel1_th">
                        <xsl:variable name="appName" select="normalize-space(../../name)"/>
                        <a href="presentation/Tabbed/{$appName}/{$appName}.html">
                        <xsl:value-of select="../../name"/></a>
                      </th>
                      <td class="tableCellLevel1">
                        <xsl:value-of select="../../brief"/><br/>
                      </td>
                    </tr>
                  </xsl:if>
                </xsl:for-each>
              </table>
              <hr noshade="noshade"/>
            </xsl:for-each>

            <a name="MissionSpecific"></a>
            <h2>Mission Specific Programs</h2>
            <xsl:for-each select="//application/category/missionItem[not(normalize-space(.)=preceding::application/category/missionItem)]">
              <xsl:sort order="ascending" select="normalize-space(.)"/>
              <h3>
                <a name="{translate(normalize-space(.), ' ', '_')}">
                  <xsl:value-of select="."/>
                </a>
              </h3>
              <xsl:variable name="missionName" select="normalize-space(.)"/>

              <table>
                <xsl:for-each select="/tableofcontents/application/category/missionItem">
                  <xsl:sort select="normalize-space(../../name)"/>
                  <xsl:if test="normalize-space(.) = $missionName">
                    <tr>
                      <th class="tableCellLevel1_th">
                        <xsl:variable name="appName" select="normalize-space(../../name)"/>
                        <a href="presentation/Tabbed/{$appName}/{$appName}.html">
                          <xsl:value-of select="../../name"/>
                        </a>
                      </th>
                      <td class="tableCellLevel1">
                        <xsl:value-of select="../../brief"/><br/>
                      </td>
                    </tr>
                  </xsl:if>
                </xsl:for-each>
              </table>
              <xsl:if test="position() != last()">
                <hr noshade="noshade"/>
              </xsl:if>
            </xsl:for-each>

          </main>
        </div>
        <xsl:call-template name="writeFooter"/>
      </body>
    </html>

  </xsl:template>

</xsl:stylesheet>
