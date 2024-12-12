# Process this file with autoconf to produce a configure script.
# Initially based on configure.ac from https://github.com/appelmar/gdalcubes
# Copyright (c) 2018-2019 Marius Appel, License: MIT
# Much of the PROJ config was removed here because gdalraster only uses PROJ via
# GDAL headers. Still checks that PROJ is available to GDAL and handles optional
# local copy of the PROJ data directory. The PROJ datadir is now obtained from
# pkg-config if possible. Compiler invocations are now logged.
# subsequent changelog:
# 2024-03-03: test that GEOS is available to GDAL (required at gdalraster 1.10)
# 2024-03-12: restore proj_include_path and proj_lib_path
# 2024-03-17: rework GEOS test, include <ogr_api.h> in initial gdal_test.cpp

AC_INIT([gdalraster],[1.12.0],[chris.toney@usda.gov])
AC_LANG(C++)

# find R home and set CC/CFLAGS
: ${R_HOME=`R RHOME`}
if test -z "${R_HOME}"; then
  echo "could not determine R_HOME"
  exit 1
fi
RBIN="${R_HOME}/bin/R"
CC=`"${RBIN}" CMD config CC`
CFLAGS=`"${RBIN}" CMD config CFLAGS`
CPPFLAGS=`"${RBIN}" CMD config CPPFLAGS`
LDFLAGS=`"${RBIN}" CMD config LDFLAGS`
CXX=`"${RBIN}" CMD config CXX`
CXXFLAGS=`"${RBIN}" CMD config CXXFLAGS`

AC_SUBST(R_HOME)

AC_MSG_NOTICE([CC: ${CC}])
AC_MSG_NOTICE([CXX: ${CXX}])

INLIBS="${LIBS}"
INCPPFLAGS="${CPPFLAGS}"
INPKG_CPPFLAGS="${PKG_CPPFLAGS}"
INPKG_LIBS="${PKG_LIBS}"

# GDAL ########################################################################
# original version copied from
# https://github.com/r-spatial/sf/blob/master/configure.ac
# sf configure.ac: GDAL stuff largely copied from rgdal, (c) Roger Bivand

GDAL_CONFIG="gdal-config"

GDAL_CONFIG_SET="no"

AC_ARG_WITH([gdal-config],
    AS_HELP_STRING([--with-gdal-config=GDAL_CONFIG],[the location of gdal-config]),
           [gdal_config=$withval])
if test [ -n "$gdal_config" ] ; then
    GDAL_CONFIG_SET="yes"
    AC_SUBST([GDAL_CONFIG],["${gdal_config}"])
    AC_MSG_NOTICE(gdal-config set to $GDAL_CONFIG)
fi

if test ["$GDAL_CONFIG_SET" = "no"] ; then
  AC_PATH_PROG([GDAL_CONFIG], ["$GDAL_CONFIG"],["no"])
  if test ["$GDAL_CONFIG" = "no"] ; then
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([gdal-config not found or not executable.])
  fi
else
  AC_MSG_CHECKING(gdal-config exists)
  if test -r "${GDAL_CONFIG}"; then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([gdal-config not found - configure argument error.])
  fi
  AC_MSG_CHECKING(gdal-config executable)
  if test -x "${GDAL_CONFIG}"; then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
    AC_MSG_ERROR([gdal-config not executable.])
  fi
fi

AC_MSG_CHECKING(gdal-config usability)
if test `${GDAL_CONFIG} --version`; then
  GDAL_CPPFLAGS=`${GDAL_CONFIG} --cflags`
  GDAL_VERSION=`${GDAL_CONFIG} --version`
  GDAL_LIBS=`${GDAL_CONFIG} --libs`
  GDAL_DEP_LIBS=`${GDAL_CONFIG} --dep-libs`
  GDAL_DATADIR=`${GDAL_CONFIG} --datadir`
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  echo "Error: gdal-config not found"
  echo "The gdal-config script distributed with GDAL could not be found."
  echo "If you have not installed the GDAL libraries, you can"
  echo "download the source from  http://www.gdal.org/"
  echo "If you have installed the GDAL libraries, then make sure that"
  echo "gdal-config is in your path. Try typing gdal-config at a"
  echo "shell prompt and see if it runs. If not, use:"
  echo " --configure-args='--with-gdal-config=/usr/local/bin/gdal-config'"
  echo "with appropriate values for your installation."
  echo ""

  exit 1
fi

AC_MSG_NOTICE([GDAL: ${GDAL_VERSION}])
AC_MSG_CHECKING([GDAL version >= 3.1.0])

GDAL_MAJ_VER=`echo $GDAL_VERSION | cut -d "." -f1`
GDAL_MIN_VER=`echo $GDAL_VERSION | cut -d "." -f2`
GDAL_PATCH_VER=`echo $GDAL_VERSION | cut -d "." -f3`
if test ${GDAL_MAJ_VER} -lt 3 && test ${GDAL_MIN_VER} lt 1; then
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([gdalraster is not compatible with GDAL versions below 3.1.0])
else
   AC_MSG_RESULT(yes)
fi

AC_MSG_CHECKING([GDAL version != 3.6.0])
if test ${GDAL_MAJ_VER} = 3 && test ${GDAL_MIN_VER} = 6 && test ${GDAL_PATCH_VER} = 0 ; then
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([GDAL 3.6.0 was retracted, use a subsequent version])
else
  AC_MSG_RESULT(yes)
fi

AC_SUBST([PKG_CPPFLAGS], ["${INPKG_CPPFLAGS} ${GDAL_CPPFLAGS}"])
AC_SUBST([PKG_LIBS], ["${INPKG_LIBS} ${GDAL_LIBS}"])

# honor PKG_xx overrides
# for CPPFLAGS we will superfluously double R's flags
# since we'll set PKG_CPPFLAGS with this, but that shouldn't hurt
CPPFLAGS="${INCPPFLAGS} ${PKG_CPPFLAGS}"

gdalok=yes
AC_CHECK_HEADERS(gdal.h,,gdalok=no)
if test "${gdalok}" = no; then
   AC_MSG_ERROR([gdal.h not found in given locations.])
fi

NEED_DEPS=no
LIBS="${INLIBS} ${PKG_LIBS}"

AC_MSG_CHECKING(GDAL: linking with --libs only)
gdalok=no

# GDAL must be built with GEOS
[cat > gdal_test.cpp <<_EOCONF
#include <gdal.h>
#include <ogr_api.h>
#ifdef __cplusplus
extern "C" {
#endif
int main(void) {
  GDALAllRegister();
  bool ret = false;
  OGRGeometryH hGeom = OGR_G_CreateGeometry(wkbPoint);
  if (hGeom == NULL)
    return (ret == FALSE); // signals OGR failure
  OGR_G_SetPoint_2D(hGeom, 0, 0, 0);
  // If GDAL is built without the GEOS library, this function will
  // always return FALSE:
  ret = OGR_G_IsSimple(hGeom);
  OGR_G_DestroyGeometry(hGeom);
  return (ret == FALSE); // signals GEOS is not available via GDAL
}
#ifdef __cplusplus
}
#endif
_EOCONF]

AC_LINK_IFELSE(
   [AC_LANG_SOURCE([`cat gdal_test.cpp`])],
   gdalok=yes,
   gdalok=no
)
if test "${gdalok}" = no; then
AC_MSG_RESULT(no)
else
AC_MSG_RESULT(yes)
fi

if test "${gdalok}" = no; then
AC_MSG_CHECKING(GDAL: linking with --libs and --dep-libs)
LIBS="${LIBS} ${GDAL_DEP_LIBS}"
AC_LINK_IFELSE(
   [AC_LANG_SOURCE([`cat gdal_test.cpp`])],
   gdalok=yes,
   gdalok=no
)
if test "${gdalok}" = yes; then
    NEED_DEPS=yes
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
fi
fi

if test "${gdalok}" = no; then
  rm -f gdal_test.cpp
  AC_MSG_NOTICE([Install failure: compilation and/or linkage problems.])
  AC_MSG_ERROR([GDALAllRegister not found in libgdal, or GDAL not built with GEOS.])
fi

AC_MSG_CHECKING(GDAL: checking whether GEOS is available at runtime:)
AC_RUN_IFELSE(
   [AC_LANG_SOURCE([`cat gdal_test.cpp`])],
   gdal_has_geos=yes,
   gdal_has_geos=no,
   gdal_has_geos=cross_compiling
)
rm -f gdal_test.cpp
if test "${gdal_has_geos}" = no; then
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([OGR_G_IsSimple() returns false for a valid point: GEOS not available?])
elif test "${gdal_has_geos}" = cross_compiling; then
  AC_MSG_RESULT([assume yes for cross-compiling])
else
  AC_MSG_RESULT(yes)
fi

#
# Optional local copies of GDAL and PROJ data directories
#

PROJ_CONFIG="pkg-config proj"
if `$PROJ_CONFIG --exists` ; then
  AC_MSG_NOTICE([pkg-config proj exists, will use it])
  proj_config_ok=yes
  proj_data_dir=`${PROJ_CONFIG} --variable=datadir`
  if test -n "${proj_data_dir}" ; then
    AC_MSG_NOTICE([PROJ data directory: ${proj_data_dir}])
  fi
else
  proj_config_ok=no
fi

data_copy=no
AC_ARG_WITH([data-copy],
    AS_HELP_STRING([--with-data-copy=yes/no],[local copy of data directories in package, default no]),
               [data_copy=$withval])
if test "${data_copy}" = "yes" ; then
AC_MSG_NOTICE([Copy data for:])
# proj_data_dir may come from pkg-config (above), environment variable or configure arg
  if test -n "${proj_data_dir}" ; then
    proj_lib0="${proj_data_dir}"
  else
    proj_lib0="${PROJ_LIB}"
  fi
  # at PROJ 9.1 the environment variable is PROJ_DATA and PROJ_LIB is deprecated
  if test -z "${proj_lib0}" ; then
    proj_lib0="${PROJ_DATA}"
  fi
  AC_ARG_WITH([proj-data],
    AS_HELP_STRING([--with-proj-data=DIR],[location of PROJ data directory]),
    [proj_lib1=$withval])
  if test -n "${proj_lib0}" ; then
    proj_lib="${proj_lib0}"
  else
    proj_lib="${proj_lib1}"
  fi
  if test -n "${proj_lib}" ; then
    if test -d "${proj_lib}" ; then
      cp -r "${proj_lib}" "${R_PACKAGE_DIR}"
      AC_MSG_NOTICE([  PROJ: ${proj_lib}])
    else
      AC_MSG_ERROR([PROJ data files not found; set environment variable PROJ_LIB=DIR or --with-proj-data=DIR.])
    fi
  else
      AC_MSG_ERROR([PROJ data files not found; set environment variable PROJ_LIB=DIR or --with-proj-data=DIR.])
  fi

  if test -d "${GDAL_DATADIR}" ; then
    cp -r "${GDAL_DATADIR}" "${R_PACKAGE_DIR}"
    AC_MSG_NOTICE([  GDAL: ${GDAL_DATADIR}])
  else
    AC_MSG_ERROR([GDAL data files not found.])
  fi
fi

#
# PROJ include dir
#

AC_ARG_WITH([proj-include],
    AS_HELP_STRING([--with-proj-include=DIR],[location of proj header files]),
    [proj_include_path=$withval])
if test [ -n "$proj_include_path" ] ; then
   AC_SUBST([PROJ_CPPFLAGS],["-I${proj_include_path}"])
else
  if test "${proj_config_ok}" = yes; then
    PROJ_INCLUDE_PATH=`${PROJ_CONFIG} --cflags`
    AC_SUBST([PROJ_CPPFLAGS],["${PROJ_INCLUDE_PATH}"])
  fi
fi

#
# PROJ libs
#

AC_ARG_WITH([proj-lib],
    AS_HELP_STRING([--with-proj-lib=LIB_PATH],[the location of proj libraries]),
               [proj_lib_path=$withval])
if test [ -n "$proj_lib_path" ] ; then
    AC_SUBST([PROJ_LIBS], ["-L${proj_lib_path} ${INPKG_LIBS} -lproj"])
else
  if test "${proj_config_ok}" = yes; then
    if test `uname` = "Darwin"; then
      PROJ_LIB_PATH=`${PROJ_CONFIG} --libs --static`
    else
      PROJ_LIB_PATH=`${PROJ_CONFIG} --libs`
    fi
    PROJ_LIBS="${PROJ_LIB_PATH}"
    proj_version=`${PROJ_CONFIG} --modversion`
    AC_MSG_NOTICE([PROJ: ${proj_version}])
  else
    PROJ_LIBS="-lproj"
  fi
fi

AC_SUBST([PKG_CPPFLAGS], ["${INPKG_CPPFLAGS} ${PROJ_CPPFLAGS} ${GDAL_CPPFLAGS}"])
AC_SUBST([PKG_LIBS], ["${INPKG_LIBS} ${GDAL_LIBS} ${PROJ_LIBS}"])
if test "${NEED_DEPS}" = yes; then
   AC_SUBST([PKG_LIBS], ["${PKG_LIBS} ${GDAL_DEP_LIBS}"])
fi

CPPFLAGS="${INCPPFLAGS} ${PKG_CPPFLAGS}"
LIBS="${LIBS} ${PROJ_LIBS}"

#
# test whether PROJ is available to gdal:
#

gdal_has_proj=no

[cat > gdal_proj.cpp <<_EOCONF
#include <gdal.h>
#include <ogr_srs_api.h>
#include <ogr_spatialref.h>
int main(int argc, char *argv[]) {
  OGRSpatialReference *dest = new OGRSpatialReference;
  OGRSpatialReference *src = new OGRSpatialReference;
  src->importFromEPSG(4326);
  dest->importFromEPSG(3857);
  OGRCoordinateTransformation *ct = OGRCreateCoordinateTransformation(src, dest);
  return(ct == NULL); // signals PROJ is not available via GDAL
}
_EOCONF]

AC_MSG_CHECKING(GDAL: checking whether PROJ is available for linking:)
AC_LINK_IFELSE(
   [AC_LANG_SOURCE([`cat gdal_proj.cpp`])],
   gdal_has_proj=yes,
   gdal_has_proj=no
)
if test "${gdal_has_proj}" = no; then
  AC_MSG_RESULT(no)
  rm -f gdal_proj.cpp
  AC_MSG_NOTICE([Install failure: compilation and/or linkage problems.])
  AC_MSG_ERROR([cannot link projection code])
else
  AC_MSG_RESULT(yes)
fi

AC_MSG_CHECKING(GDAL: checking whether PROJ is available at runtime:)
AC_RUN_IFELSE(
   [AC_LANG_SOURCE([`cat gdal_proj.cpp`])],
   gdal_has_proj=yes,
   gdal_has_proj=no,
   gdal_has_proj=cross_compiling
)
rm -f gdal_proj.cpp
if test "${gdal_has_proj}" = no; then
  AC_MSG_RESULT(no)
  AC_MSG_ERROR([OGRCoordinateTransformation() returns NULL: PROJ not available?])
elif test "${gdal_has_proj}" = cross_compiling; then
  AC_MSG_RESULT([assume yes for cross-compiling])
else
  AC_MSG_RESULT(yes)
fi


AC_MSG_NOTICE([Package CPP flags: ${PKG_CPPFLAGS}])
AC_MSG_NOTICE([Package LIBS: ${PKG_LIBS}])

AC_CONFIG_FILES([src/Makevars])
AC_OUTPUT
