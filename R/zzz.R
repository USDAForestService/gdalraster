.gdalraster_env <- new.env(parent=.GlobalEnv)

.onLoad <- function(libname, pkgname) {
	# set environment variables on Windows
	if (dir.exists(system.file("proj", package="gdalraster"))) {
		gdalraster_proj <- system.file("proj", package="gdalraster")
		proj_path_set <- FALSE
		if (as.integer(gdal_version()[2] >= 3000100)) {
			# set with API
			path_api <- proj_search_paths(gdalraster_proj)
			if (normalizePath(path_api) == normalizePath(gdalraster_proj))
				proj_path_set <- TRUE
		}
		if (!proj_path_set) {
			assign(".orig_proj_lib", Sys.getenv("PROJ_LIB"),
					envir=.gdalraster_env)
			Sys.setenv("PROJ_LIB" = gdalraster_proj)
		}
	}
	if (dir.exists(system.file("gdal", package="gdalraster"))) {
		assign(".orig_gdal_data", Sys.getenv("GDAL_DATA"),
				envir=.gdalraster_env)
		Sys.setenv("GDAL_DATA" = system.file("gdal", package="gdalraster"))
	}
}

.onAttach <- function(libname, pkgname) {
	msg <- gdal_version()[1]
	if (!is.na(proj_version()$name))
		msg <- paste0(msg, ", PROJ ", proj_version()$name)
	packageStartupMessage(msg)
}

.onUnload <- function(libname, pkgname) {
	# reset env variables to original values on Windows
	if (dir.exists(system.file("proj", package="gdalraster"))) {
		if (exists(".orig_proj_lib", where=.gdalraster_env))
			Sys.setenv("PROJ_LIB"=get(".orig_proj_lib", envir=.gdalraster_env))
	}
	if (dir.exists(system.file("gdal", package="gdalraster"))) {
		Sys.setenv("GDAL_DATA"=get(".orig_gdal_data", envir=.gdalraster_env))
	}
}

