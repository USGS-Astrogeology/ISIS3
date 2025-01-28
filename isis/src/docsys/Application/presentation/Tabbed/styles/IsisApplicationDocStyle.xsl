<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:fo="http://www.w3.org/1999/XSL/Format"
    exclude-result-prefixes="xmlns fo">

  <!--

  This stylesheet generates the TABBED HTML version of the application documentation

  Author
  Deborah Lee Soltesz
  4/2002

  -->

  <xsl:param name="menuPath"/>

  <xsl:output
    media-type="text/html"
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
    doctype-system="http://www.w3.org/TR/html4/loose.dtd"
    indent="yes"
    encoding="utf-8"
    omit-xml-declaration="yes"/>

  <xsl:include href="../../../../build/menu.xsl"/>
  <xsl:include href="../../../../build/header.xsl"/>
  <xsl:include href="../../../../build/footer.xsl"/>

  <xsl:template match="/">
     <xsl:apply-templates select="application" />
  </xsl:template>

  <xsl:template name="class" match="application">
    <html>
      <head>
        <title>
          <xsl:value-of select="@name"/> ISIS Application Documentation - USGS
        </title>

        <!-- Govt -->
        <link rel="stylesheet" href="{$menuPath}assets/styles/uswds.css"/>
        <script src="{$menuPath}assets/scripts/uswds-init.min.js"></script>
        
        <!-- USGS -->
        <link rel="stylesheet" href="{$menuPath}assets/styles/usgs/common.css" />
        <link rel="stylesheet" href="{$menuPath}assets/styles/usgs/custom.css" />

        <!-- ISIS Docs -->
        <link rel="stylesheet" href="{$menuPath}assets/styles/IsisStyleCommon.css"></link>
        <link rel="stylesheet" href="../styles/IsisApplicationDocStyle.css"></link>
        <link rel="stylesheet" media="print" href="{$menuPath}assets/styles/print.css"/>
        
        <noscript> <!-- Use Print stylesheet, unhide all sections if no script -->
          <link rel="stylesheet" href="{$menuPath}assets/styles/print.css"/>
        </noscript> <!-- Note: currently hides header/menu -->

        <xsl:variable name="keywordList">
          Isis, image processing,
          <xsl:value-of select="normalize-space(//application/@name)"/>
          <xsl:for-each select="//application/category/item">
            , <xsl:value-of select="normalize-space(.)"/>
          </xsl:for-each>
        </xsl:variable>
        <meta name="keywords" content="{normalize-space($keywordList)}"/>


        <!-- 'author' is the person who originally wrote this program - see history for detailed list of authors-->
        <xsl:for-each select="history/change">
          <xsl:sort order="ascending" select="@date"/>
          <xsl:if test="position() = 1">
            <meta name="author" content="{@name}"/>
          </xsl:if>
        </xsl:for-each>

        <meta name="description" content="{normalize-space(brief)}"/>
        <meta name="publisher" content="USGS - GD - Astrogeology Research Program"/>

        <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
        <meta name="country" content="USA"/>
        <meta name="state" content="AZ"/>
        <meta name="county" content="Coconino"/>
        <meta name="city" content="Flagstaff"/>
        <meta name="zip" content="86001"/>

        <script type="text/javascript">

          numExamples = <xsl:value-of select="count(/application/examples/example)"/> ;
          appName = "<xsl:value-of select="normalize-space(//application/@name)"/>" ;

          //<xsl:comment><![CDATA[

          layerArray = new Array (2 + numExamples) ;
          layerArray[0] = "overview" ;
          layerArray[1] = "Parameters" ;

          for (i = 1, j = 2 ; i <= numExamples ; i++, j++) {
            layerArray[j] = "Example" + i ;
          }
          layerArrLength = layerArray.length ;


          // CONTENT TOGGLE DISPLAY
          // changes page view

          function contentToggleVisibility(showMe) {
            for (i = 0 ; i < layerArrLength ; i++) {
              m_layer = document.getElementById([layerArray[i]]) ;
              if (m_layer) {
                m_layer.style.display = "none" ;
              }
            }
            document.getElementById([showMe]).style.display = "block" ;
          }


          tabArray = new Array (2 + numExamples) ;
          tabArray[0] = "overviewTab" ;
          tabArray[1] = "ParametersTab" ;

          for (i = 1, j = 2 ; i <= numExamples ; i++, j++) {
            tabArray[j] = "Example" + i + "Tab" ;
          }
          tabArrLength = tabArray.length ;
          currentView = "overviewTab" ;


          // CONTENT TOGGLE TAB
          // changes tab to highlight current view

          function contentToggleTab(activeTab) {
            currentView = activeTab ;
            for (i = 0 ; i < tabArrLength ; i++) {
              tab = document.getElementById([tabArray[i]]) ;
              if (tab) {
                tab.className = "tab tabOff" ;
              }
            }
            document.getElementById([activeTab]).className = "tab tabOn" ;
          }

          //]]></xsl:comment>
        </script>
      </head>

      <body onload="contentToggleVisibility('overview');">

        <script src="{$menuPath}assets/scripts/uswds.min.js"></script>

        <xsl:call-template name="writeHeader"/>

        <div id="page">

          <div class="isisMenu">
            <xsl:call-template  name="writeMenu"/>
          </div>

          <main class="isisContent">

            <noscript>
              <div style="margin: 0px; padding: 10px; font-weight: bold; background-color: gold;">
                  JavaScript is not enabled, using print styling.
                  Please note, this may disable the navigation menu.
              </div>
            </noscript>

            <h1>
              <xsl:value-of select="@name"/>
            </h1>

            <p style="margin-top:0px; font-style:italic;">
              <xsl:value-of select="brief"/>
            </p>

            <!-- TABS -->

            <div class="tab-line">
              <ul class="tab-list">

                <!-- Overview and Parameter Tabs -->
                <li>
                  <a class="tab tabOn"  id="overviewTab" 
                      onclick="contentToggleTab('overviewTab');   contentToggleVisibility('overview');"
                  >
                    Overview
                  </a>
                </li>
                <li>
                  <a class="tab tabOff" id="ParametersTab" 
                      onclick="contentToggleTab('ParametersTab'); contentToggleVisibility('Parameters');"
                  >
                    Parameters
                  </a>
                </li>
                
                <!-- Example Tabs -->
                <xsl:for-each select="examples/example">
                  <xsl:variable name="curExample"><xsl:number/></xsl:variable>
                  <li>
                    <a class="tab tabOff" id="Example{$curExample}Tab" 
                        onclick="contentToggleTab('Example{$curExample}Tab'); contentToggleVisibility('Example{$curExample}');"
                    >
                      Example <xsl:value-of select="$curExample"/>
                    </a>
                  </li>
                </xsl:for-each>

              </ul>
            </div>

            <!-- OVERVIEW PAGE VIEW -->

            <div id="overview">
              <!-- Description  -->
              <a name="Description"></a>

              <div style="font-weight: normal;">
                <xsl:apply-templates select="description/* | description/text()" mode="copyContents"/>
              </div>


              <!-- categories -->
              <a name="Categories"></a>
              <hr/>
              <h2>
                Categories
              </h2>

              <ul>
                <xsl:for-each select="category/categoryItem">
                  <li>
                      <a href="../../../index.html#{translate(normalize-space(.), ' ', '_')}">
                      <xsl:value-of select="." /></a>
                  </li>
                </xsl:for-each>
                <xsl:for-each select="category/missionItem">
                  <li>
                    <a href="../../../index.html#{translate(normalize-space(.), ' ', '_')}">
                    <xsl:value-of select="." /></a>
                  </li>
                </xsl:for-each>
              </ul>


              <!-- oldName -->
              <xsl:if test="oldName">
                <hr/>
                <h2>
                  Related Applications to Previous Versions of ISIS
                </h2>

                This program replaces the following
                <xsl:choose>
                  <xsl:when test="count(oldName/item) > 1">
                  applications
                  </xsl:when>
                  <xsl:otherwise>
                  application
                  </xsl:otherwise>
                </xsl:choose>
                existing in previous versions of ISIS:

                <ul>
                  <xsl:for-each select="oldName/item">
                    <li><xsl:value-of select="."/></li>
                  </xsl:for-each>
                </ul>
              </xsl:if>


              <!-- SeeAlso -->
              <xsl:choose>
                <xsl:when test="seeAlso">
                  <hr/>
                  <h2>
                    <a name="SeeAlso">
                      Related Objects and Documents</a>
                  </h2>

                  <!-- seeAlso -->
                  <xsl:for-each select="seeAlso">

                    <xsl:if test="applications">
                    <h3>Applications</h3>
                      <ul>
                        <xsl:for-each select="applications">
                          <xsl:for-each select="item">
                            <li><a href="../{.}/{.}.html">
                            <xsl:value-of select="."/></a></li>
                          </xsl:for-each>
                        </xsl:for-each>
                      </ul>
                    </xsl:if>

                    <xsl:if test="documents">
                    <h3>Documents</h3>
                      <ul>
                        <xsl:for-each select="documents">
                          <xsl:for-each select="document">
                            <xsl:choose>

                              <xsl:when test="source/path">
                                <li><a href="{source/path}{source/filename}">
                                <xsl:value-of select="title"/></a></li>
                              </xsl:when>

                              <xsl:when test="source/filename">
                                <li><a href="../documents/{source/filename}">
                                <xsl:value-of select="title"/></a></li>
                              </xsl:when>

                              <xsl:otherwise>
                                <li>
                                  <xsl:value-of select="title"/>

                                  <xsl:if test="author">
                                  ,  <xsl:value-of select="title"/>
                                  </xsl:if>

                                  <xsl:if test="date">
                                  ,  <xsl:value-of select="date"/>
                                  </xsl:if>

                                  <xsl:if test="publisher">
                                  ;  <xsl:value-of select="publisher"/>
                                  </xsl:if>

                                  <xsl:if test="pages">
                                  ;  <xsl:value-of select="pages"/>
                                  </xsl:if>
                                </li>
                              </xsl:otherwise>

                            </xsl:choose>
                          </xsl:for-each>
                        </xsl:for-each>
                      </ul>
                    </xsl:if>

                  </xsl:for-each>
                </xsl:when>
              </xsl:choose>


              <!-- History  -->
              <xsl:if test="history">
                <a name="History"></a>
                <hr/>
                <h2>History</h2>

                <table>
                  <xsl:for-each select="history/change[(@hidden != 'yes' and @hidden != 'true') or not(@hidden)]">
                    <tr>
                      <td class="tableCellHistory_name">
                        <xsl:value-of select="@name"/>
                      </td>

                      <td class="tableCellHistory_date">
                        <xsl:value-of select="@date"/>
                      </td>

                      <td class="tableCellHistory_description">
                        <xsl:value-of select="."/>
                      </td>
                    </tr>
                  </xsl:for-each>
                </table>
              </xsl:if>

              <!-- Liens  -->
              <xsl:if test="Liens">
                <a name="ThingsToDo"></a>
                <hr/>
                <h2>
                    Things To Do
                </h2>

                <ul>
                  <xsl:for-each select="liens/item">
                    <li><xsl:value-of select="."/></li>
                  </xsl:for-each>
                </ul>
              </xsl:if>

            </div>


            <!-- PARAMETER GROUPS PAGE VIEW -->

            <!-- Groups -->
            <xsl:if test="groups">

              <div id="Parameters">

                <h1 class="print-only">
                  Parameters
                </h1>
                
                <a name="Groups"></a>
                <!-- table of groups links -->
                <xsl:for-each select="groups">
                  <xsl:for-each select="group">
                    <xsl:variable name="groupName" select="@name"/>
                    <h2><xsl:value-of select="@name"/></h2>

                    <div class="usa-accordion usa-accordion--bordered usa-accordion--multiselectable acc-param" data-allow-multiple="">

                      <xsl:for-each select="parameter">
                        <h4 class="usa-accordion__heading">
                          <button type="button" class="usa-accordion__button acc-flex-head"
                                  aria-expanded="false" aria-controls="{@name}-info"
                          >
                            <div class="acc-param-name">
                              <xsl:value-of select="@name"/>
                            </div>
                            <div class="acc-param-desc">
                              <xsl:value-of select="brief"/>
                            </div>
                          </button>
                        </h4>
                        <div id="{@name}-info" class="usa-accordion__content usa-prose">

                          <!-- contents of pop-up window for parameter-->
                          <!-- description -->
                          <xsl:apply-templates select="description/* | description/text()" mode="copyContents"/>
                          
                          <div>
                            <table>
                              <tr>
                                <th class="tableCellLevel1_th" align="right">
                                  Type
                                </th>
                                <td class="tableCellLevel1_description">
                                  <xsl:value-of select="type"/>
                                </td>
                              </tr>

                              <!-- fileMode -->
                              <xsl:if test="fileMode">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    File Mode
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="fileMode"/>
                                  </td>
                                </tr>
                              </xsl:if>

                              <!-- pixelType -->
                              <xsl:if test="pixelType">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Pixel Type
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="pixelType"/>
                                  </td>
                                </tr>
                              </xsl:if>

                              <!-- default path -->
                              <xsl:if test="defaultPath">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Default Path
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="defaultPath"/>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="count">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Count
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="count"/>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="default">
                                <tr>
                                  <th class="tableCellLevel1_th">
                                    Default
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="default"/>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="internalDefault">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Internal Default
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="internalDefault"/>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="list">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right" valign="top">
                                    Option List:
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <table>
                                      <tr>
                                        <th class="tableCellLevel2" valign="top">
                                        Option</th>
                                        <th class="tableCellLevel2" valign="top">
                                        Brief</th>
                                        <th class="tableCellLevel2" valign="top">
                                        Description</th>
                                      </tr>
                                      <xsl:for-each select="list/option">
                                        <tr>
                                          <td class="tableCellLevel2_name" valign="top">
                                            <xsl:value-of select="@value"/>
                                          </td>
                                          <td class="tableCellLevel2_type" valign="top">
                                            <xsl:value-of select="brief"/>
                                          </td>
                                          <td class="tableCellLevel2_description" valign="top">
                                            <xsl:apply-templates select="description/* | description/text()" mode="copyContents"/>

                                            <xsl:if test="exclusions">
                                              <h4>Exclusions</h4>
                                              <ul>
                                              <xsl:for-each select="exclusions/item">
                                                <li><xsl:value-of select="."/></li>
                                              </xsl:for-each>
                                              </ul>
                                            </xsl:if>
                                            <xsl:if test="inclusions">
                                              <h4>Inclusions</h4>
                                              <ul>
                                              <xsl:for-each select="inclusions/item">
                                                <li><xsl:value-of select="."/></li>
                                              </xsl:for-each>
                                              </ul>
                                            </xsl:if>

                                          </td>

                                        </tr>
                                      </xsl:for-each>
                                    </table>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="minimum">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Minimum
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="minimum"/>
                                    <xsl:choose>
                                      <xsl:when test="minimum/@inclusive = 'yes'">
                                        (inclusive)
                                      </xsl:when>
                                      <xsl:when test="minimum/@inclusive = 'true'">
                                        (inclusive)
                                      </xsl:when>
                                      <xsl:otherwise>
                                        (exclusive)
                                      </xsl:otherwise>
                                    </xsl:choose>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="maximum">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Maximum
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="maximum"/>
                                    <xsl:choose>
                                      <xsl:when test="maximum/@inclusive = 'yes'">
                                        (inclusive)
                                      </xsl:when>
                                      <xsl:when test="maximum/@inclusive = 'true'">
                                        (inclusive)
                                      </xsl:when>
                                      <xsl:otherwise>
                                        (exclusive)
                                      </xsl:otherwise>
                                    </xsl:choose>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="greaterThan">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Greater Than
                                  </th>
                                  <td class="tableCellLevel1_description">
                                      <xsl:for-each select="greaterThan/item">
                                          <xsl:value-of select="."/><br/>
                                      </xsl:for-each>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="greaterThanOrEqual">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Greater Than or Equal
                                  </th>
                                  <td class="tableCellLevel1_description">
                                      <xsl:for-each select="greaterThanOrEqual/item">
                                          <xsl:value-of select="."/><br/>
                                      </xsl:for-each>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="lessThan">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Less Than
                                  </th>
                                  <td class="tableCellLevel1_description">
                                      <xsl:for-each select="lessThan/item">
                                        <xsl:value-of select="."/><br/>
                                      </xsl:for-each>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="lessThanOrEqual">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Less Than or Equal
                                  </th>
                                  <td class="tableCellLevel1_description">
                                      <xsl:for-each select="lessThanOrEqual/item">
                                        <xsl:value-of select="."/><br/>
                                      </xsl:for-each>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="notEqual">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Not Equal
                                  </th>
                                  <td class="tableCellLevel1_description">
                                      <xsl:for-each select="notEqual/item">
                                        <xsl:value-of select="."/><br/>
                                      </xsl:for-each>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="odd">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Odd
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    This value must be an odd number
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="exclusions">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Exclusions
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <ul>
                                      <xsl:for-each select="exclusions/item">
                                        <li><xsl:value-of select="."/></li>
                                      </xsl:for-each>
                                    </ul>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="inclusions">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Inclusions
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <ul>
                                      <xsl:for-each select="inclusions/item">
                                        <li><xsl:value-of select="."/></li>
                                      </xsl:for-each>
                                    </ul>
                                  </td>
                                </tr>
                              </xsl:if>

                              <xsl:if test="filter">
                                <tr>
                                  <th class="tableCellLevel1_th" align="right">
                                    Filter
                                  </th>
                                  <td class="tableCellLevel1_description">
                                    <xsl:value-of select="filter"/>
                                  </td>
                                </tr>
                              </xsl:if>
                            </table>
                          </div>


                        </div>
                      </xsl:for-each>

                    </div>

                  </xsl:for-each>
                </xsl:for-each>

              </div>
            </xsl:if>

            <!-- EXAMPLES PAGE VIEWS -->
            <!-- Examples -->
            <xsl:if test="examples">
              <xsl:for-each select="examples">
                <xsl:for-each select="example">
                <xsl:variable name="curExample"><xsl:number/></xsl:variable>
                  <div id="Example{$curExample}">

                    <h1 class="print-only">
                      Example <xsl:value-of select="$curExample"/>
                    </h1>
                    
                    <h2>
                      <xsl:value-of select="brief"/>
                    </h2>

                    <div>
                      <xsl:apply-templates select="description/* | description/text()" mode="copyContents"/>
                    </div>

                    <xsl:if test="terminalInterface">
                      <h3>
                      Command Line
                      </h3>

                      <div>
                        <xsl:for-each select="terminalInterface">
                          <div class="cmd-line-caption">
                            <xsl:apply-templates select="description/* | description/text()" mode="copyContents"/>
                          </div>
                          <div>
                            <code class="cmd-line">
                              <xsl:value-of select="/application/@name"/>
                              <xsl:text> </xsl:text>
                              <xsl:value-of select="commandLine"/>
                            </code>
                          </div>
                        </xsl:for-each>
                      </div>

                    </xsl:if>

                    <!-- GUI Screenshots -->
                    <xsl:if test="guiInterfaces">
                      <h3>GUI Screenshot</h3>
                      <xsl:for-each select="guiInterfaces/guiInterface/image">
                        <xsl:apply-templates mode="cardImages" select="."/>
                      </xsl:for-each>
                    </xsl:if>

                    <!-- Input Images -->
                    <xsl:if test="inputImages">
                      <h3>
                        <xsl:choose>
                          <xsl:when test="count(inputImages/image) > 1">
                                  Input Images
                          </xsl:when>
                          <xsl:otherwise>
                            Input Image
                          </xsl:otherwise>
                        </xsl:choose>
                      </h3>

                      <xsl:for-each select="inputImages/image">
                        <xsl:apply-templates mode="cardImages" select="."/>
                      </xsl:for-each>

                    </xsl:if>


                    <!-- Data Files -->
                    <xsl:if test="dataFiles">
                      <h3>
                        <xsl:choose>
                    <xsl:when test="count(dataFiles/dataFile) > 1">
                            Data Files
                    </xsl:when>
                    <xsl:otherwise>
                      Data File
                    </xsl:otherwise>
                        </xsl:choose>
                      </h3>
                      <span class="caption">
                      Links open in a new window.
                      </span>

                      <div>
                        <table cellpadding="5">
                          <xsl:for-each select="dataFiles/dataFile">
                            <tr>
                              <th class="tableCellLevel1_th">
                                <a href="{@path}" target="_blank"><xsl:value-of select="brief"/></a>
                              </th>
                              <td class="tableCellLevel1">
                                <xsl:apply-templates select="description/* | description/text()" mode="copyContents"/>
                              </td>
                            </tr>
                          </xsl:for-each>
                        </table>
                      </div>

                    </xsl:if>


                    <xsl:if test="outputImages">
                      <h3>
                        <xsl:choose>
                          <xsl:when test="count(outputImages/image) > 1">
                                  Output Images
                          </xsl:when>
                          <xsl:otherwise>
                            Output Image
                          </xsl:otherwise>
                        </xsl:choose>
                      </h3>

                      <xsl:for-each select="outputImages/image">
                        <xsl:apply-templates mode="cardImages" select="."/>
                      </xsl:for-each>

                    </xsl:if>

                  </div>
                </xsl:for-each>
              </xsl:for-each>
            </xsl:if>

          </main>
        </div>
        <xsl:call-template name="writeFooter"/>
      </body>
    </html>
  </xsl:template>


  <xsl:template match="image" mode="cardImages">

    <div class="ex-image-box">
      <div class="ex-image-img-div">
        <a title="View Image" href="{normalize-space(@src)}" target="_blank" rel="noopener noreferrer">
          <img class="ex-image-img" src="{normalize-space(thumbnail/@src)}" alt="{normalize-space(thumbnail/@caption)}"/>
        </a>
        
      </div>
      <div class="ex-image-desc">
        <h4 class="usa-card__heading">
          <xsl:value-of select="brief"/> <!-- Not sure if brief and caption should be switched here -->
        </h4>
        <em style="font-style: italic;"><xsl:value-of select="thumbnail/@caption"/></em>
        <xsl:if test="parameterName">
          <p>
            <span style="font-weight:bold;">
            Parameter Name:
            </span>
            <xsl:value-of select="parameterName"/>
            <br/>
            <xsl:value-of select="parameterName/description"/>
          </p>
        </xsl:if>
        <p>
          <xsl:apply-templates select="description/* | description/text()" mode="copyContents"/>
        </p>
      </div>
    </div>

  </xsl:template>

  <xsl:variable name="lower" select="'abcdefghijklmnopqrstuvwxyz'"/>
  <xsl:variable name="upper" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>

  <xsl:template match="def" mode="copyContents">
    <xsl:variable name="text">
      <xsl:choose>
        <xsl:when test ="@link">
          <xsl:value-of select="normalize-space(@link)"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="normalize-space(.)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="anchor">
      <xsl:value-of select="translate(translate($text, ' ', '-'), $upper, $lower)" />
    </xsl:variable>

    <a href="https://astrogeology.usgs.gov/docs/concepts/glossary/#{$anchor}">
      <xsl:apply-templates mode="copyContents"/>
    </a>
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
