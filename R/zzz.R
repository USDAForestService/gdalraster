.onLoad <- function(libname, pkgname) {
  if (dir.exists(system.file("proj", package="gdalraster"))) {
    Sys.setenv("PROJ_LIB" = system.file("proj", package="gdalraster"))
  }
  if (dir.exists(system.file("gdal", package="gdalraster"))) {
    Sys.setenv("GDAL_DATA" = system.file("gdal", package="gdalraster"))
  }
 }

.onAttach <- function(libname, pkgname) {
	msg = gdal_version()[1]
	packageStartupMessage(msg)
}

