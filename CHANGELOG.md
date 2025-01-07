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

### Changed

### Fixed
- Fixed kaguyatc2isis invalid BandBin values [#5629](https://github.com/DOI-USGS/ISIS3/issues/5629)
- Fixed SpiceClient to handle redirect requests.

## [9.0.0] - 09-25-2024

### Added
- Added TOVECT output parameter which generate a geospatial CSV file with a VRT metadata sidecar file [#5571](https://github.com/DOI-USGS/ISIS3/issues/5571)  
- Added Vectorize to ProcessGroundPolygon library
- Added gtest files for the app and unit test 
- Added Chandrayaan2 template for isisimport
- Added majority replacement for reduce app [#5101](https://github.com/DOI-USGS/ISIS3/issues/5101).
- Added HRSC support in socetlinescankeywords [#5465](https://github.com/DOI-USGS/ISIS3/issues/5465)
- Added option to save and apply bundle adjustment values in `jigsaw` [#4474](https://github.com/DOI-USGS/ISIS3/issues/4474)
- Added the ability to pass column types in csv2table to set the column types in the resulting ISIS table [#5631](https://github.com/DOI-USGS/ISIS3/pull/5631)

### Changed
- Refactored the pixel2map app
- Updated pixel2map documentation
- Changed PVL parsing to no longer allow valueless keywords [#5573](https://github.com/DOI-USGS/ISIS3/pull/5573)
- Changed all `.trn` files with an `Auto`, `Debug` or `Optional` keyword with no value to have a value of `1` [#5573](https://github.com/DOI-USGS/ISIS3/pull/5573)
- Changed 'User Parameters' group in camstats to UserParameters for PVL compliance [#5625](https://github.com/DOI-USGS/ISIS3/issues/5625).

### Fixed
- Fixed noseam bug where a debugging output statement was inadvertently left in noseam.cpp.
Issue: [5660](https://github.com/DOI-USGS/ISIS3/issues/5660)
- Fixed jigsaw bugs in which RADIUS is handled incorrectly in the jigsaw gui and in the bundleout.txt
file. Slightly modified the FunctionalTestJigsawBundleXYZ ctest accordingly. Issue: [5642](https://github.com/DOI-USGS/ISIS3/issues/5642)
- Fixed a bug in isisminer in which bad (e.g. self-intersecting) polygon geometries were not treated properly. Added pertinent unit tests to GisGeometry and Strategy classes. Issue: [5612](https://github.com/DOI-USGS/ISIS3/issues/5612)
- Fixed a bug in kaguyasp2isis that doesn't work for data with a detached label.
- Fixed FunctionalTestCamstatsDefaultParameters test by increasing the runtime speed [#5459](https://github.com/DOI-USGS/ISIS3/issues/5459)
- Fixed XmlToJson namespaced key conversion [#5652](https://github.com/DOI-USGS/ISIS3/pull/5652)
- Fixed PHOTOMET not accepting backplanes [#5281](https://github.com/DOI-USGS/ISIS3/issues/5281)
- Fixed dstripe parallel test failing by converting tests to gtests [#5613](https://github.com/DOI-USGS/ISIS3/issues/5613)
- Fixed autoseed SeedDomain = SampleLine only working for first overlap [#5673](https://github.com/DOI-USGS/ISIS3/issues/5673)

## [8.3.0] - 2024-09-30

### Added
- Added backplane options for SunIllumination and SurfaceObliqueDetectorResolution to phocube [#5467](https://github.com/DOI-USGS/ISIS3/issues/5467)
- Added *.history to the ignore list in downloadIsisData

### Changed
- Noseam has been refactored to be callable; old Makefile test has been removed and replaced by a gtest. Issue: [#5599](https://github.com/USGS-Astrogeology/ISIS3/issues/5599)
- Explode has been refactored to be callable; old Makefile test has been removed and replaced by a gtest. Issue: [#5557](https://github.com/USGS-Astrogeology/ISIS3/issues/5557)
- Isisminer has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5516](https://github.com/USGS-Astrogeology/ISIS3/issues/5516)
- Algebra has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5594](https://github.com/USGS-Astrogeology/ISIS3/issues/5594)
- Photrim has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5581](https://github.com/USGS-Astrogeology/ISIS3/issues/5581)
- Bandtrim has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5571](https://github.com/USGS-Astrogeology/ISIS3/issues/5571)
- Modified kaguyasp2isis to work with new (detached) data [#5436](https://github.com/DOI-USGS/ISIS3/issues/5436)
- Added jigsaw error message for csminit'd images without csm parameters[#5486](https://github.com/DOI-USGS/ISIS3/issues/5486)
- Changed `qwt` dependency version to 6.2.0 or below [#5498](https://github.com/DOI-USGS/ISIS3/issues/5498)
- Pinned `suitesparse` dependency version to maximum not including 7.7.0 [#5496](https://github.com/DOI-USGS/ISIS3/issues/5496)


### Fixed
- Fixed a bug in noproj.cpp which left a persisent lbl file after running noproj. [#5577] (https://github.com/DOI-USGS/ISIS3/issues/5577)
- Fixed a bug in QVIEW's FindTool in which camera was prioritized over projction [#5508](https://github.com/DOI-USGS/ISIS3/issues/5508)
- Fixed a bug in PolygonTools in which the program exited before attempting to fix an invalid Polygon [#5520](https://github.com/DOI-USGS/ISIS3/issues/5520)
- Fixed a bug in QVIEW's Stretch tool where the default min/max type was not an available option [#5289](https://github.com/DOI-USGS/ISIS3/issues/5289)
- Fixed a bug in QVIEW where images would double load if loaded from the commandline [#5505](https://github.com/DOI-USGS/ISIS3/pull/5505)
- Fixed qmos segfault under GEOS 3.12 [#5539](https://github.com/DOI-USGS/ISIS3/issues/5539)
- Fixed CSMSerialNumber.trn typo [#5485](https://github.com/DOI-USGS/ISIS3/issues/5485)
- Fixed CSMSerialNumber.trn to be PVL compliant [#5484](https://github.com/DOI-USGS/ISIS3/issues/5484)
- Fixed hrsc2isis to support level 3 hrsc images [#5497](https://github.com/DOI-USGS/ISIS3/issues/5497)
- Fixed gllssi2isis to support V1.1 data [#5396](https://github.com/DOI-USGS/ISIS3/issues/5396)

### Added
- Added versioned default values to lrowacphomap's PHOALGO and PHOPARCUBE parameters and updated lrowacphomap to handle them properly. [#5452](https://github.com/DOI-USGS/ISIS3/pull/5452)

## [8.2.0] - 2024-04-18

### Changed
- Changed the default spiceinit url to https://astrogeology.usgs.gov/apis/ale/v0.9.1/spiceserver/ and added deprecation warning for use of https://services.isis.astrogeology.usgs.gov/cgi-bin/spiceinit.cgi url. [#5327](https://github.com/USGS-Astrogeology/ISIS3/issues/5327)
- Skypt has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5443](https://github.com/USGS-Astrogeology/ISIS3/issues/5443)

### Fixed
- Fixed a bug in which capital E was problematic for OSX / Ubuntu [#5466](https://github.com/DOI-USGS/ISIS3/pull/5466)
- Fixed bug in which not all references to 'version' file were replaced with new 'isis_version.txt' file [#5374](https://github.com/DOI-USGS/ISIS3/issues/5374)
- Fixed a bug in which the IrregularBodyCameraGroundMap unit test was removed but not the associated truth file. [#5461](https://github.com/DOI-USGS/ISIS3/issues/5461)
- Fixed a bug in which the histogram tool used the entire image to calculate bin size, which caused an issue with high dynamic range images. [#5371](https://github.com/DOI-USGS/ISIS3/issues/5371)
- Fixed a bug in which 'version' file was compiled as source and prevented subsequent ISIS recompilation [#5374](https://github.com/DOI-USGS/ISIS3/issues/5374)
- Fixed <i>noproj</i> bug where some temporary files were not deleted after call to cam2cam.  Issue: [#4813](https://github.com/USGS-Astrogeology/ISIS3/issues/4813)
- Fixed <i>noproj</i> bug where missing shapemodel-related keywords (RayTraceEngine, BulletParts, Tolerance) are dropped when the output label is created. This resulted in the Bullet collision detection engine not being used. Issue: [#5377](https://github.com/USGS-Astrogeology/ISIS3/issues/5377)
- Fixed ISIS failing to expand env variables with an "_" in them. [#5402](https://github.com/DOI-USGS/ISIS3/pull/5402)
- Fixed <i>noproj</i> bug where match cube not NULL when not specifying MATCH parameter [#5381](https://github.com/USGS-Astrogeology/ISIS3/issues/5381)
- Fixed findFeaturesSegment trying to merge networks when only 1 segmented network was successfully created [5416](https://github.com/DOI-USGS/ISIS3/issues/5416)
- Fixed rclone.conf/downloadIsisData pointing to old naif kernel location, updated to archived locations and https [5394](https://github.com/DOI-USGS/ISIS3/issues/5394)
- Fixed a parsing error in FitsToJson when an empty comment was reached. [#5469](https://github.com/DOI-USGS/ISIS3/pull/5469)
- Fixed TrackTool, FindTool, and AdvancedTrackTool not reporting RA and DEC for images targeting the Sky. [#5409](https://github.com/DOI-USGS/ISIS3/pull/5409)

### Added
- Added 8 new functions to the Sensor Utility Library: Slant Distance, Target Center Distance, Right Ascension Declination, Local Solar Time, Line Resolution, Sample Resolution, Pixel Resolution, and Solar Longitude.
- Added new option in `ctxcal` to use monthly computed flatfield files for "Frown" removal in CTX images. [#5338](https://github.com/DOI-USGS/ISIS3/pull/5338)
- CSMCamera can now read and use the body rotation from ALE produced ISDs [#5072](https://github.com/DOI-USGS/ISIS3/pull/5072)
- CSMSkyMap added to CSMCamera for use with local rover projections in ISIS [#5072](https://github.com/DOI-USGS/ISIS3/pull/5072)

- Cneteditor can now display point coordinates and sigmas in either lat/lon/radius or XYZ
format. A priori coordinate sigmas can now be edited in both formats. New filters have been implemented for coordinates and sigmas in XYZ. [#5421](https://github.com/DOI-USGS/ISIS3/issues/5421)

- Added new Hayabusa2 translation for `SpacecraftName` to accept `HAYABUSA2` [#5395](https://github.com/DOI-USGS/ISIS3/issues/5395)
- Added ALLOWERROR parameter to campt [#5393](https://github.com/DOI-USGS/ISIS3/pull/5393)
- OSIRIS-REx Tagcams instrument support, tests, and test data added [#5424](https://github.com/DOI-USGS/ISIS3/issues/5424)
- Added min/max type drop-down selection option in QView's Stretch tool [#5289](https://github.com/DOI-USGS/ISIS3/issues/5289)

## [8.0.3] - 2024-04-18

### Changed
- Changed the default spiceinit url to https://astrogeology.usgs.gov/apis/ale/v0.9.1/spiceserver/ and added deprecation warning for use of https://services.isis.astrogeology.usgs.gov/cgi-bin/spiceinit.cgi url. [#5327](https://github.com/USGS-Astrogeology/ISIS3/issues/5327)
- Skypt has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5443](https://github.com/USGS-Astrogeology/ISIS3/issues/5443)

### Fixed
- Fixed a bug in which capital E was problematic for OSX / Ubuntu [#5466](https://github.com/DOI-USGS/ISIS3/pull/5466)
- Fixed bug in which not all references to 'version' file were replaced with new 'isis_version.txt' file [#5374](https://github.com/DOI-USGS/ISIS3/issues/5374)
- Fixed a bug in which the IrregularBodyCameraGroundMap unit test was removed but not the associated truth file. [#5461](https://github.com/DOI-USGS/ISIS3/issues/5461)
- Fixed a bug in which the histogram tool used the entire image to calculate bin size, which caused an issue with high dynamic range images. [#5371](https://github.com/DOI-USGS/ISIS3/issues/5371)
- Fixed a bug in which 'version' file was compiled as source and prevented subsequent ISIS recompilation [#5374](https://github.com/DOI-USGS/ISIS3/issues/5374)
- Fixed <i>noproj</i> bug where some temporary files were not deleted after call to cam2cam.  Issue: [#4813](https://github.com/USGS-Astrogeology/ISIS3/issues/4813)
- Fixed <i>noproj</i> bug where missing shapemodel-related keywords (RayTraceEngine, BulletParts, Tolerance) are dropped when the output label is created. This resulted in the Bullet collision detection engine not being used. Issue: [#5377](https://github.com/USGS-Astrogeology/ISIS3/issues/5377)
- Fixed ISIS failing to expand env variables with an "_" in them. [#5402](https://github.com/DOI-USGS/ISIS3/pull/5402)
- Fixed <i>noproj</i> bug where match cube not NULL when not specifying MATCH parameter [#5381](https://github.com/USGS-Astrogeology/ISIS3/issues/5381)
- Fixed rclone.conf/downloadIsisData pointing to old naif kernel location, updated to archived locations and https [5394](https://github.com/DOI-USGS/ISIS3/issues/5394)
- Fixed a parsing error in FitsToJson when an empty comment was reached. [#5469](https://github.com/DOI-USGS/ISIS3/pull/5469)

### Added
- Added new Hayabusa2 translation for `SpacecraftName` to accept `HAYABUSA2` [#5395](https://github.com/DOI-USGS/ISIS3/issues/5395)

## [8.0.2] - 2023-12-05

### Changed
- Cnetedit has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5346](https://github.com/USGS-Astrogeology/ISIS3/issues/5346), 
- Cnetdiff has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5322](https://github.com/USGS-Astrogeology/ISIS3/issues/5322), 
### Fixed
- Bug fix for Cnetthinner app resolving divide by zero in CnetManager.cpp. Issue: [#5354](https://github.com/USGS-Astrogeology/ISIS3/issues/5354), 
- Updated photomet MinnaertEmpirical model to support photemplate-style PVL format [#3621](https://github.com/DOI-USGS/ISIS3/issues/3621)
- Fixed target name translation for any dawn images with target "4 CERES" [#5294](https://github.com/DOI-USGS/ISIS3/pull/5294)
- Fixed exception pop ups in qview when viewing images created using the CSM Camera [#5259](https://github.com/DOI-USGS/ISIS3/pull/5295/files)
- Fixed shadowtau input file parseing errors when using example file [#5316](https://github.com/DOI-USGS/ISIS3/pull/5316)
- Fixed ProgramLauncher failing while reporting errors from launched programs [#5331](https://github.com/DOI-USGS/ISIS3/pull/5331)
- Fixed high/low filter functionality in trimfilter [#5311](https://github.com/DOI-USGS/ISIS3/issues/5311)
- Fixed downloadIsisData script excluding needed files in the calibration folder [#5272](https://github.com/DOI-USGS/ISIS3/issues/5272)

## [8.1.0] - 2024-01-08

### Changed
- Cnetthinner has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5354](https://github.com/USGS-Astrogeology/ISIS3/issues/5354), 
- Cnetedit has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5346](https://github.com/USGS-Astrogeology/ISIS3/issues/5346), 
- Cnetdiff has been refactored to be callable; old Makefile tests have been removed and replaced by gtests. Issue: [#5322](https://github.com/USGS-Astrogeology/ISIS3/issues/5322), 
- Removed the `.py` extention from the _isisdataeval_ tool `isisdata_mockup` for consistency and install it in $ISISROOT/bin; added the `--tojson` and `--hasher` option to _isisdata_mockup_ tool improve utility; updated the tool `README.md` documentation to reflect this change, removed help output and trimmed example results;  fixed paths to test data in `make_isisdata_mockup.sh`. [#5163](https://github.com/DOI-USGS/ISIS3/pull/5163)
- Significantly refactored FASTGEOM processing in <i>findfeatures</i> to accommodate stability and functionality. The scope of the algorithm was taken out of the ImageSource class and isolated to support this feature. [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Report better information regarding the behavior of <i>findfeatures</i>, FASTGEOM algorithms, and creation of the output network. [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Modified tgocassisstitch to optionally allow either a outputprefix or an
  outputsuffix, both, or neither for naming convention purposes. [#5162](https://github.com/DOI-USGS/ISIS3/pull/5162)
- Changed the default PCK load behavior to try and load mission specific PCKs, if that fails we default to the base PCKs [#5335](https://github.com/DOI-USGS/ISIS3/pull/5335)
- Disabled option to use web=true when running spiceinit with HRSC images. [#5223](https://github.com/DOI-USGS/ISIS3/issues/5223)
- Seperated `normal` and `localNormal` in ISIS shapemodels, now the local normal is stored in its own member variable with setters and getters. [#5345](https://github.com/DOI-USGS/ISIS3/pull/5345)
- Set build option `pybindings=ON` in `build.sh` to turn on python bindings. [#5389](https://github.com/DOI-USGS/ISIS3/pull/5389)
- Updated Ale to version 0.10.0 [#5399](https://github.com/DOI-USGS/ISIS3/pull/5399)

### Added
- Added rclone to run dependencies in meta.yaml [#5183](https://github.com/DOI-USGS/ISIS3/issues/5183)
- Add new program option <b>TONOGEOM</b> to <i>findfeatures</i> that logs captures geometry errors in the FASTGEOM algorithm and records them to the file provided in this parameter. These images are excluded from the matching process. References [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Added a new Radial FASTGEOM transform mapping algorithm to <i>findfeatures</i> to address performance problems with the Grid algorithm. This is now the default algorithm if none are selected by the user (see the new <b>GLOBALS</b> parameter to specify the algorithm) [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Added new parameter <b>GLOBALS</b> to make parameterization of <i>findfeatures</i> behavior significantly easier and convenient. Wrote significant documentation for the parameter and provide several examples showing its use. [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Added two new examples demonstrating/documenting the use of FASTGEOM algorithm, parameterization using <b>GLOBALS</b> and creation of a regional mosaic using <i>findfeatures</i>. [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Added new option <b>GEOMSOURCE=BOTH</b> to <i>findfeatures</i> to check both the MATCH and FROM/FROMLIST images for valid control measure geometry to produce better networks and prevent downstream processing errors. Ignore points that end up with no valid measures (but can be retained with use of <b>PreserveIgnoredControl</b> via GLOBALS parameterization). [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Added new gtests for <i>findfeatures</i> that replaces all the old application tests. These tests are <i>FunctionalTestFindfeaturesFastGeomDefault</i>, <i>FunctionalTestFindfeaturesFastGeomRadialConfig</i>, <i>FunctionalTestFindfeaturesFastGeomGridDefault</i> and <i>FunctionalTestFindfeaturesFastGeomGridConfig</i>. [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Added an optional cubename parameter to tgocassisstitch which lets the user
  override the timestamp style naming convention of the output cube with their
  own name; if not specified retains existing behavior [#5125](https://github.com/USGS-Astrogeology/ISIS3/issues/5162)
- Added new parameters <b>ONERROR</b>, <b>ERRORLOG</b>, and <b>ERRORLIST</b> to <i>mosrange</i> to provide better control over error behavior and provide diagnostics when problems are encountered processing the input file list.[#3606](https://github.com/DOI-USGS/ISIS3/issues/3606)
- Added ability to delegate calculation of nadir pointing to ALE [#5117](https://github.com/USGS-Astrogeology/ISIS3/issues/5117)
- Added --no-kernels flag to downloadIsisData [#5264](https://github.com/DOI-USGS/ISIS3/issues/5264)
- Added notice for depreciation to PvlObject::addLogGroup function which will be depreciated in favor of Application::appendAndLog(https://github.com/DOI-USGS/ISIS3/issues/5310)
- Added new csm plugins path to IsisPreferences [#5397](https://github.com/DOI-USGS/ISIS3/pull/5397)

### Deprecated

### Removed

### Fixed
- Bug fix for Cnetthinner app resolving divide by zero in CnetManager.cpp. Issue: [#5354](https://github.com/USGS-Astrogeology/ISIS3/issues/5354) 
- Updated photomet MinnaertEmpirical model to support photemplate-style PVL format [#3621](https://github.com/DOI-USGS/ISIS3/issues/3621)
- Fix matrix inversion errors in <i>findfeatures</i> due to bad FASTGEOM matrix transforms using a more robust implementation to detect these errors and throw exceptions. Images with these errors are captured and logged to the <b>TONOTMATCHED</b> file. Fixes [#4639](https://github.com/DOI-USGS/ISIS3/issues/4639)
- Fixed <i>findfeatures</i> use of projected mosaics with correct check for <b>TargetName</b> in the Mapping labels. [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Fixed a bug in the <i>cnetwinnow</i> test that did not clean/remove it during test runs.
- Fixed <i>findfeatures</i> instantiation and use of projection classes to correctly return geometry data from projected images and mosaics. [#4772](https://github.com/DOI-USGS/ISIS3/issues/4772)
- Fixed target name translation for any dawn images with target "4 CERES" [#5294](https://github.com/DOI-USGS/ISIS3/pull/5294)
- Fixed exception pop ups in qview when viewing images created using the CSM Camera [#5259](https://github.com/DOI-USGS/ISIS3/pull/5295/files)
- Fixed shadowtau input file parseing errors when using example file [#5316](https://github.com/DOI-USGS/ISIS3/pull/5316)
- Fixed ProgramLauncher failing while reporting errors from launched programs [#5331](https://github.com/DOI-USGS/ISIS3/pull/5331)
- Fixed high/low filter functionality in trimfilter [#5311](https://github.com/DOI-USGS/ISIS3/issues/5311)
- Fixed downloadIsisData script excluding needed files in the calibration folder [#5272](https://github.com/DOI-USGS/ISIS3/issues/5272)
- Fixed tgocassis2isis cas_cal_sc ingest errors and added error for cas_raw_sc images [#5413](https://github.com/DOI-USGS/ISIS3/issues/5413)

## [8.0.1] - 2023-08-23

### Fixed
- Updated History constructor to check for invalid BLOB before copying History BLOB to output cube [#4966](https://github.com/DOI-USGS/ISIS3/issues/4966)
- Fixed downloadIsisData script copying existing files with every rclone download [#5245](https://github.com/DOI-USGS/ISIS3/issues/5245)
- Fixed gaussstretch segmentation fault error and refactored gaussstretch files/tests to use gtest [#5240](https://github.com/DOI-USGS/ISIS3/issues/5240)
- Fixed `cubeit` attribute error to allow attribute specification on the output cube filename [#5234](https://github.com/DOI-USGS/ISIS3/issues/5234)
- Fixed `campt` to handle input band selection attribute correctly [#5234](https://github.com/DOI-USGS/ISIS3/issues/5235)
- Fixed problem networks segfaulting in jigsaw by updating cholmod 32 bit calls to 64 bit calls [#5173](https://github.com/DOI-USGS/ISIS3/issues/5173) and [#5176](https://github.com/DOI-USGS/ISIS3/issues/5176)

## [8.0.0] - 2023-04-19

### Changed
- Updated download location for Dawn source files to include updated pck from HAMO Dawn mosaic [#4001](https://github.com/USGS-Astrogeology/ISIS3/issues/4001)
- Pinned cspice version to 67 [#5083](https://github.com/USGS-Astrogeology/ISIS3/issues/5083)
- Changed the `rsync` related commands in the ISIS SPICE Web Service document to `downloadIsisData` command
- Updated documentation for `tgocassisrdrgen` to be more descriptive and accurate.
- Updated Geos from version 3.7 to 3.9 [#3627](https://github.com/DOI-USGS/ISIS3/issues/3627)
- Updated Ale to version 0.9.1 [#5209](https://github.com/DOI-USGS/ISIS3/pull/5209)
- Changed buildCondaRelease.py numpy version from 1.23 to 1.22 to resolve spiceinit warning messages [#5224](https://github.com/DOI-USGS/ISIS3/issues/5224)

### Added
- Instructions on setting `channel_priority=flexible` for isis environment manually during installation [#5158](https://github.com/DOI-USGS/ISIS3/issues/5158)
- Added additional filters to downloadIsisData to reduce download of extraneous kernels [#5143](https://github.com/DOI-USGS/ISIS3/issues/5143)

### Deprecated

### Removed
- edrget application [#4665](https://github.com/DOI-USGS/ISIS3/issues/4665)

### Fixed
- Updated README.md to remove remaining references to downloading data from discontinued rsync server [#5152](https://github.com/DOI-USGS/ISIS3/issues/5152)
- Fixed users not being able to modify planetographic projections in qmos
- Modified spice::readValue to add check for numValuesRead to stop reading garbase values [#4928](https://github.com/USGS-Astrogeology/ISIS3/issues/4928)

## [7.2.0] - 2022-12-07

### Changed
- Updated Hayabusa2 mission name and updated translation files to make L2DShapeModelFileName, L2DPhaseFunctionFileName, L2CShapeModelFileName, and BandRegistration optional keywords.

### Added
- Added LatLonGrid Tool to Qview to view latitude and longitude lines if camera model information is present.
- Added a new application, _isisdataeval_, that validates/verifies ISISDATA installations and aids in the development and management of this resource. Users can use _isisdataeval_ to help resolve runtime problems related to ISIS kernels and calibration processing. In fact, it is designed to be used on any directory as it will create a detailed install volume inventory with file/volume hashes and is not restricted to ISIS use. [#5110](https://github.com/USGS-Astrogeology/ISIS3/issues/5110) [#5111](https://github.com/USGS-Astrogeology/ISIS3/pull/5111)

### Deprecated

### Fixed
- Fixed some wrong parameter types [#4780](https://github.com/USGS-Astrogeology/ISIS3/issues/4780)
- Added mapping group to track cube via processmosaic [#4810](https://github.com/USGS-Astrogeology/ISIS3/issues/4810)
- Fixed bugs in downloadIsisData script [#5024](https://github.com/USGS-Astrogeology/ISIS3/issues/5024)
- Fixed shadow shifting image by 2 pixels to the upper left corner. [#5035](https://github.com/USGS-Astrogeology/ISIS3/issues/5035)
- Fixed compiler warnings on ubuntu [#4911](https://github.com/USGS-Astrogeology/ISIS3/issues/4911)
- Fixes embree shape models not being from .BDS extension files [5064](https://github.com/USGS-Astrogeology/ISIS3/issues/5064)
- Fixed failing shapemodel parameters when bullet was the preferred ray tracing engine but could not be created. [#5062](https://github.com/USGS-Astrogeology/ISIS3/issues/5062)
- Fixed version for Qt to prevent depreciations. [#5070](https://github.com/USGS-Astrogeology/ISIS3/issues/5070)
- Fixed NaN velocities in spkwriter as a result of jigsaw position solves. [#4942](https://github.com/USGS-Astrogeology/ISIS3/issues/4942)

## [7.1.0] - 2022-07-27

### Changed
- Updated the LRO photometry application Lronacpho, to use by default the current 2019 photometric model (LROC_Empirical). The model's coefficients are found in the PVL file that exists in the LRO data/mission/calibration directory. If the old parameter file is provided, the old algorithm(2014) will be used. This functionality is desired for calculation comparisons. Issue: [#4512](https://github.com/USGS-Astrogeology/ISIS3/issues/4512), PR: [#4519](https://github.com/USGS-Astrogeology/ISIS3/pull/4519)
- Updated the LRO calibration application Lrowaccal to add a units label to the RadiometricType keyword of the Radiometry group in the output cube label if the RadiometricType parameter is Radiance. No functionality is changed if the RadiometricType parameter is IOF. Lrowaccal has also been refactored to be callable for testing purposes. Issue: [#4939](https://github.com/USGS-Astrogeology/ISIS3/issues/4939), PR: [#4940](https://github.com/USGS-Astrogeology/ISIS3/pull/4940)
- Changed how logs are reported so they no longer only printing at the end of the applications execution. [#4914](https://github.com/USGS-Astrogeology/ISIS3/issues/4914)
- Update marcical to include step 3 of the mission team's MARCI calibration process described [here](https://pds-imaging.jpl.nasa.gov/data/mro/mars_reconnaissance_orbiter/marci/mrom_1343/calib/marcical.txt). [#5004](https://github.com/USGS-Astrogeology/ISIS3/pull/5004)
- Updated center / width values for TGO CaSSIS as requested [here](https://github.com/USGS-Astrogeology/ISIS3/issues/5006)

### Added
- Improved functionality of msi2isis and MsiCamera model to support new Eros dataset, including support for Gaskell's SUMSPICE files that adjust timing, pointing and spacecraft position ephemeris. [#4886](https://github.com/USGS-Astrogeology/ISIS3/issues/4886)
- Added a new application, framestitch, for stitching even and odd push frame images back together prior to processing in other applications. [4924](https://github.com/USGS-Astrogeology/ISIS3/issues/4924)
- Re-added and refactored the LRO photometry application lrowacphomap to be callable for testing purposes. Issue: [#4960](https://github.com/USGS-Astrogeology/ISIS3/issues/4960), PR: [#4961](https://github.com/USGS-Astrogeology/ISIS3/pull/4961)
- Added check to determine if poles were a valid projection point in ImagePolygon when generating footprint for a map projected image. [#4390](https://github.com/USGS-Astrogeology/ISIS3/issues/4390)
- Added changes to lronaccal to use time-dependent dark files for dark correction and use of specific dark files for images with exp code of zero. Also added GTests for lronaccal and refactored code to make callable. Added 3 truth cubes to testing directory.  PR[#4520](https://github.com/USGS-Astrogeology/ISIS3/pull/4520)
- Added failure for images with missing original label in caminfo. [#4817](https://github.com/USGS-Astrogeology/ISIS3/pull/4817)

### Deprecated

### Fixed
- Added check to determine if poles were a valid projection point in ImagePolygon when generating footprint for a map projected image. [#4390](https://github.com/USGS-Astrogeology/ISIS3/issues/4390)
- Fixed the Mars Express HRSC SRC camera and serial number to use the StartTime instead of the StartClockCount  [#4803](https://github.com/USGS-Astrogeology/ISIS3/issues/4803)
- Fixed algorithm for applying rolling shutter jitter. Matches implementation in USGSCSM.
- Fixed isis2std incorrectly outputing signed 16 bit tiff files. [#4897](https://github.com/USGS-Astrogeology/ISIS3/issues/4897)
- Fixed CNetCombinePt logging functionality such that only merged points are included in the log. [#4973](https://github.com/USGS-Astrogeology/ISIS3/issues/4973)
- Removed SpkCenterId functions in Cassini camera models due to spkwriter writing positions of Cassini relative to Titan but labeling
it in the kernel as the position relative to the Saturn Barycenter. [#4942](https://github.com/USGS-Astrogeology/ISIS3/issues/4942)
- Corrected issue where footprintinit would fail with a segmentation error. [4943](https://github.com/USGS-Astrogeology/ISIS3/issues/4943)


## [7.0.0] - 2022-02-11

### Changed
- Disabled SURF algorithm for findfeatures, latest version of opencv no longer provides SURF as part of the base library [#3885](https://github.com/USGS-Astrogeology/ISIS3/issues/3885)
- Changed caminfo's parameter default values for MAXEMISSION and MAXINCIDENCE to be
synchronized with footprintinit default values of the same parameters.
This corrects inconsistencies of footprint generation failing in caminfo
but passing in footprintinit. [#4651](https://github.com/USGS-Astrogeology/ISIS3/issues/4651).
- Changed the internal logic of ObliqueDataResolution() to use LocalEmission angle rather than Emission angle.
This will cause differences in output; new values will be more accurate because they use DEM rather than
ellipsoid. The cost to compute the local emissoin will increase by approximately x1.5. [#3600](https://github.com/USGS-Astrogeology/ISIS3/issues/3600)
- Changed website layout to better surface relevant documents.
[#4839](https://github.com/USGS-Astrogeology/ISIS3/pull/4839)
[#4847](https://github.com/USGS-Astrogeology/ISIS3/pull/4847)
[#4851](https://github.com/USGS-Astrogeology/ISIS3/pull/4851)
[#4852](https://github.com/USGS-Astrogeology/ISIS3/pull/4852)
[#4856](https://github.com/USGS-Astrogeology/ISIS3/pull/4856)
[#4865](https://github.com/USGS-Astrogeology/ISIS3/pull/4865)
[#4859](https://github.com/USGS-Astrogeology/ISIS3/pull/4859)
[#4872](https://github.com/USGS-Astrogeology/ISIS3/pull/4872)
[#4871](https://github.com/USGS-Astrogeology/ISIS3/pull/4871)

### Added
- Added the USECAMSTATSTBL option to caminfo. This allows caminfo to extract existing
camera statistics from the CameraStatistics Table of the input cube instead
of recalculating CameraStatistics. Updated caminfo to output all CameraStatistics
Keywords when running CAMSTATS.  [#3605](https://github.com/USGS-Astrogeology/ISIS3/issues/3605).
- Added the ability to search filenames in measure's drop down boxes in Qnet Point Editor. [#4581](https://github.com/USGS-Astrogeology/ISIS3/issues/4581)
- Added slope, local normal, and ellipsoid normal calculations to phocube. [#3635](https://github.com/USGS-Astrogeology/ISIS3/issues/3645)
- Added additional translation files for TGO CaSSiS in order to support PSA compliant labels. [#4567](https://github.com/USGS-Astrogeology/ISIS3/issues/4567)
- Added support for KaguyaTC SP Support data ingest. [#4668](https://github.com/USGS-Astrogeology/ISIS3/issues/4668)
- Added examples to the jigsaw documentation. [#4718](https://github.com/USGS-Astrogeology/ISIS3/issues/4718)
- Added ALLDNS option to phocube. [#3877](https://github.com/USGS-Astrogeology/ISIS3/issues/3877)
- Added import templates for isisimport, Cassini ISS, Cassini Vims, Kaguya TC Kaguya MI, Dawn FC, Dawn VIR, LROC NAC, LO HRC, MGS MOC, MER MI, MRO CTX, Rosetta Osiris, Viking VIS [#4606](https://github.com/USGS-Astrogeology/ISIS3/issues/4606)
- Added export templates for isisexport, LROC NAC EDR [#4606](https://github.com/USGS-Astrogeology/ISIS3/issues/4606)
- Added optional JSON data output parameter, DATA, for debugging template engine failures [#4606](https://github.com/USGS-Astrogeology/ISIS3/issues/4606)
- Added new documentatin for contributing code.
[#4859](https://github.com/USGS-Astrogeology/ISIS3/pull/4859)
[#4871](https://github.com/USGS-Astrogeology/ISIS3/pull/4871)
- Added versioning to website documentation.
[#4852](https://github.com/USGS-Astrogeology/ISIS3/pull/4852)
[#4872](https://github.com/USGS-Astrogeology/ISIS3/pull/4872)

### Deprecated
- Deprecated edrget as discussed in [#3313](https://github.com/USGS-Astrogeology/ISIS3/issues/3313).

### Fixed
- Fixed Maptrim failures when mode=both for PositiveWest longitude direction. [#4646](https://github.com/USGS-Astrogeology/ISIS3/issues/4646)
- Fixed the Vesta target name not being translated properly in dawnfc2isis. [#4638](https://github.com/USGS-Astrogeology/ISIS3/issues/4638)
- Fixed a bug where the measure residuals reported in the bundleout.txt file were incorrect. [#4655](https://github.com/USGS-Astrogeology/ISIS3/issues/4655)
- Fixed a bug where jigsaw would raise an error when solving for framing camera pointing in observation mode. [#4686](https://github.com/USGS-Astrogeology/ISIS3/issues/4686)
- Fixed slow runs of automos when the priority was BAND. [#4793](https://github.com/USGS-Astrogeology/ISIS3/pull/4793)
- Fixed qview crashing when attempting to load image DNs. [4818](https://github.com/USGS-Astrogeology/ISIS3/issues/4818)
- Fixed qnet crashing when entering an invalid image name in the measure selection box. [#4581](https://github.com/USGS-Astrogeology/ISIS3/issues/4581)
- Modified cnetcheck noLatLonCheck logic to correctly exclude ignored measures. [#4649](https://github.com/USGS-Astrogeology/ISIS3/issues/4649)
- Fixed bug where the original label was not attached to stereo HRSC images on import [#4816](https://github.com/USGS-Astrogeology/ISIS3/issues/4816)

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

[unreleased]: https://github.com/USGS-Astrogeology/ISIS3/compare/7.0.0...HEAD
[7.0.0]: https://github.com/USGS-Astrogeology/ISIS3/compare/6.0.0...7.0.0
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
