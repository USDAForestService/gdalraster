.gdalraster_env <- new.env(parent=.GlobalEnv)

.onLoad <- function(libname, pkgname) {
	# set environment variables on Windows
	if (dir.exists(system.file("proj", package="gdalraster"))) {
		assign(".orig_proj_lib", Sys.getenv("PROJ_LIB"),
				envir=.gdalraster_env)
		Sys.setenv("PROJ_LIB" = system.file("proj", package="gdalraster"))
		# PROJ_DATA for PROJ >= 9.1
		assign(".orig_proj_data", Sys.getenv("PROJ_DATA"),
				envir=.gdalraster_env)
		Sys.setenv("PROJ_DATA" = system.file("proj", package="gdalraster"))
	}
	if (dir.exists(system.file("gdal", package="gdalraster"))) {
		assign(".orig_gdal_data", Sys.getenv("GDAL_DATA"),
				envir=.gdalraster_env)
		Sys.setenv("GDAL_DATA" = system.file("gdal", package="gdalraster"))
	}
}

.onAttach <- function(libname, pkgname) {
	msg <- gdal_version()[1]
	packageStartupMessage(msg)
}

.onUnload <- function(libname, pkgname) {
	# reset env variables to original values on Windows
	if (dir.exists(system.file("proj", package="gdalraster"))) {
		Sys.setenv("PROJ_LIB"=get(".orig_proj_lib", envir=.gdalraster_env))
		Sys.setenv("PROJ_DATA"=get(".orig_proj_data", envir=.gdalraster_env))
	}
	if (dir.exists(system.file("gdal", package="gdalraster"))) {
		Sys.setenv("GDAL_DATA"=get(".orig_gdal_data", envir=.gdalraster_env))
	}
}

