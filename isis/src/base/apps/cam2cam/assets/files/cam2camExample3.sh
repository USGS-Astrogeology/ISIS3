#!/bin/sh

# Get example image acquired by OREX OCAMS during "Detailed Survey" phase\
dtype="map_iofL2b"\
fbase="20190509T180552S020"\

# Download the OSIRIS-REx image. Note wget can be installed using the command\
# "conda install wget"\
wget -P . "https://sbnarchive.psi.edu/pds4/orex/orex.ocams/data_calibrated/detailed_survey/$\{fbase\}_$\{dtype\}.fits"\

# Import into ISIS\
ocams2isis from="$\{fbase\}_$\{dtype\}.fits" to="$\{fbase\}_pck.cub"\
ocams2isis from="$\{fbase\}_$\{dtype\}.fits" to="$\{fbase\}_dtm.cub"\

spiceinit from="$\{fbase\}_pck.cub" shape=ellipsoid\
spiceinit from="$\{fbase\}_dtm.cub" shape=user model='$osirisrex/kernels/dsk/g_00880mm_alt_ptm_0000n00000_v020.bds' -pref=$ISISROOT/BulletEngineSelect.pref\

# Run cam2cam for each set\
cam2cam from="$\{fbase\}_pck.cub" match="$\{fbase\}_dtm.cub" to="$\{fbase\}_pck_to_dtm_def.cub"\
cam2cam from="$\{fbase\}_pck.cub" match="$\{fbase\}_dtm.cub" to="$\{fbase\}_pck_to_dtm_off.cub" offbody=true offbodytrim=false\
cam2cam from="$\{fbase\}_pck.cub" match="$\{fbase\}_dtm.cub" to="$\{fbase\}_pck_to_dtm_offtrim.cub" offbody=true offbodytrim=true\

cam2cam from="$\{fbase\}_dtm.cub" match="$\{fbase\}_pck.cub" to="$\{fbase\}_dtm_to_pck_def.cub"\
cam2cam from="$\{fbase\}_dtm.cub" match="$\{fbase\}_pck.cub" to="$\{fbase\}_dtm_to_pck_off.cub" offbody=true offbodytrim=false\
cam2cam from="$\{fbase\}_dtm.cub" match="$\{fbase\}_pck.cub" to="$\{fbase\}_dtm_to_pck_offtrim.cub" offbody=true offbodytrim=true\

