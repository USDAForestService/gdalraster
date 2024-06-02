.gdalraster_env <- new.env()

.gdalraster_finalizer <- function(env) {
    # clean-up for /vsicurl/ and related file systems
    push_error_handler("quiet")
    .cpl_http_cleanup()
    pop_error_handler()
}

.onLoad <- function(libname, pkgname) {
    # set environment variables on Windows
    if (dir.exists(system.file("proj", package="gdalraster"))) {
        gdalraster_proj <- system.file("proj", package="gdalraster")
        proj_path_set <- FALSE
        path_api <- proj_search_paths(gdalraster_proj)
        if (normalizePath(path_api) == normalizePath(gdalraster_proj))
            proj_path_set <- TRUE
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

    # register a finalizer for cleanup at the end of an R session
    reg.finalizer(.gdalraster_env, .gdalraster_finalizer, onexit = TRUE)
}

.onAttach <- function(libname, pkgname) {
    msg <- gdal_version()[1]
    if (!is.na(geos_version()$name))
        geos_ver <- geos_version()$name
    else
        geos_ver <- "(version NA)"
    msg <- paste0(msg, ", GEOS ", geos_ver)
    if (!is.na(proj_version()$name))
        proj_ver <- proj_version()$name
    else
        proj_ver <- "(version NA)"
    msg <- paste0(msg, ", PROJ ", proj_ver)
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
    .gdalraster_finalizer(.gdalraster_env)
}
