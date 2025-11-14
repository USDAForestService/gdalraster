# Miscellaneous helper functions for working with the GDAL API
# Chris Toney <chris.toney at usda.gov>

#' @noRd
#' @export
.get_crs_name <- function(wkt) {
    # name of form "<srs name> [(EPSG:####[, confidence ##])]"
    # include EPSG code if confidence > 50
    # include the confidence value if < 100

    crs_name <- srs_get_name(wkt)
    epsg <- srs_find_epsg(wkt, all_matches = TRUE)
    if (!is.null(epsg)) {
        if (nrow(epsg) >= 1 && epsg$confidence[1] > 50) {
            crs_name <- paste0(crs_name, " (", epsg$authority_name[1], ":",
                               epsg$authority_code[1])
            if (epsg$confidence[1] < 100) {
                crs_name <- paste0(crs_name, ", confidence ",
                                   epsg$confidence[1])
            }
            crs_name <- paste0(crs_name, ")")
        }
    }
    return(crs_name)
}


#' Compute a GDAL integer version number from major, minor, revision
#'
#' `gdal_compute_version()` computes a full integer version number
#' (GDAL_VERSION_NUM) from individual components (major, minor, revision).
#' Convenience function for checking a GDAL version requirement using
#' `gdal_version_num()`.
#'
#' @param maj Numeric value, major version component (coerced to integer by
#' truncation).
#' @param min Numeric value, min version component (coerced to integer by
#' truncation).
#' @param rev Numeric value, revision version component (coerced to integer by
#' truncation).
#' @returns Integer version number compatible with `gdal_version_num()`.
#'
#' @seealso
#' [gdal_version_num()]
#'
#' @examples
#' (gdal_version_num() >= gdal_compute_version(3, 7, 0))
#' @export
gdal_compute_version <- function(maj, min, rev) {
    if (!is.numeric(maj) || length(maj) != 1)
        stop("'maj' must be a single numeric value", call. = FALSE)
    else
        maj <- as.integer(maj)
    if (!is.numeric(min) || length(min) != 1)
        stop("'min' must be a single numeric value", call. = FALSE)
    else
        min <- as.integer(min)
    if (!is.numeric(rev) || length(rev) != 1)
        stop("'rev' must be a single numeric value", call. = FALSE)
    else
        rev <- as.integer(rev)

    return(as.integer(maj * 1000000 + min * 10000 + rev * 100))
}


#' Create/append to a potentially Seek-Optimized ZIP file (SOZip)
#'
#' `addFilesInZip()` will create new or open existing ZIP file, and
#' add one or more compressed files potentially using the seek optimization
#' extension. This function is basically a wrapper for `CPLAddFileInZip()`
#' in the GDAL Common Portability Library, but optionally creates a new ZIP
#' file first (with `CPLCreateZip()`). It provides a subset of functionality
#' in the GDAL `sozip` command-line utility
#' (\url{https://gdal.org/en/stable/programs/sozip.html}). Requires GDAL >= 3.7.
#'
#' @details
#' A Seek-Optimized ZIP file (SOZip) contains one or more compressed files
#' organized and annotated such that a SOZip-aware reader can perform very
#' fast random access within the .zip file
#' (see \url{https://github.com/sozip/sozip-spec}).
#' Large compressed files can be accessed directly from SOZip without prior
#' decompression. The .zip file is otherwise fully backward compatible.
#'
#' If `sozip_enabled="AUTO"` (the default), a file is seek-optimized only if
#' its size is above the values of `sozip_min_file_size` (default 1 MB) and
#' `sozip_chunk_size` (default `32768`).
#' In `"YES"` mode, all input files will be seek-optimized. In `"NO"` mode, no
#' input files will be seek-optimized. The default can be changed with the
#' `CPL_SOZIP_ENABLED` configuration option.
#'
#' @param zip_file Filename of the ZIP file. Will be created if it does not
#' exist or if `overwrite = TRUE`. Otherwise will append to an existing file.
#' @param add_files Character vector of one or more input filenames to add.
#' @param overwrite Logical scalar. Overwrite the target zip file if it already
#' exists.
#' @param full_paths Logical scalar. By default, the full path will be stored
#' (relative to the current directory). `FALSE` to store just the name of a
#' saved file (drop the path).
#' @param sozip_enabled String. Whether to generate a SOZip index for the file.
#' One of `"AUTO"` (the default), `"YES"` or `"NO"` (see Details).
#' @param sozip_chunk_size The chunk size for a seek-optimized file.
#' Defaults to 32768 bytes. The value is specified in bytes, or K and M
#' suffix can be used respectively to specify a value in kilo-bytes or
#' mega-bytes. Will be coerced to string.
#' @param sozip_min_file_size The minimum file size to decide if a file
#' should be seek-optimized, in `sozip_enabled="AUTO"` mode. Defaults to
#' 1 MB byte. The value is specified in bytes, or K, M or G suffix can be used
#' respectively to specify a value in kilo-bytes, mega-bytes or giga-bytes.
#' Will be coerced to string.
#' @param num_threads Number of threads used for SOZip generation. Defaults to
#' `"ALL_CPUS"` or specify an integer value (coerced to string).
#' @param content_type String Content-Type value for the file. This is stored
#' as a key-value pair in the extra field extension 'KV' (0x564b) dedicated to
#' storing key-value pair metadata.
#' @param quiet Logical scalar. `TRUE` for quiet mode, no progress messages
#' emitted. Defaults to `FALSE`.
#' @returns Logical indicating success (invisible \code{TRUE}).
#' An error is raised if the operation fails.
#'
#' @note
#' The `GDAL_NUM_THREADS` configuration option can be set to `ALL_CPUS` or an
#' integer value to specify the number of threads to use for SOZip-compressed
#' files (see [set_config_option()]).
#'
#' SOZip can be validated with:
#' ```
#' vsi_get_file_metadata(zip_file, domain="ZIP")
#' ```
#'
#' where `zip_file` uses the /vsizip/ prefix.
#'
#' @seealso
#' [vsi_get_file_metadata()]
#'
#' @examples
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' zip_file <- file.path(tempdir(), "storml_lcp.zip")
#'
#' # Requires GDAL >= 3.7
#' if (gdal_version_num() >= gdal_compute_version(3, 7, 0)) {
#'   addFilesInZip(zip_file, lcp_file, full_paths = FALSE,
#'                 sozip_enabled = "YES", num_threads = 1)
#'
#'   print("Files in zip archive:")
#'   print(unzip(zip_file, list = TRUE))
#'
#'   # Open with GDAL using Virtual File System handler '/vsizip/'
#'   # see: https://gdal.org/en/stable/user/virtual_file_systems.html#vsizip-zip-archives
#'   lcp_in_zip <- file.path("/vsizip", zip_file, "storm_lake.lcp")
#'   print("SOZip metadata:")
#'   print(vsi_get_file_metadata(lcp_in_zip, domain = "ZIP"))
#'
#'   ds <- new(GDALRaster, lcp_in_zip)
#'   ds$info()
#'   ds$close()
#'   \dontshow{vsi_unlink(zip_file)}
#' }
#' @export
addFilesInZip <- function(
        zip_file,
        add_files,
        overwrite = FALSE,
        full_paths = TRUE,
        sozip_enabled = NULL,
        sozip_chunk_size = NULL,
        sozip_min_file_size = NULL,
        num_threads = NULL,
        content_type = NULL,
        quiet = FALSE) {

    if (gdal_version_num() < gdal_compute_version(3, 7, 0))
        stop("addFilesInZip() requires GDAL >= 3.7", call. = FALSE)

    if (!is.character(zip_file) || length(zip_file) > 1)
        stop("'zip_file' must be a character string", call. = FALSE)
    else
        zip_file <- .check_gdal_filename(zip_file)

    if (!is.character(add_files))
        stop("'add_files' must be a character vector of filenames",
             call. = FALSE)

    if (!is.null(overwrite)) {
        if (!is.logical(overwrite) || length(overwrite) > 1)
            stop("'overwrite' must be a logical scalar", call. = FALSE)
    } else {
        overwrite <- FALSE
    }

    if (!is.null(full_paths)) {
        if (!is.logical(full_paths) || length(full_paths) > 1)
            stop("'full_paths' must be a logical scalar", call. = FALSE)
    } else {
        full_paths <- FALSE
    }

    if (!is.null(quiet)) {
        if (!is.logical(quiet) || length(quiet) > 1)
            stop("'quiet' must be a logical scalar", call. = FALSE)
    } else {
        quiet <- FALSE
    }

    opt <- NULL
    if (!is.null(sozip_enabled)) {
        if (!is.character(sozip_enabled) || length(sozip_enabled) > 1)
            stop("'sozip_enabled' must be a character string", call. = FALSE)
        sozip_enabled <- toupper(sozip_enabled)
        if (!(sozip_enabled %in% c("AUTO", "YES", "NO")))
            stop("'sozip_enabled' must be one of \"AUTO\", \"YES\" or \"NO\"",
                 call. = FALSE)
        else
            opt <- c(opt, paste0("SOZIP_ENABLED=", sozip_enabled))
    }
    if (!is.null(sozip_chunk_size)) {
        if (length(sozip_chunk_size) > 1)
            stop("'sozip_chunk_size' must be length-1", call. = FALSE)
        opt <- c(opt, paste0("SOZIP_CHUNK_SIZE=", sozip_chunk_size))
    }
    if (!is.null(sozip_min_file_size)) {
        if (length(sozip_min_file_size) > 1)
            stop("'sozip_min_file_size' must be length-1", call. = FALSE)
        opt <- c(opt, paste0("SOZIP_MIN_FILE_SIZE=", sozip_min_file_size))
    }
    if (!is.null(num_threads)) {
        if (length(num_threads) > 1)
            stop("'num_threads' must be length-1", call. = FALSE)
        opt <- c(opt, paste0("NUM_THREADS=", num_threads))
    }
    if (!is.null(content_type)) {
        if (!is.character(content_type) || length(content_type) > 1)
            stop("'content_type' must be a character string", call. = FALSE)
        opt <- c(opt, paste0("CONTENT_TYPE=", content_type))
    }

    if (overwrite)
        unlink(zip_file)

    ret <- FALSE
    for (f in add_files) {
        if (!(utils::file_test("-f", f)))
            stop("file not found: ", f, call. = FALSE)

        archive_fname <- f
        if (!full_paths) {
            archive_fname <- basename(f)
        } else if (substr(f, 1, 1) == "/") {
            archive_fname <- substring(f, 2)
        } else if (nchar(f) > 3 && substr(f, 2, 2) == ":" &&
                       (substr(f, 3, 3) == "/" || substr(f, 3, 3) == "\\")) {
            archive_fname <- substring(f, 4)
        }
        archive_fname <- .check_gdal_filename(archive_fname)

        if (!.addFileInZip(zip_file,
                           overwrite = FALSE,
                           archive_fname,
                           f,
                           opt,
                           quiet)) {
            ret <- FALSE
            break
        } else {
            ret <- TRUE
        }
    }

    if (!ret)
        stop("failed to add file, error from CPLAddFileInZip()",
             call. = FALSE)

    return(invisible(ret))
}


#' Return the list of creation options for a GDAL driver
#'
#' `getCreationOptions()` returns the list of creation options supported by a
#' GDAL format driver.
#' This function is a wrapper of `GDALGetDriverCreationOptionList()` in the
#' GDAL API, parsing its XML output into a named list.
#'
#' @details
#' The output is a nested list with names matching the creation option names.
#' The information for each creation option is a named list with the following
#' elements:
#' * `type`: a character string describing the data type, e.g., `"int"`,
#' `"float"`, `"string"`. The type `"string-select"` denotes a list of allowed
#' string values which are returned as a character vector in the `values`
#' element (see below).
#' * `description`: a character string describing the option, or `NA` if no
#' description is provided by the GDAL driver.
#' * `default`: the default value of the option as either a character string
#' or numeric value, or `NA` if no description is provided by the GDAL driver.
#' * `values`: a character vector of allowed string values for the creation
#' option if `type` is `"string-select"`, otherwise `NULL` if the option is
#' not a `"string-select"` type.
#' * `min`: (GDAL >= 3.11) the minimum value of the valid range for the
#' option, or `NA` if not provided by the GDAL driver or the option is not a
#' numeric type.
#' * `max`: (GDAL >= 3.11) the maximum value of the valid range for the
#' option, or `NA` if not provided by the GDAL driver or the option is not a
#' numeric type.
#'
#' @param format Format short name (e.g., `"GTiff"`).
#' @param filter Optional character vector of creation option names.
#' @returns A named list with names matching the creation option names, and
#' each element a named list with elements `type`, `description`, `default`
#' and `values` (see Details).
#'
#' @seealso
#' [create()], [createCopy()], [translate()], [validateCreationOptions()],
#' [warp()]
#'
#' @examples
#' opt <- getCreationOptions("GTiff", "COMPRESS")
#' names(opt)
#'
#' (opt$COMPRESS$type == "string-select")  # TRUE
#' opt$COMPRESS$values
#'
#' all_opt <- getCreationOptions("GTiff")
#' names(all_opt)
#'
#' # $description and $default will be NA if no value is provided by the driver
#' # $values will be NULL if the option is not a 'string-select' type
#'
#' all_opt$PREDICTOR
#'
#' all_opt$BIGTIFF
#' @export
getCreationOptions <- function(format, filter = NULL) {

    if (!is.character(format) || length(format) > 1)
        stop("'format' must be a character string", call. = FALSE)

    if (is.null(filter) || filter == "") {
        filter <- "_all_"
    } else {
        filter <- toupper(filter)
    }

    if (.getCreationOptions(format) == "") {
        message("no creation options found for ", format)
        return(NULL)
    }

    xml <- xml2::read_xml(.getCreationOptions(format))
    el <- xml2::xml_children(xml)
    out <- list()
    if (length(el) == 0) {
        return(NULL)
    } else {
        for (i in seq_along(el)) {
            a <- xml_attrs(el[[i]])
            if (filter[1] == "_all_" || toupper(a["name"]) %in% filter) {
                type_name <- unname(a["type"])
                str_values <- xml_children(el[[i]]) |> as_list() |> unlist()
                default_val <- unname(a["default"])
                if (toupper(type_name) %in% c("INT", "INTEGER", "UNSIGNED INT",
                                              "FLOAT")) {
                    default_val <- as.numeric(default_val)
                }

                # The XML returned by GDAL < 3.11 does not include the
                # min/max attributes even though they are populated in the
                # driver code that builds the XML string.
                # (https://github.com/OSGeo/gdal/issues/11967)
                if (gdal_version_num() < 3110000) {
                    out[[unname(a["name"])]] <- list(
                        type = type_name,
                        description = unname(a["description"]),
                        default = default_val,
                        values = str_values)
                } else {
                    out[[unname(a["name"])]] <- list(
                        type = type_name,
                        description = unname(a["description"]),
                        default = default_val,
                        values = str_values,
                        min = as.numeric(unname(a["min"])),
                        max = as.numeric(unname(a["max"])))
                }
            }
        }
    }

    return(out)
}


#' Return the list of options associated with a virtual file system handler
#'
#' `vsi_get_fs_options()` returns the list of options associated with a virtual
#' file system handler. Those options may be set as configuration options with
#' `set_config_option()`.
#' Wrapper for `VSIGetFileSystemOptions()` in the GDAL API.
#'
#' @param filename Filename, or prefix of a virtual file system handler.
#' @param as_list Logical scalar. If `TRUE` (the default), the XML string
#' returned by GDAL will be coerced to list. `FALSE` to return the configuration
#' options as a serialized XML string.
#' @returns An XML string, or empty string (`""`) if no options are declared.
#' If `as_list = TRUE` (the default), the XML string will be coerced to list
#' with `xml2::as_list()`.
#'
#' @seealso
#' [set_config_option()], [vsi_get_fs_prefixes()]
#'
#' \url{https://gdal.org/en/stable/user/virtual_file_systems.html}
#'
#' @examples
#' vsi_get_fs_options("/vsimem/")
#'
#' vsi_get_fs_options("/vsizip/")
#'
#' vsi_get_fs_options("/vsizip/", as_list = FALSE)
#' @export
vsi_get_fs_options <- function(filename, as_list = TRUE) {

    if (!is.character(filename) || length(filename) > 1)
        stop("'filename' must be a length-1 character vector.", call.=FALSE)

    opts <- .vsi_get_fs_options(filename)

    if (opts == "")
        return(opts)
    else if (as_list)
        return(xml2::read_xml(opts) |> xml2::as_list())
    else
        return(opts)
}


#' Apply geotransform (raster column/row to geospatial x/y)
#'
#' `apply_geotransform()` applies geotransform coefficients to raster
#' coordinates in pixel/line space (column/row), converting into
#' georeferenced (x/y) coordinates. Wrapper of `GDALApplyGeoTransform()` in
#' the GDAL API, operating on matrix input.
#'
#' @param col_row Numeric matrix of raster column, row (pixel/line) coordinates
#' (or two-column data frame that will be coerced to numeric matrix, or a
#' vector of column, row for one coordinate).
#' @param gt Either a numeric vector of length six containing the affine
#' geotransform for the raster, or an object of class `GDALRaster` from
#' which the geotransform will be obtained.
#' @returns Numeric matrix of geospatial x/y coordinates.
#'
#' @note
#' Bounds checking on the input coordinates is done if `gt` is obtained from an
#' object of class `GDALRaster`. See Note for [get_pixel_line()].
#'
#' @seealso [`GDALRaster$getGeoTransform()`][GDALRaster], [get_pixel_line()]
#'
#' @examples
#' raster_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds <- new(GDALRaster, raster_file)
#'
#' # compute some raster coordinates in column/row space
#' set.seed(42)
#' col_coords <- runif(10, min = 0, max = ds$getRasterXSize() - 0.00001)
#' row_coords <- runif(10, min = 0, max = ds$getRasterYSize() - 0.00001)
#' col_row <- cbind(col_coords, row_coords)
#'
#' # convert to geospatial x/y coordinates
#' gt <- ds$getGeoTransform()
#' apply_geotransform(col_row, gt)
#'
#' # or, using the class method
#' ds$apply_geotransform(col_row)
#'
#' # bounds checking
#' col_row <- rbind(col_row, c(ds$getRasterXSize(), ds$getRasterYSize()))
#' ds$apply_geotransform(col_row)
#'
#' ds$close()
#' @export
apply_geotransform <- function(col_row, gt) {
    if (!(is.vector(col_row) || is.matrix(col_row) || is.data.frame(col_row)))
        stop("'col_row' must be a data frame or numeric matrix", call. = FALSE)

    if ((is.matrix(col_row) || is.data.frame(col_row)) && ncol(col_row) != 2)
        stop("'col_row' must have 2 columns", call. = FALSE)
    else if (is.vector(col_row) && length(col_row) != 2)
        stop("'col_row' as vector must have length 2", call. = FALSE)

    # allow for c(NA, NA) which is logical type, NA input should return NA out
    if ((is.vector(xy) || is.matrix(xy)) && !is.numeric(xy) && !is.logical(xy))
        stop("'col_row' must be numeric", call. = FALSE)

    if (is(gt, "Rcpp_GDALRaster")) {
        return(.apply_geotransform_ds(col_row, gt))
    } else if (is.numeric(gt) && length(gt) == 6) {
        return(.apply_geotransform_gt(col_row, gt))
    } else {
        stop("'gt' must be a numeric vector of length 6, or GDALRaster object",
             call. = FALSE)
    }
}


#' Raster pixel/line from geospatial x,y coordinates
#'
#' `get_pixel_line()` converts geospatial coordinates to pixel/line (raster
#' column, row numbers).
#' The upper left corner pixel is the raster origin (0,0) with column, row
#' increasing left to right, top to bottom.
#'
#' @param xy Numeric matrix of geospatial x, y coordinates in the same spatial
#' reference system as \code{gt} (or two-column data frame that will be coerced
#' to numeric matrix, or a vector x, y for one coordinate).
#' @param gt Either a numeric vector of length six containing the affine
#' geotransform for the raster, or an object of class `GDALRaster` from
#' which the geotransform will be obtained (see Note).
#' @returns Integer matrix of raster pixel/line.
#'
#' @note
#' This function applies the inverse geotransform to the input points. If `gt`
#' is given as the numeric vector, no bounds checking is done (i.e., min
#' pixel/line could be less than zero and max pixel/line could be greater than
#' the raster x/y size). If `gt` is obtained from an object of class
#' `GDALRaster`, then `NA` is returned for points that fall outside the
#' raster extent and a warning emitted giving the number points that were
#' outside. This latter case is equivalent to calling the
#' \code{$get_pixel_line()} class method on the `GDALRaster` object (see
#' Examples).
#'
#' @seealso [`GDALRaster$getGeoTransform()`][GDALRaster], [inv_geotransform()]
#'
#' @examples
#' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
#' # id, x, y in NAD83 / UTM zone 12N
#' pts <- read.csv(pt_file)
#' print(pts)
#'
#' raster_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds <- new(GDALRaster, raster_file)
#' gt <- ds$getGeoTransform()
#' get_pixel_line(pts[, -1], gt)
#'
#' # or, using the class method
#' ds$get_pixel_line(pts[, -1])
#'
#' # add a point outside the raster extent
#' pts[11, ] <- c(11, 323318, 5105104)
#' get_pixel_line(pts[, -1], gt)
#'
#' # with bounds checking on the raster extent
#' ds$get_pixel_line(pts[, -1])
#'
#' ds$close()
#' @export
get_pixel_line <- function(xy, gt) {
    if (!(is.vector(xy) || is.matrix(xy) || is.data.frame(xy)))
        stop("'xy' must be a data frame or numeric matrix", call. = FALSE)

    if ((is.matrix(xy) || is.data.frame(xy)) && ncol(xy) != 2)
        stop("'xy' must have 2 columns", call. = FALSE)
    else if (is.vector(xy) && length(xy) != 2)
        stop("'xy' as vector must have length 2", call. = FALSE)

    # allow for c(NA, NA) which is logical type, NA input should return NA out
    if ((is.vector(xy) || is.matrix(xy)) && !is.numeric(xy) && !is.logical(xy))
        stop("'xy' must be numeric", call. = FALSE)

    if (is(gt, "Rcpp_GDALRaster")) {
        return(.get_pixel_line_ds(xy, gt))
    } else if (is.numeric(gt) && length(gt) == 6) {
        return(.get_pixel_line_gt(xy, gt))
    } else {
        stop("'gt' must be a numeric vector of length 6, or GDALRaster object",
             call. = FALSE)
    }
}


#' Report open datasets
#'
#' `dump_open_datasets()` dumps a list of all open datasets (shared or not) to
#' the console. This function is primarily intended to assist in debugging
#' "dataset leaks" and reference counting issues. The information reported
#' includes the dataset name, referenced count, shared status, driver name,
#' size, and band count. This a wrapper for `GDALDumpOpenDatasets()` with
#' output to the console.
#'
#' @returns Number of open datasets.
#'
#' @examples
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#' ds <- new(GDALRaster, elev_file)
#' dump_open_datasets()
#' ds2 <- new(GDALRaster, elev_file)
#' dump_open_datasets()
#' # open without using shared mode
#' ds3 <- new(GDALRaster, elev_file, read_only = TRUE,
#'            open_options = NULL, shared = FALSE)
#' dump_open_datasets()
#' ds$close()
#' dump_open_datasets()
#' ds2$close()
#' dump_open_datasets()
#' ds3$close()
#' dump_open_datasets()
#' @export
dump_open_datasets <- function() {
    f <- tempfile(fileext = ".txt")
    nopen <- .dump_open_datasets(f)
    if (nopen < 0)
        stop("failed to obtain the list of open datasets", call. = FALSE)

    out <- readLines(f)
    unlink(f)
    writeLines(out)
    return(nopen)
}


#' Obtain information about a GDAL raster or vector dataset
#'
#' `inspectDataset()` returns information about the format and content
#' of a dataset. The function first calls `identifyDriver()`, and then opens
#' the dataset as raster and/or vector to obtain information about its content.
#' The return value is a list with named elements.
#'
#' @param filename Character string containing the name of the file to access.
#' This may not refer to a physical file, but instead contain information for
#' the driver on how to access a dataset (e.g., connection string, URL, etc.)
#' @param ... Additional arguments passed to `identifyDriver()`.
#'
#' @returns
#' A list with the following named elements:
#' * `format`: character string, the format short name
#' * `supports_raster`: logical, `TRUE` if the format supports raster data
#' * `contains_raster`: logical, `TRUE` if this is a raster dataset or the
#' source contains raster subdatasets
#' * `supports_subdatasets`: logical, `TRUE` if the format supports raster
#' subdatasets
#' * `contains_subdatasets`: logical, `TRUE` if the source contains subdatasets
#' * `subdataset_names`: character vector containing the subdataset names, or
#' empty vector if subdatasets are not supported or not present
#' * `supports_vector`: logical, `TRUE` if the format supports vector data
#' * `contains_vector`: logical, `TRUE` if the source contains one or more
#' vector layers
#' * `layer_names`: character vector containing the vector layer names, or
#' empty vector if the format does not support vector or the source does not
#' contain any vector layers
#'
#'@note
#' Subdataset names are the character strings that can be used to
#' instantiate `GDALRaster` objects.
#' See https://gdal.org/en/stable/en/latest/user/raster_data_model.html#subdatasets-domain.
#'
#' PostgreSQL / PostGISRaster are handled as a special case. If additional
#' arguments `raster` or `vector` are not given for `identifyDriver()`, then
#' `raster = FALSE` is assumed.
#'
#' @seealso
#' [gdal_formats()], [identifyDriver()]
#'
#' @examples
#' f <- system.file("extdata/ynp_features.zip", package = "gdalraster")
#' ynp_dsn <- file.path("/vsizip", f, "ynp_features.gpkg")
#'
#' inspectDataset(ynp_dsn)
#' @export
inspectDataset <- function(filename, ...) {
    if (!is.character(filename))
        stop("'filename' must be a character string", call. = FALSE)

    filename_in <- .check_gdal_filename(filename)
    fmt <- identifyDriver(filename = filename_in, ...)
    if (is.null(fmt)) {
        warning("failed to identify a format driver", call. = FALSE)
        return(NULL)
    }

    if (!hasArg("raster") && !hasArg("vector")) {
        # check for possibly two different drivers
        fmt_rast <- identifyDriver(filename = filename_in, vector = FALSE, ...)
        fmt_vect <- identifyDriver(filename = filename_in, raster = FALSE, ...)
        if (!is.null(fmt_rast) && !is.null(fmt_vect)) {
            if (fmt_rast != fmt_vect) {
                if (fmt_vect == "PostgreSQL") {
                    # for PostGISRaster / PostgreSQL, assume vector is intended
                    fmt <- fmt_vect
                } else {
                    message("identified separate raster and vector drivers: ",
                            fmt_rast, ", ", fmt_vect)
                    stop("need additional arguments for `identifyDriver()`",
                         call. = FALSE)
                }
            }
        }
    }

    out <- list()
    out$format <- fmt
    fmt_info <- gdal_formats(fmt)
    if (nrow(fmt_info) == 0) {
        stop("failed to obtain format information", call. = FALSE)
    }

    out$supports_raster <- fmt_info$raster
    out$contains_raster <- FALSE
    if (out$supports_raster) {
        push_error_handler("quiet")
        ds <- try(new(GDALRaster, filename_in), silent = TRUE)
        pop_error_handler()
        if (is(ds, "Rcpp_GDALRaster"))
            out$contains_raster <- TRUE
    }

    out$supports_subdatasets <- fmt_info$subdatasets
    out$contains_subdatasets <- FALSE
    out$subdataset_names <- character(0)
    if (out$contains_raster) {
        md <- ds$getMetadata(band = 0, domain = "SUBDATASETS")
        if (length(md) > 1) {
            out$contains_subdatasets <- TRUE
            Encoding(md) <- "UTF-8"
            for (i in seq_along(md)) {
                mdi <- strsplit(md[i], "=", fixed = TRUE)
                if (grepl("_NAME", mdi[[1]][1], ignore.case = TRUE)) {
                    out$subdataset_names <- c(out$subdataset_names, mdi[[1]][2])
                }
            }
        }
    }

    if (out$supports_raster && is(ds, "Rcpp_GDALRaster")) {
        ds$close()
    }

    out$supports_vector <- fmt_info$vector
    out$contains_vector <- FALSE
    out$layer_names <- character(0)
    if (out$supports_vector) {
        if (ogr_ds_layer_count(filename_in) > 0) {
            out$contains_vector <- TRUE
            out$layer_names <- ogr_ds_layer_names(filename_in)
        }
    }

    return(out)
}


#' Generate an index of chunk offsets and sizes for iterating a raster
#'
#' `make_chunk_index()` returns a matrix of `xchunkoff`, `ychunkoff`, `xoff`,
#' `yoff`, `xsize`, `ysize`, `xmin`, `xmax`, `ymin` and `ymax`, i.e., indexing
#' of potentially multi-block chunks defined on block boundaries for iterating
#' I/O operations over a raster. The last four columns are geospatial
#' coordinates of the bounding box. Note that class `GDALRaster` provides a
#' method of the same name that is more convenient to use with a dataset object.
#'
#' @details
#' The stand-alone function here supports the general case of chunking/tiling a
#' raster layout without using a dataset object. If the `max_pixels` argument
#' is set to zero, the chunks are raster blocks (the internal tiles in the case
#' of a tiled format). Otherwise, chunks are defined as the maximum number of
#' consecutive whole blocks containing `<= max_pixels`, that may span one or
#' multiple whole rows of blocks.
#'
#' @param raster_xsize Integer value giving the number of raster columns.
#' @param raster_ysize Integer value giving the number of raster rows.
#' @param block_xsize Integer value giving the horizontal size of a raster block
#' in number of pixels.
#' @param block_ysize Integer value giving the vertical size of a raster block
#' in number of pixels.
#' @param gt A numeric vector of length six containing the affine geotransform
#' for the raster (defaults to `c(0, 1, 0, 0, 0, 1)`). Required only if
#' geospatial bounding boxes of the chunks are needed in the output.
#' @param max_pixels Numeric value (a whole number), optionally carrying the
#' `bit64::integer64` class attribute. Specifies the maximum number of pixels
#' per chunk. Can be set to zero to define chunks as the blocks.
#' @return A numeric matrix with number of rows equal to the number of chunks,
#' and named columns: `xchunkoff`, `ychunkoff`, `xoff`, `yoff`, `xsize`,
#' `ysize`, `xmin`, `xmax`, `ymin`, `ymax`. Offsets are 0-based.
#'
#' @seealso
#' Methods \code{make_chunk_index()}, \code{readChunk()} and
#' \code{writeChunk()} in class [GDALRaster].
#'
#' Usage example in the web article
#' [GDAL Block Cache](https://usdaforestservice.github.io/gdalraster/articles/gdal-block-cache.html).
#'
#' @examples
#' ## chunk as one block
#' blocks <- make_chunk_index(raster_xsize = 156335, raster_ysize = 101538,
#'                            block_xsize = 256, block_ysize = 256,
#'                            gt = c(-2362395, 30, 0, 3267405, 0, -30),
#'                            max_pixels = 0)
#'
#' nrow(blocks)
#'
#' head(blocks)
#'
#' tail(blocks)
#'
#' ## chunk as 16 consectutive blocks
#' chunks <- make_chunk_index(raster_xsize = 156335, raster_ysize = 101538,
#'                            block_xsize = 256, block_ysize = 256,
#'                            gt = c(-2362395, 30, 0, 3267405, 0, -30),
#'                            max_pixels = 256 * 256 * 16)
#'
#' nrow(chunks)
#'
#' head(chunks)
#'
#' tail(chunks)
#' @export
make_chunk_index <- function(raster_xsize, raster_ysize,
                             block_xsize, block_ysize,
                             gt = c(0, 1, 0, 0, 0, 1),
                             max_pixels = 0) {

    return(.make_chunk_index(raster_xsize, raster_ysize, block_xsize,
                             block_ysize, gt, max_pixels))
}
