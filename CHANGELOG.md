# Changelog

All changes that impact users of this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

<!---
This document is intended for users of the applications and API. Changes to things
like tests should not be noted in this document.

When updating this file for a PR, add an entry for your change under Unreleased
and one of the following headings:
 - Added - for new features.
 - Changed - for changes in existing functionality.
 - Deprecated - for soon-to-be removed features.
 - Removed - for now removed features.
 - Fixed - for any bug fixes.
 - Security - in case of vulnerabilities.

If the heading does not yet exist under Unreleased, then add it as a 3rd heading,
with three #.


When preparing for a public release candidate add a new 2nd heading, with two #, under
Unreleased with the version number and the release date, in year-month-day
format. Then, add a link for the new version at the bottom of this document and
update the Unreleased link so that it compares against the latest release tag.


When preparing for a bug fix release create a new 2nd heading above the Fixed
heading to indicate that only the bug fixes and security fixes are in the bug fix
release.
-->

## [Unreleased]

### Added
- Added the USECAMSTATSTBL option to caminfo. This allows caminfo to extract existing
camera statistics from the CameraStatistics Table of the input cube instead
of recalculating CameraStatistics. Updated caminfo to output all CameraStatistics
Keywords when running CAMSTATS.  [#3605](https://github.com/USGS-Astrogeology/ISIS3/issues/3605).
- Added the ability to search filenames in measure's drop down boxes in Qnet Point Editor. [#4581](https://github.com/USGS-Astrogeology/ISIS3/issues/4581)
- Added slope, local normal, and ellipsoid normal calculations to phocube. [#3635](https://github.com/USGS-Astrogeology/ISIS3/issues/3645)
- Added additional translation files for TGO CaSSiS in order to support PSA compliant labels. [#4567](https://github.com/USGS-Astrogeology/ISIS3/issues/4567)

### Deprecated
- Deprecated edrget as discussed in [#3313](https://github.com/USGS-Astrogeology/ISIS3/issues/3313).

## [6.0.0] - 2021-08-27

### Added
- Added a new application, isisimport. The application is designed to be a replacement for many of the mission/instrument specific import applications. It does not contain the templates for those applications at this time. It uses a templateing engine instead of the translation files.
- Added a new dark current correction to hical that works with the higher temperatures recent images are captured at. Use the new config file, $ISISDATA/mro/calibration/hical.0023_darkrate.conf, to enable the new dark current correction over the old dark current correction. Runs of hical without the new dark current correction will also produce an extra line in the output log indicating that the ZeroDarkRate module is disabled. [#4324](https://github.com/USGS-Astrogeology/ISIS3/issues/4324)
- Added the ability to bundle adjust CSM models in jigsaw. Use the new CSMSOLVESET, CSMSOLVETYPE, and CSMSOLVELIST arguments to specify what you solve for. [#4537](https://github.com/USGS-Astrogeology/ISIS3/pull/4537)

### Changed
- Added the ability to export footprint information from a CaSSIS ISIS Cube label to the generated output PDS4 label in tgocassisrdrgen. [#4473](https://github.com/USGS-Astrogeology/ISIS3/issues/4473)
- isisVarInit.py no longer writes a "cat" statement by default to the activate scripts which cause the ISIS version information to be written on conda activate.  This can be included in those scripts via a command line option to isisVarInit.py.  Also, a quiet option is provided to isisVarInit.py to suppress its own writing to standard out, if needed.
- Changed how the images.csv file is output in jigsaw when there are multiple models. Now each sensor model will have its own images.csv file so that column headers can all be correct. For example, a solution involving LRONAC pairs and Apollo Metric images would generate three images.csv files: LRONAC Left, LRONAC Right, and Apollo Metric. [#4324](https://github.com/USGS-Astrogeology/ISIS3/issues/4324)
- Changed the API of many Bundle utility classes as part of CSM support in jigsaw. [#4324](https://github.com/USGS-Astrogeology/ISIS3/issues/4324)

## [5.0.2] - 2021-07-30

### Fixed
- Fixed logging in FindFeatures where we were trying to get a non-existent Pvl group from the Pvl log. [#4375](https://github.com/USGS-Astrogeology/ISIS3/issues/4375)
- Fixed an Minimum|Maximum calculation error when comparing all equal data in the qview statstics tool. [#4433](https://github.com/USGS-Astrogeology/ISIS3/issues/4414)
- Fixed ISIS docs for incorrect path and -WEBHELP issues [#4510](https://github.com/USGS-Astrogeology/ISIS3/issues/4510)
- Fixed a bug where writing out updated positions for framing cameras in jigsaw caused an error. [#4545](https://github.com/USGS-Astrogeology/ISIS3/issues/4545)
- Fixed a bug where comments on BLOB labels were not written out. [#4442](https://github.com/USGS-Astrogeology/ISIS3/issues/4442)

## [5.0.1] - 2021-06-10

### Fixed
- Fixed an arccos evaluating a double close to either 1, -1 when calculating the ground azimuth in camera.cpp. [#4393](https://github.com/USGS-Astrogeology/ISIS3/issues/4393)
- Fixed hist outputs to N/A when all DNs are special pixels. [#3709](https://github.com/USGS-Astrogeology/ISIS3/issues/3709)
- Fixed SolarLon to compute from Table if cube is spiceinited. [#3703](https://github.com/USGS-Astrogeology/ISIS3/issues/3703)
- Fixed GUI alignment to be top aligned rather than centered to make parameters less ambiguous. [#3710](https://github.com/USGS-Astrogeology/ISIS3/issues/3710)
- Fixed hideal2pds bug where parameterizing for 8-bit output create 18-bit output. [#4006](https://github.com/USGS-Astrogeology/ISIS3/issues/4006)
- Fixed Thm2isis to properly use output attributes [#4213](https://github.com/USGS-Astrogeology/ISIS3/issues/4213)
- Fixed caminfo uselabel SegFault. [#4401](https://github.com/USGS-Astrogeology/ISIS3/issues/4401)

### Changed
- isisVarInit.py no longer writes a "cat" statement by default to the activate scripts which cause the ISIS version information to be written on conda activate.  This can be included in those scripts via a command line option to isisVarInit.py.  Also, a quiet option is provided to isisVarInit.py to suppress its own writing to standard out, if needed.
- Changed the name of topds4 to isisexport. The application is designed to be a replacement for many of the mission/instrument specific export applications. It uses a templateing engine instead of the translation files.


## [5.0.0] - 2021-04-01

### Added
- Added the ability to read MiMAP v3 images through mimap2isis from the Kaguya data set. [#2067](https://github.com/USGS-Astrogeology/ISIS3/issues/2067)
- The following calibration applications were updated to not require local mission-specific SPICE kernels when working with spiceinited cubes: amicacal, ctxcal, lrowaccal, moccal, mdiscal, hical, hicalbeta, vikcal, and gllssical. This makes it possible to first run spiceinit using the spice server and then run these calibration applications without ever needing to download mission-specific kernels. If spiceinit has not been run on the input cube, these apps will still require the kernels area to run. [#4303](https://github.com/USGS-Astrogeology/ISIS3/issues/4303)
- Added the new csminit application and CSM Library loading to the IsisPreferences file. Together these allow users to get CSM state strings from ISD files. Once CSM camera model support is added, these will be used to setup a Cube to use a CSM camera model.
- Added a new application, topds4, which generates an output PDS4 XML label and a PDS4-compliant ISIS Cube from an input Cube, a PDS4 label template, and optionally additional input XML, PVL, or JSON data. The Inja templating engine is used to render the output PDS4 label from the label template. [#4246](https://github.com/USGS-Astrogeology/ISIS3/pull/4246)
- Added the ability to use a Community Sensor Model (CSM) instead of an ISIS camera model. To use a CSM sensor model with a Cube run the csminit application on the Cube instead of spiceinit.

### Fixed

- Fixed relative paths not being properly converted to absolute paths in isisVarInit.py [4274](https://github.com/USGS-Astrogeology/ISIS3/issues/4274)
- Fixed issue where serial numbers for Kaguya TC and MI image could not be generated. [4235](https://github.com/USGS-Astrogeology/ISIS3/issues/4235)
- Fixed hardcoded file naming in the hijitter app dealing with output from pipeline. [#4372](https://github.com/USGS-Astrogeology/ISIS3/pull/4372)
- Fixed "About Qview" to point to website documentation. [4333](https://github.com/USGS-Astrogeology/ISIS3/issues/4333)
- Fixed bug where the time bias was not being added to the ephemeris times in ckwriter. [4129](https://github.com/USGS-Astrogeology/ISIS3/issues/4129)

### Changed

- Updated the FileList object to handle files that do not contain a trailing new line character. [#4372](https://github.com/USGS-Astrogeology/ISIS3/pull/4372)
- Refactored Blob class to be used by classes that serialize to a Cube instead of inherited from. Impacted classes are GisBlob, History, ImagePolygon, OriginalLabel, OriginalXmlLabel, StrethBlob, StringBlob, and Table. [#4082](https://github.com/USGS-Astrogeology/ISIS3/issues/4082)
- Fixed hi2isis MRO:ADC_TIMING_SETTINGS label conversion issue [4290](https://github.com/USGS-Astrogeology/ISIS3/issues/4290)
- Changed csv2table to identify headers with arrays and create table fields as arrays instead of single fields for each element [3676](https://github.com/USGS-Astrogeology/ISIS3/issues/3676)

## [4.4.0] - 2021-02-11

### Added

- Added warning to ocams2isis about the model being out of date. [#4200](https://github.com/USGS-Astrogeology/ISIS3/issues/4200)
- Added documentation to lronaccal and lrowaccal to describe why there are negative DNs in I/F calibrated images. [#3860](https://github.com/USGS-Astrogeology/ISIS3/issues/3860)
- Update qview MeasureTool to add an option to calculate distances using RA/DEC and update qview to show DEC/RA rather than LAT/LON in lower-right corner [#3371](https://github.com/USGS-Astrogeology/ISIS3/issues/3371)
- Updated spiceinit so that a user can specify a shape model and use the spice web service without any errors. [#1986](https://github.com/USGS-Astrogeology/ISIS3/issues/1986)

### Fixed

- Fixed lrowaccal so required SPICE files are reported instead of continuing without them. [#4038](https://github.com/USGS-Astrogeology/ISIS3/issues/4038)
- Fixed not being able to enable USECOORDLIST argument in mappt. [#4150](https://github.com/USGS-Astrogeology/ISIS3/issues/4150)
- Fixed history entry not being added to a cube when running spiceinit with web=true. [4040](https://github.com/USGS-Astrogeology/ISIS3/issues/4040)
- Updated wavelength and bandbin values in translation files for the TGO CaSSIS BandBin group. [4147](https://github.com/USGS-Astrogeology/ISIS3/issues/4147)
- Fixed the JunoCam serialNumber translation using an old keyword. [4341](https://github.com/USGS-Astrogeology/ISIS3/issues/4341)
- Fixed map2map bug where small images would return all null image [#632](https://github.com/USGS-Astrogeology/ISIS3/issues/631)

## [4.3.0] - 2020-10-02

### Changed

 - Camera models now use the ALE library to interpolate states and orientations. Users will likely see very small changes in sensor ephemerides. These were tested and are within existing interpolation tolerances. [#2370](https://github.com/USGS-Astrogeology/ISIS3/issues/2370)
 - The isis3VarInit script is now just called isisVarInit and allows for more robust paths. [#3945](https://github.com/USGS-Astrogeology/ISIS3/pull/3945)
 - Isis2raw will now output straight to a 32bit file (no stretch) when stretch is set to None and bittype is set to 32bit. [#3878](https://github.com/USGS-Astrogeology/ISIS3/issues/3878)
 - Findimageoverlaps can now have calculations and writes happen at the same time or sequentially. [#4047](https://github.com/USGS-Astrogeology/ISIS3/pull/4047)
 - IsisPreferences has had the default path to Osirisrex updated to point to new kernels released by NAIF [#4060](https://github.com/USGS-Astrogeology/ISIS3/issues/4060)

### Fixed

 - Fixed some line scan images using the incorrect state interpolation. [#3928](https://github.com/USGS-Astrogeology/ISIS3/issues/3928)
 - The ISIS library now has the correct version suffix. [#3365](https://github.com/USGS-Astrogeology/ISIS3/issues/3365)
 - Equalizer now reports the correct equation and values used to perform the adjustment. [#3987](https://github.com/USGS-Astrogeology/ISIS3/issues/3987)
 - Map2cam now works correctly when specifying bands for input cubes. [#3856](https://github.com/USGS-Astrogeology/ISIS3/issues/3856)

 - mro/hideal2pds app now writes the correct SAMPLE_BIT_MASK values to the output label. [#3978](https://github.com/USGS-Astrogeology/ISIS3/issues/3978)

 - For Histograms in ISIS, updated the math for calculating what bin data should be placed in and the min/max values of each bin to be more intuitive. In addition, the output of hist and cnethist were changed to display the min/max values of each bin instead of the middle pixel's DN. [#3882](https://github.com/USGS-Astrogeology/ISIS3/issues/3882)

### Added

 - A Gui Helper gear was added to hist to fill in the minimum and maximum parameters with what would have been automatically calculated. [#3880](https://github.com/USGS-Astrogeology/ISIS3/issues/3880)

- Added some Python programs (in isis/scripts/) to manage the authoritative .zenodo.json file
  which contains the ISIS authors, and to generate the AUTHORS.rst file from it.


## [4.2.0] - 2020-07-27

### Added

 - Added the ability to pass a list of coordinates to mappt similar to campt. [#3872](https://github.com/USGS-Astrogeology/ISIS3/issues/3872)

## [4.1.1] - 2020-06-15

### Changed

 - stats now reports "N/A" for pixel value statistics when the input cube contains only special pixels. [#3870](https://github.com/USGS-Astrogeology/ISIS3/issues/3870)

### Fixed

 - Mosaics whose tracking band was removed but still have a tracking table no longer repeatedly raise warnings in qview [#3685](https://github.com/USGS-Astrogeology/ISIS3/issues/3685)
 - Several programs now properly close cube files between each step of a batchlist run. [#3841](https://github.com/USGS-Astrogeology/ISIS3/issues/3841) & [#3842](https://github.com/USGS-Astrogeology/ISIS3/issues/3842)
 - Fixed several projection values in exported PDS4 PolarStereographic image labels. [#3869](https://github.com/USGS-Astrogeology/ISIS3/issues/3869)
 - qview now only updates stretches when they change. This was causing significant slow-down with some data sets. [#3854](https://github.com/USGS-Astrogeology/ISIS3/issues/3854)
 - isis2ascii uses more intelligent spacing when the input cube has negative pixel values. [#3859](https://github.com/USGS-Astrogeology/ISIS3/issues/3859)

## [4.1.0] - 2020-05-07

### Added

 - Preliminary Europa Imaging System support. [#3661](https://github.com/USGS-Astrogeology/ISIS3/issues/3661)
 - Stretches can now be saved to cube files so that they always open with a specific stretch in qview and the like. [#3717](https://github.com/USGS-Astrogeology/ISIS3/issues/3717)
 - kaguyatc2isis now supports data from the JAXA online archive. [#3670](https://github.com/USGS-Astrogeology/ISIS3/issues/3670) & [#1764](https://github.com/USGS-Astrogeology/ISIS3/issues/1764)
 - hyb2onc2isis now supports data from the JAXA online archive. [#3698](https://github.com/USGS-Astrogeology/ISIS3/issues/3698)
 - Smithed kernels for Cassini ISS observations of Enceladus are now available in the data area. [#3669](https://github.com/USGS-Astrogeology/ISIS3/issues/3669)
 - cam2map now outputs NULL pixels in occluded regions when using a 2.5D DEM. [#3757](https://github.com/USGS-Astrogeology/ISIS3/issues/3757)
 - jigsaw can now be configured to solve for different parameters for different observations and/or instruments in the same solution. [#3369](https://github.com/USGS-Astrogeology/ISIS3/issues/3369)

### Changed

 - Improved vimscal for Jupiter and Saturn spectra. [#3357](https://github.com/USGS-Astrogeology/ISIS3/issues/3357)
 - Changed the environment variables that specify where the data and test data areas are located from $ISIS3DATA and $ISIS3TESTDATA to just $ISISDATA and $ISISTESTDATA. [#3727](https://github.com/USGS-Astrogeology/ISIS3/issues/3727)
 - Moved the data used by applications for things like icons, templates, and translations into the distribution. The base data area is no longer needed to run some applications. [#3727](https://github.com/USGS-Astrogeology/ISIS3/issues/3727)
 - Improved pds2isis's documentation describing how it handles special pixel values. [#3648](https://github.com/USGS-Astrogeology/ISIS3/issues/3648)
 - Improved slpmap's documentation. [#3562](https://github.com/USGS-Astrogeology/ISIS3/issues/3562)

### Fixed

 - Enlarge no correctly reports output lines and samples. [#3659](https://github.com/USGS-Astrogeology/ISIS3/issues/3659)
 - The spiceinit web server no longer errors when using ISIS4+. [#3725](https://github.com/USGS-Astrogeology/ISIS3/issues/3725)
 - Fixed how some keywords were read in hyb2onccal. [#3698](https://github.com/USGS-Astrogeology/ISIS3/issues/3698)

## [4.0.1] - 2020-03-04

### Fixed

 - Grid lines in qmos now properly update when the map projection changes. [#3573](https://github.com/USGS-Astrogeology/ISIS3/issues/3573)
 - ddd2isis now properly ingests 32-bit files. [#3715](https://github.com/USGS-Astrogeology/ISIS3/issues/3715)

## [4.0.0] - 2020-02-04

### Changed

 - Moved much of the logic in spiceinit into a new library called [ALE](https://github.com/USGS-Astrogeology/ale). [#2370](https://github.com/USGS-Astrogeology/ISIS3/issues/2370)
 - lronaccal no longer requires SPICE data available locally if it has been attached to the input cube with spiceinit. [#1790](https://github.com/USGS-Astrogeology/ISIS3/issues/1790)

### Fixed

 - qview no longer crashes when a band has only one pixel value in it. [#3323](https://github.com/USGS-Astrogeology/ISIS3/issues/3323)
 - photomet now correctly reads array valued PvlKeywords. [#3608](https://github.com/USGS-Astrogeology/ISIS3/issues/3608)

## [3.9.1] - 2019-11-19

### Changed

 - Improved hijitreg's documentation.
 - Improved camdev's documentation.
 - voy2isis is less picky about the exact instrument name.
 - photomet now raises a warning when parameters are missing.
 - makeflat and cisscal now use ISIS variables in their report files intead of absolute paths.

### Fixed

 - qmos now properly handles both PositiveWest and PositiveEast longitude domains.
 - jigsaw's bundleout.txt output file now has proper alignment in its tables.
 - cam2map no longer runs indefinitely with certain parameters.
 - marci2isis no longer writes out a cube when it fails.
 - marcical now uses the correct variable exposure time from the image label instead of a constant exposure time.

## [3.9.0] - 2019-09-27

### Added

 - Users can now specify the output bit type for hideal2pds.
 - oblique resolution is now available in qview's advanced tracking tool.
 - oblique resolution statistics are now computed in caminfo.
 - PDS4 exported image labels now have more precise ground ranges.
 - PDS4 exported TGO CaSSIS image labels now contain additional fields.
 - kerneldbgen can now take an explicit list of kernel files instead of a directory and a filter.
 - FISH shell is now nominally supported.
 - kerneldbgen now works with kernels that have an extremely large number of intervals.

### Changed

 - cisscal now matches version 3.9.1 of the IDL program.
 - cisscal also now reports the matching IDL version.
 - Improved the format of jigsaw's bundleout.txt output file.
 - cam2map now uses better buffer sizes.
 - voycal now reports all missing coefficients instead of just the first one encountered.

## [3.8.1] - 2019-08-16

### Changed

 - Dawn FC's error message no longer tells users to bother Jeff Anderson.

### Fixed

 - caminfo now properly errors when passed a projected cube.
 - Filenames with months in them are now properly translated using the host system's local instead of US English.
 - findfeatures now properly resets the input images between algorithm runs when running in multi-algorithm mode.
 - qmos no longer hangs when drawing a grid over a projected image with positive west longitude domain.

## [3.8.0] - 2019-07-29

### Changed

 - Removed some dev tools from the installation environment.

### Fixed

 - Fixed ingestion of Rosetta VIRITIS lvl2 spectra.

## [3.7.1] - 2019-05-30

### Fixed

- Paths no longer break findfeature's algorithm parameter.

## [3.7.0] - 2019-04-30

### Added

 - Added tab completion for TCSH. [#3244](https://github.com/USGS-Astrogeology/ISIS3/pull/3244)
 - shadow now reports the pixel type of the output image.
 - phocube now reports right ascensions and declination for off-body pixels.
 - tgocassismos now supports mosaic tracking. [#2636](https://github.com/USGS-Astrogeology/ISIS3/issues/2636)

### Changed

 - cnetbin2pvl now always prints out the line and sample residual, even if they are 0. [#2698](https://github.com/USGS-Astrogeology/ISIS3/issues/2698)
 - Updated spiceinit's web server to work with ISIS3.5 and later.
 - Updated tgocassisrdrgen to the latest PDS4 standards. [#2635](https://github.com/USGS-Astrogeology/ISIS3/issues/2635)

### Fixed

 - gllssi2isis now properly attaches the original label to the ingested cube when running in summing mode. [#3226](https://github.com/USGS-Astrogeology/ISIS3/pull/3226)
 - findfeatures now reports an error instead of seg faulting when it tries to invert an empty matrix. [#557](https://github.com/USGS-Astrogeology/ISIS3/issues/557)
 - jigsaw now runs to completion when a measure does not project to a ground point with apriori parameters. [#2591](https://github.com/USGS-Astrogeology/ISIS3/issues/2591)
 - findrx now properly adds a history entry. [#3150](https://github.com/USGS-Astrogeology/ISIS3/issues/3150)
 - sumspice now properly adds a history entry.
 - Fixed a memory leak when using the Bullet library to intersect DSKs.
 - pds2hideal now returns a better error when attempting to export a compressed image.
 - Fixed summing mode keyword in tgocassis2isis. [#2634](https://github.com/USGS-Astrogeology/ISIS3/issues/2634)


## [3.6.2] - 2019-02-28

### Added

 - Multi-segment DSKs are now supported. [#2632](https://github.com/USGS-Astrogeology/ISIS3/issues/2632)

<!---
Below here are link definitions that compare each version against the last
version on github. The github comparison format is
{REPO_NAME}/compare/{NEW_VERSION_TAG}...{OLD_VERSION_TAG}

The unreleased comparison should always be
{REPO_NAME}/compare/{LAST_VERSION_TAG}...HEAD
-->

[unreleased]: https://github.com/USGS-Astrogeology/ISIS3/compare/6.0.0...HEAD
[6.0.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/5.0.2...6.0.0
[5.0.2]: https://github.com/USGS-Astrogeology/ISIS3/compare/5.0.1...5.0.2
[5.0.1]: https://github.com/USGS-Astrogeology/ISIS3/compare/5.0.0...5.0.1
[5.0.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/4.4.0...5.0.0
[4.4.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/4.3.0...4.4.0
[4.3.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/4.2.0...4.3.0
[4.2.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/4.1.1...4.2.0
[4.1.1]: https://github.com/USGS-Astrogeology/ISIS3/compare/4.1.0...4.1.1
[4.1.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/4.0.1...4.1.0
[4.0.1]: https://github.com/USGS-Astrogeology/ISIS3/compare/4.0.0...4.0.1
[4.0.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/3.9.1...4.0.0
[3.9.1]: https://github.com/USGS-Astrogeology/ISIS3/compare/3.9.0...3.9.1
[3.9.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/3.8.1...3.9.0
[3.8.1]: https://github.com/USGS-Astrogeology/ISIS3/compare/3.8.0...3.8.1
[3.8.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/3.7.1...3.8.0
[3.7.1]: https://github.com/USGS-Astrogeology/ISIS3/compare/3.7.0_0...3.7.1
[3.7.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/v3.6.2...3.7.0_0
[3.6.2]: https://github.com/USGS-Astrogeology/ISIS3/compare/3.6.1...v3.6.2
