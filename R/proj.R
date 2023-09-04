# R wrappers for PROJ utility functions (via GDAL headers)
# Chris Toney <chris.toney at usda.gov>

#' Get PROJ version
#'
#' @description
#' `proj_version()` returns version information for the PROJ library in use by
#' GDAL. Requires GDAL >= 3.0.1.
#' 
#' @returns A list of length four containing:
#'   * `name` - a string formatted as "major.minor.patch"
#'   * `major` - major version as integer
#'   * `minor` - minor version as integer
#'   * `patch` - patch version as integer
#' 
#' List elements will be `NA` if GDAL < 3.0.1.
#' 
#' @seealso
#' [gdal_version()], [proj_search_paths()], [proj_networking()]
#' 
#' @examples
#' proj_version()
#' @export
proj_version <- function() {
	ver <- .getPROJVersion()
	pv <- list()
	if (anyNA(ver))
		pv$name <- NA_character_
	else
		pv$name <- paste(ver, collapse=".")
	pv$major <- ver[1]
	pv$minor <- ver[2]
	pv$patch <- ver[3]
	return(pv)
}

#' Get or set search path(s) for PROJ resource files
#'
#' @description
#' `proj_search_paths()` returns the search path(s) for PROJ resource files,
#' optionally setting them first. Requires GDAL 3.0.3 or later.
#' 
#' @param paths Optional character vector containing one or more directory
#' paths to set.
#' @returns A character vector containing the currently used search path(s) for
#' PROJ resource files. An empty string (`""`) is returned if no search paths
#' are returned by the function `OSRGetPROJSearchPaths()` in the GDAL Spatial
#' Reference System C API. `NA` is returned if GDAL < 3.0.3.
#'
#' @seealso
#' [gdal_version()], [proj_version()], [proj_networking()]
#' 
#' @examples
#' proj_search_paths()
#' @export
proj_search_paths <- function(paths=NULL) {
	if (!is.null(paths))
		.setPROJSearchPaths(paths)
	return(.getPROJSearchPaths())
}

#' Check, enable or disable PROJ networking capabilities
#'
#' @description
#' `proj_networking()` returns the status of PROJ networking capabilities,
#' optionally enabling or disabling first. Requires GDAL 3.4 or later and
#' PROJ 7 or later.
#' 
#' @param enabled Optional logical scalar. Set to `TRUE` to enable networking
#' capabilities or `FALSE` to disable.
#' @returns Logical `TRUE` if PROJ networking capabilities are enabled (as
#' indicated by the return value of `OSRGetPROJEnableNetwork()` in the GDAL
#' Spatial Reference System C API). Logical `NA` is returned if GDAL < 3.4.
#'
#' @seealso
#' [gdal_version()], [proj_version()], [proj_search_paths()]
#'
#' \href{https://github.com/OSGeo/PROJ-data}{PROJ-data on GitHub},
#' \href{https://cdn.proj.org/}{PROJ Content Delivery Network}
#' 
#' @examples
#' proj_networking()
#' @export
proj_networking <- function(enabled=NULL) {
	if (!is.null(enabled)) {
		if (!is.logical(enabled) || length(enabled) > 1)
			stop("Argument must be a logical scalar.", call.=FALSE)
		.setPROJEnableNetwork(enabled)
	}
	return(.getPROJEnableNetwork())
}

