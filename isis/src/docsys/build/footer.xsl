<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:fo="http://www.w3.org/1999/XSL/Format"
    exclude-result-prefixes="xmlns fo">

<!--

This stylesheet will be used to generate the footer for all pages,
Include this file in other XSL files
and apply the template mode writeFooter .

Author
Deborah Lee Soltesz
12/13/2002

-->
  <xsl:template mode="writeFooter" name="writeFooter">

    <!-- REQUIRED Official USGS Footer ** DO NOT ALTER ** -->
    <footer class="footer">
      <div class="tmp-container">
        <div class="footer-doi">
          <ul class="menu nav">
            <li class="first leaf menu-links menu-level-1"><a
                href="https://www.doi.gov/privacy">DOI Privacy Policy</a></li>
            <li class="leaf menu-links menu-level-1"><a
                href="https://www.usgs.gov/policies-and-notices">Legal</a></li>
            <li class="leaf menu-links menu-level-1"><a
                href="https://www.usgs.gov/accessibility-and-us-geological-survey">Accessibility</a></li>
            <li class="leaf menu-links menu-level-1"><a
                href="https://www.usgs.gov/sitemap">Site Map</a></li>
            <li class="last leaf menu-links menu-level-1"><a
                href="https://answers.usgs.gov/">Contact USGS</a></li>
          </ul>
        </div>
        <hr/>
        <div class="footer-doi">
          <ul class="menu nav">
            <li class="first leaf menu-links menu-level-1"><a
                href="https://www.doi.gov/">U.S. Department of the Interior</a></li>
            <li class="leaf menu-links menu-level-1"><a
                href="https://www.doioig.gov/">DOI Inspector General</a></li>
            <li class="leaf menu-links menu-level-1"><a
                href="https://www.whitehouse.gov/">White House</a></li>
            <li class="leaf menu-links menu-level-1"><a
                href="https://www.whitehouse.gov/omb/management/egov/">E-gov</a></li>
            <li class="leaf menu-links menu-level-1"><a
                href="https://www.doi.gov/pmb/eeo/no-fear-act">No Fear Act</a></li>
            <li class="last leaf menu-links menu-level-1"><a
                href="https://www.usgs.gov/about/organization/science-support/foia">FOIA</a></li>
          </ul>
        </div>
      </div>
    </footer>

  </xsl:template>

</xsl:stylesheet>
