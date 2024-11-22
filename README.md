<p align="center">
  <img src="isis/src/docsys/assets/img/image-source-files/isis-logo-red.svg" alt="ISIS" width=200> 
</p>

# ISIS

[![Anaconda-Server Badge](https://anaconda.org/usgs-astrogeology/isis3/badges/version.svg)](https://anaconda.org/usgs-astrogeology/isis3)
[![Anaconda-Server Badge](https://anaconda.org/usgs-astrogeology/isis/badges/version.svg)](https://anaconda.org/usgs-astrogeology/isis)
[![Badge for DOI 10.5066/P13TADS5](https://img.shields.io/badge/DOI-10.5066%2FP13TADS5-blue)](https://doi.org/10.5066/P13TADS5)

## Quick Reference

To start using ISIS, see:

- [Installing ISIS](https://astrogeology.usgs.gov/docs/how-to-guides/environment-setup-and-maintenance/installing-isis-via-anaconda/)
    - Distubuted with conda, compatible with Unix and Mac.
- [Intro to ISIS](https://astrogeology.usgs.gov/docs/getting-started/using-isis-first-steps/introduction-to-isis/)
    - A good primer, at the end there's an example you can try!
- [Setting Up the ISIS Data Area](https://astrogeology.usgs.gov/docs/how-to-guides/environment-setup-and-maintenance/isis-data-area/)
    - You'll need this to process images on your own.

For more info, see:

- [Tutorials and Guides](https://astrogeology.usgs.gov/docs/)
- [FAQ](https://astrogeology.usgs.gov/docs/getting-started/using-isis-first-steps/isis-faq/)
- [ISIS Application Manuals](https://isis.astrogeology.usgs.gov)

For Development and Contributing, see:

- [Building ISIS](https://astrogeology.usgs.gov/docs/how-to-guides/isis-developer-guides/developing-isis3-with-cmake/)
- [ISIS Test Data](https://astrogeology.usgs.gov/docs/how-to-guides/isis-developer-guides/obtaining-maintaining-submitting-test-data/)
- [Contributing](https://astrogeology.usgs.gov/docs/how-to-guides/isis-developer-guides/contributing-to-isis/)

## In this README:

- [Requests for Comment](README.md#Requests-for-Comment)
- [Citing ISIS](README.md#Citing-ISIS)
- [System Requirements](README.md#system-requirements)
- [Semantic Versioning](README.md#Semantic-Versioning-and-It's-Role-in-Describing-the-Software)


## Requests for Comment
The ISIS project uses a Request for Comment (RFC) model where major changes to the code, data area, or distribution are proposed, discussed, and potentially adopted.  RFCs are being housed in this repository's [wiki](https://github.com/USGS-Astrogeology/ISIS3/wiki).

No RFCs are currently open.

All contributors and users are welcome to review and comment on open RFCs.

## Citing ISIS

The badge at the top of this README lists the DOI of the most recent ISIS version.  As of 11/22/2024, the latest release of ISIS is version 8.3.0, and its DOI is [`10.5066/P13TADS5`](https://doi.org/10.5066/P13TADS5).

The [Releases Page on GitHub](https://github.com/DOI-USGS/ISIS3/releases) lists the DOI for each version of ISIS.  Older versions may be listed on [Zenodo](https://doi.org/10.5281/zenodo.2563341).  It is good practice to cite the version of the software being used by the citing work, so others can reproduce your exact results.

## System Requirements

ISIS is supported on these UNIX variants (and may work on others, though unsupported):

-   Ubuntu 18.04 LTS
-   macOS 10.13.6 High Sierra
-   Fedora 28
-   CentOS 7.2

[//]: # ( These are all past their vendors' standard support, most EOL:         )
[//]: # ( Ubuntu 18.04LTS            Released 2018.04, ESS 2023.05, EOL 2028.04 )
[//]: # ( macOS 10.13.6 High Sierra  Released 2017.09,              EOL 2020.11 )
[//]: # ( Fedora 28                  Released 2018.05,              EOL 2019.05 )
[//]: # ( CentOS 7.2                 Released 2015.12,              EOL 2024.06 )

ISIS is not supported on Windows, but 
[using WSL](https://planetarygis.blogspot.com/2024/02/isis-and-asp-on-windows-11-wsl-take-3.html) 
may be possible.

#### Architecture Support

- 64-bit x86 processors: Supported. 
- Apple Silicon: In Development.

#### Storage Space Required

- 2.5 GB for ISIS binaries
- 10 GB up to multiple TB for mission data and processing large images



[//]: # ( Legacy ISIS Versions, links broken )

[//]: # ( ISIS 2  - http://isis.astrogeology.usgs.gov/Isis2/isis-bin/installation.cgi )

[//]: # ( ISIS3 3.5.2 - https://isis.astrogeology.usgs.gov/documents/LegacyInstallGuide/index.html )


## Semantic Versioning

Versions of ISIS now use a Major.Minor.Patch scheme (e.g., 8.3.0), detailed in [RFC 2](https://github.com/DOI-USGS/ISIS3/wiki/RFC2:-Release-Process#terms). 

| Major | Minor | Patch |
|-------|-------|-------|
| 8     | .3    | .0    |


- **Patch Releases** (i.e. 8.0.2 → 8.0.3) fix a bug, but don't make breaking changes or add features.

- **Minor Releases** (i.e. 8.2.0 → 8.3.0) add features, but no breaking changes.

- **Major Releases** (i.e. 8.3.0 → 9.0.0) add a breaking change.

#### Breaking Changes

A breaking change alters API signatures, existing arguments to ISIS apps, or output.  Anything that could break backwards compatibility with a script is considered breaking*.

Additions (i.e, an added optional argument, and added column in a .csv file) aren't considered breaking, but changes to existing output or input (i.e, changing an existing argment, changing the title of an existing .csv output column) are breaking.

> *.txt files, or output meant only for human readers, are excluded from ISIS's definition of a breaking change.

Contributors must make sure that breaking changes are well-identified.  Breaking changes require input, discussion, and approval from the community before they can be adopted into ISIS.

#### Upgrade considerations

Most users can safely upgrade to Minor and Patch Versions, but should be more cautious about a Major upgrade, which may introduce changes that could alter their workflow.  You can reference the [Changelog](https://github.com/USGS-Astrogeology/ISIS3/blob/dev/CHANGELOG.md) for more specific information on the changes.

### ISIS Release Cadence (LTS)
ISIS has a [Long Term Support (LTS)](https://github.com/USGS-Astrogeology/ISIS3/discussions/4691) model. This assumes that users will update at either:

  - Each LTS release (every 18 months), or 
  - Each quarterly release (every 3 months).
