<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:fo="http://www.w3.org/1999/XSL/Format"
    exclude-result-prefixes="xmlns fo">

<!--

This stylesheet will be used to generate the header for all pages,
Include this file in other XSL files
and apply the template mode writeHeader .

Author
Deborah Lee Soltesz
12/13/2002

-->

  <xsl:template mode="writeHeader" name="writeHeader">

    <header>
      <!-- Government Website Banner-->
      <section class="usa-banner" aria-label="Official website of the United States government">
        <div class="usa-accordion">
          <header class="usa-banner__header">
            <div class="usa-banner__inner">
              <div class="grid-col-auto">
                <img
                  aria-hidden="true"
                  class="usa-banner__header-flag"
                  src="{$menuPath}assets/img/us_flag_small.png"
                  alt=""
                />
              </div>
              <div class="grid-col-fill tablet:grid-col-auto" aria-hidden="true">
                <p class="usa-banner__header-text">
                  An official website of the United States government
                </p>
                <p class="usa-banner__header-action">Here’s how you know</p>
              </div>
              <button
                type="button"
                class="usa-accordion__button usa-banner__button"
                aria-expanded="false"
                aria-controls="gov-banner-default"
              >
                <span class="usa-banner__button-text">Here’s how you know</span>
              </button>
            </div>
          </header>
          <div
            class="usa-banner__content usa-accordion__content"
            id="gov-banner-default"
          >
            <div class="grid-row grid-gap-lg">
              <div class="usa-banner__guidance tablet:grid-col-6">
                <img
                  class="usa-banner__icon usa-media-block__img"
                  src="{$menuPath}assets/img/icon-dot-gov.svg"
                  role="img"
                  alt=""
                  aria-hidden="true"
                />
                <div class="usa-media-block__body">
                  <p>
                    <strong>Official websites use .gov</strong><br />A
                    <strong>.gov</strong> website belongs to an official government
                    organization in the United States.
                  </p>
                </div>
              </div>
              <div class="usa-banner__guidance tablet:grid-col-6">
                <img
                  class="usa-banner__icon usa-media-block__img"
                  src="{$menuPath}assets/img/icon-https.svg"
                  role="img"
                  alt=""
                  aria-hidden="true"
                />
                <div class="usa-media-block__body">
                  <p>
                    <strong>Secure .gov websites use HTTPS</strong><br />A
                    <strong>lock</strong>
                    (<span class="icon-lock"
                      ><svg
                        xmlns="http://www.w3.org/2000/svg"
                        width="52"
                        height="64"
                        viewBox="300 100 800 1000"
                        class="usa-banner__lock-image-pd"
                        role="img"
                        aria-labelledby="banner-lock-description-default"
                        focusable="false"
                      >
                        <title id="banner-lock-title-default">Lock</title>
                        <desc id="banner-lock-description-default">Locked padlock icon</desc>
                        <path
                          fill="#000000"
                          fill-rule="evenodd"
                          d="M955.5,560.9v471.4c0,14.3-5.7,28-15.9,38.1-10.1,10.1-23.8,15.8-38.1,15.9H302.5c-14.3,0-28-5.8-38.1-15.9-10.1-10.1-15.8-23.8-15.9-38.1v-471.4c0-14.3,5.7-28,15.9-38.1,10.1-10.1,23.8-15.8,38.1-15.9h58.4v-120.6c0-63.9,25.4-125.2,70.6-170.4,45.2-45.2,106.5-70.6,170.5-70.6s125.2,25.4,170.5,70.6c45.2,45.2,70.6,106.5,70.6,170.5v120.6h58.4,0c14.3,0,28,5.7,38.1,15.9,10.1,10.1,15.8,23.8,15.9,38.1h0ZM767.5,386.3c0-43.9-17.4-86-48.5-117-31-31-73.1-48.5-117-48.5s-86,17.4-117,48.5-48.5,73.1-48.5,117v120.6h330.9v-120.6Z"
                        />
                      </svg> </span
                    >) or <strong>https://</strong> means you’ve safely connected to
                    the .gov website. Share sensitive information only on official,
                    secure websites.
                  </p>
                </div>
              </div>
            </div>
          </div>
        </div>
      </section>
    </header>

    <!-- Official USGS Header -->
    <header id="navbar" class="header-nav" role="banner">
      <div class="tmp-container">
        <div class="header-search">
          <a class="logo-header" href="https://www.usgs.gov/" title="Home">
            <img src="https://asc-docs.s3.us-west-2.amazonaws.com/common/img/usgs-vis-2x.png"
              style="height: 50px; margin-top: 8px; margin-bottom: 8px;" alt="Home" class="img" border="0" />
          </a>
        </div>
      </div>
    </header>

    <header class="usa-header usa-header--extended narrow-only">
      <nav aria-label="Mobile navigation" class="usa-nav">
        <div class="usa-nav__inner">
          <button type="button" class="usa-nav__close">
            <img src="{$menuPath}assets/img/usa-icons/close.svg" role="img" alt="Close" />
          </button>
          <xsl:call-template  name="writeMenu"/>
        </div>
      </nav>
    </header>

    <div class="topnav-container narrow-only">
      <div class="logo-bar">
        <img class="nav-m-logo"
          src="{$menuPath}assets/img/isis-logo-yellow-notxt.svg"/>
        <div class="name-bar"><em class="isis-name">ISIS Documentation</em></div>
      </div>
      <button class="usa-menu-btn" type="button">Menu</button>
    </div>
  </xsl:template>

</xsl:stylesheet>
