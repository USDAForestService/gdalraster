#' @name GDALRaster-class
#'
#' @aliases
#' Rcpp_GDALRaster Rcpp_GDALRaster-class GDALRaster
#'
#' @title Class encapsulating a subset of the GDAL Raster C API
#'
#' @description
#' `GDALRaster` provides an interface for accessing a raster dataset via GDAL 
#' and calling methods on the underlying GDALDataset, GDALDriver and 
#' GDALRasterBand objects. See \url{https://gdal.org/api/index.html} for
#' details of the GDAL API.
#'
#' @param filename Character string containing the file name of a raster 
#' dataset to open, as full path or relative to the current working directory. 
#' In some cases, `filename` may not refer to a physical file, but instead 
#' contain format-specific information on how to access a dataset (GDAL raster 
#' format descriptions: \url{https://gdal.org/drivers/raster/index.html}).
#' @param read_only Logical. `TRUE` to open the dataset read-only, or `FALSE` 
#' to open with write access.
#' @returns An object of class `GDALRaster` which contains a pointer to the 
#' opened dataset, and methods that operate on the dataset as described in 
#' Details.
#'
#' @section Usage:
#' \preformatted{
#' ds <- new(GDALRaster, filename, read_only)
#'
#' ## Methods (see Details)
#' ds$getFilename()
#' ds$open(read_only)
#' ds$isOpen()
#'
#' ds$info()
#'
#' ds$getDriverShortName()
#' ds$getDriverLongName()
#'
#' ds$getRasterXSize()
#' ds$getRasterYSize()
#' ds$getGeoTransform()
#' ds$setGeoTransform(transform)
#' ds$getProjectionRef()
#' ds$setProjection(projection)
#' ds$bbox()
#' ds$res()
#' ds$dim()
#'
#' ds$getRasterCount()
#' ds$getBlockSize(band)
#' ds$getOverviewCount(band)
#' ds$buildOverviews(resampling, levels, bands)
#' ds$getDataTypeName(band)
#' ds$getStatistics(band, approx_ok, force)
#' ds$getNoDataValue(band)
#' ds$setNoDataValue(band, nodata_value)
#' ds$deleteNoDataValue(band)
#' ds$getUnitType(band)
#' ds$setUnitType(band, unit_type)
#' ds$getScale(band)
#' ds$setScale(band, scale)
#' ds$getOffset(band)
#' ds$setOffset(band, offset)
#'
#' ds$getMetadata(band, domain)
#' ds$getMetadataItem(band, mdi_name, domain)
#' ds$setMetadataItem(band, mdi_name, mdi_value, domain)
#'
#' ds$read(band, xoff, yoff, xsize, ysize, out_xsize, out_ysize)
#' ds$write(band, xoff, yoff, xsize, ysize, rasterData)
#' ds$fillRaster(value, ivalue)
#'
#' ds$getChecksum(band, xoff, yoff, xsize, ysize)
#'
#' ds$close()
#' }
#'
#' @section Details:
#'
#' \code{new(GDALRaster, filename, read_only)}
#' Constructor. Returns an object of class `GDALRaster`.
#'
#' \code{$getFilename()}
#' Returns a character string containing the filename associated with this 
#' `GDALRaster` object.
#'
#' \code{$open(read_only)}
#' (Re-)opens the raster dataset on the existing filename. Use this method to
#' open a dataset that has been closed using \code{$close()}. May be used to
#' re-open a dataset with a different read/write access (`read_only` set to 
#' `TRUE` or `FALSE`). The method will first close an open dataset, so it is 
#' not required to call \code{$close()} explicitly in this case. 
#' No return value, called for side effects.
#'
#' \code{$isOpen()}
#' Returns logical indicating whether the associated raster dataset is open.
#'
#' \code{$info()}
#' Prints various information about the raster dataset to the console (no 
#' return value, called for that side effect only).
#' Equivalent to the output of the \command{gdalinfo} command-line utility
#' (\command{gdalinfo -norat -noct filename}). Intended here as an 
#' informational convenience function.
#'
#' \code{$getDriverShortName()}
#' Returns the short name of the raster format driver
#' (e.g., "HFA").
#'
#' \code{$getDriverLongName()}
#' Returns the long name of the raster format driver
#' (e.g., "Erdas Imagine Images (.img)").
#'
#' \code{$getRasterXSize()}
#' Returns the number of pixels along the x dimension.
#'
#' \code{$getRasterYSize()}
#' Returns the number of pixels along the y dimension.
#'
#' \code{$getGeoTransform()}
#' Returns the affine transformation coefficients for transforming between
#' pixel/line raster space (column/row) and projection coordinate space 
#' (geospatial x/y). The return value is a numeric vector of length six.
#' See \url{https://gdal.org/tutorials/geotransforms_tut.html}
#' for details of the affine transformation. \emph{With 1-based indexing 
#' in R}, the geotransform vector contains (in map units of the raster spatial
#' reference system):
#' \tabular{rl}{
#' GT\[1\] \tab x-coordinate of upper-left corner of the upper-left pixel\cr
#' GT\[2\] \tab x-component of pixel width\cr
#' GT\[3\] \tab row rotation (zero for north-up raster)\cr
#' GT\[4\] \tab y-coordinate of upper-left corner of the upper-left pixel\cr
#' GT\[5\] \tab column rotation (zero for north-up raster)\cr
#' GT\[6\] \tab y-component of pixel height (negative for north-up raster)
#'}
#'
#' \code{$setGeoTransform(transform)}
#' Sets the affine transformation coefficients on this dataset.
#' \code{transform} is a numeric vector of length six.
#' Returns logical TRUE on success or FALSE if the geotransform could not 
#' be set.
#'
#' \code{$getProjectionRef()}
#' Returns the coordinate reference system of the raster as an OpenGIS WKT
#' format string.
#' An empty string is returned when a projection definition is not available.
#'
#' \code{$setProjection(projection)}
#' Sets the projection reference for this dataset.
#' \code{projection} is a string in OGC WKT format.
#' Returns logical TRUE on success or FALSE if the projection could not be set.
#'
#' \code{$bbox()}
#' Returns a numeric vector of length four containing the bounding box 
#' (xmin, ymin, xmax, ymax) assuming this is a north-up raster.
#'
#' \code{$res()}
#' Returns a numeric vector of length two containing the resolution
#' (pixel width, pixel height as positive values) assuming this is a north-up 
#' raster.
#'
#' \code{$dim()}
#' Returns an integer vector of length three containing the raster dimensions.
#' Equivalent to:
#' `c(ds$getRasterXSize(), ds$getRasterYSize(), ds$getRasterCount())`
#'
#' \code{$getRasterCount()}
#' Returns the number of raster bands on this dataset. For the methods 
#' described below that operate on individual bands, the \code{band} 
#' argument is the integer band number (1-based).
#'
#' \code{$getBlockSize(band)}
#' Returns an integer vector of length two (Xsize, Ysize) containing the 
#' "natural" block size of \code{band}.
#' GDAL has a concept of the natural block 
#' size of rasters so that applications can organize data access efficiently 
#' for some file formats. The natural block size is the block size that is 
#' most efficient for accessing the format. For many formats this is simply a 
#' whole row in which case block Xsize is the same as \code{$getRasterXSize()} 
#' and block Ysize is 1. However, for tiled images block size will typically 
#' be the tile size. Note that the X and Y block sizes don't have to divide 
#' the image size evenly, meaning that right and bottom edge blocks may be 
#' incomplete.
#'
#' \code{$getOverviewCount(band)}
#' Returns the number of overview layers available for \code{band}.
#'
#' \code{$buildOverviews(resampling, levels, bands)}
#' Build one or more raster overview images using the specified downsampling
#' algorithm.
#' \code{resampling} is one of "AVERAGE", "AVERAGE_MAGPHASE", "RMS",
#' "BILINEAR", "CUBIC", "CUBICSPLINE", "GAUSS", "LANCZOS", "MODE", "NEAREST",
#' or "NONE".
#' \code{levels} is an integer vector giving the list of overview decimation
#' factors to build (e.g., `c(2, 4, 8)`), or `0` to delete all overviews
#' (at least for external overviews (.ovr) and GTiff internal overviews).
#' \code{bands} is an integer vector giving a list of band numbers to build
#' overviews for, or `0` to build for all bands.
#' Note that for GTiff, overviews will be created internally if the dataset is
#' open in update mode, while external overviews (.ovr) will be created if the
#' dataset is open read-only.
#' Starting with GDAL 3.2, the GDAL_NUM_THREADS configuration option can be set
#' to "ALL_CPUS" or an integer value to specify the number of threads to use
#' for overview computation (see [set_config_option()]). 
#' No return value, called for side effects.
#'
#' \code{$getDataTypeName(band)}
#' Returns the name of the pixel data type for \code{band}. The possible data 
#' types are:
#' \tabular{rl}{
#'  `Unknown`   \tab  Unknown or unspecified type\cr
#'  `Byte`      \tab  8-bit unsigned integer\cr
#'  `Int8`      \tab  8-bit signed integer (GDAL >= 3.7)\cr
#'  `UInt16`    \tab  16-bit unsigned integer\cr
#'  `Int16`     \tab  16-bit signed integer\cr
#'  `UInt32`    \tab  32-bit unsigned integer\cr
#'  `Int32`     \tab  32-bit signed integer\cr
#'  `UInt64`    \tab  64-bit unsigned integer (GDAL >= 3.5)\cr
#'  `Int64`     \tab  64-bit signed integer (GDAL >= 3.5)\cr
#'  `Float32`   \tab  32-bit floating point\cr
#'  `Float64`   \tab  64-bit floating point\cr
#'  `CInt16`    \tab  Complex Int16\cr
#'  `CInt32`    \tab  Complex Int32\cr
#'  `CFloat32`  \tab  Complex Float32\cr
#'  `CFloat64`  \tab  Complex Float64
#' }
#' Some raster formats including GeoTIFF (`GTiff`) and Erdas Imagine .img 
#' (`HFA`) support sub-byte data types. Rasters can be created with these 
#' data types by specifying the `NBITS=n` creation option where n=1...7 for 
#' GTiff or n=1/2/4 for HFA. In these cases, \code{$getDataTypeName()} reports 
#' the apparent type `Byte`. GTiff also supports n=9...15 (`UInt16` type) and 
#' n=17...31 (`UInt32` type), and n=16 is accepted for `Float32` to generate 
#' half-precision floating point values.
#'
#' \code{$getStatistics(band, approx_ok, force)}
#' Returns a numeric vector of length four containing the minimum, maximum, 
#' mean and standard deviation of pixel values in \code{band} (excluding 
#' nodata pixels). Some raster formats will cache statistics allowing fast 
#' retrieval after the first request.
#'
#' \code{approx_ok}:
#'   * `TRUE`: Approximate statistics are sufficient, in which case overviews 
#'   or a subset of raster tiles may be used in computing the statistics.
#'   * `FALSE`: All pixels will be read and used to compute statistics (if 
#'   computation is forced).
#'
#' \code{force}:
#'   * `TRUE`: The raster will be scanned to compute statistics. Once computed,
#'   statistics will generally be “set” back on the raster band if the format 
#'   supports caching statistics.
#'   (Note: `ComputeStatistics()` in the GDAL API is called automatically here.
#'   This is a change in the behavior of `GetStatistics()` in the API, to a
#'   definitive `force`.)
#'   * `FALSE`: Results will only be returned if it can be done quickly (i.e., 
#'   without scanning the raster, typically by using pre-existing 
#'   STATISTICS_xxx metadata items). NAs will be returned if statistics cannot 
#'   be obtained quickly.
#'
#' \code{$getNoDataValue(band)}
#' Returns the nodata value for \code{band} if one exists.
#' This is generally a special value defined to mark pixels that are not
#' valid data. NA is returned if a nodata value is not defined for 
#' \code{band}. Not all raster formats support a designated nodata value.
#'
#' \code{$setNoDataValue(band, nodata_value)}
#' Sets the nodata value for \code{band}.
#' \code{nodata_value} is a numeric value to be defined as the nodata marker.
#' Depending on the format, changing the nodata value may or may not have an 
#' effect on the pixel values of a raster that has just been created (often  
#' not). It is thus advised to call \code{$fillRaster()} explicitly if the 
#' intent is to initialize the raster to the nodata value. In any case, 
#' changing an existing nodata value, when one already exists on an initialized
#' dataset, has no effect on the pixels whose values matched the previous 
#' nodata value.
#' Returns logical TRUE on success or FALSE if the nodata value could not 
#' be set.
#'
#' \code{$deleteNoDataValue(band)}
#' Removes the nodata value for \code{band}.
#' This affects only the definition of the nodata value for raster formats
#' that support one (does not modify pixel values). No return value, called 
#' for side effects. An error is raised if the nodata value cannot be removed.
#'
#' \code{$getUnitType(band)}
#' Returns the name of the unit type of the pixel values for \code{band} 
#' (e.g., "m" or "ft").
#' An empty string \code{""} is returned if no units are available.
#'
#' \code{$setUnitType(band, unit_type)}
#' Sets the name of the unit type of the pixel values for \code{band}.
#' `unit_type` should be one of "" (the default indicating it is unknown),
#' "m" indicating meters, or "ft" indicating feet, though other nonstandard
#' values are allowed.
#' Returns logical TRUE on success or FALSE if the unit type could not 
#' be set.
#'
#' \code{$getScale(band)}
#' Returns the pixel value scale (units value = (raw value * scale) + offset) 
#' for \code{band}.
#' This value (in combination with the \code{getOffset()} value) can be used to 
#' transform raw pixel values into the units returned by \code{getUnitType()}.
#' Returns NA if a scale value is not defined for this \code{band}.
#'
#' \code{$setScale(band, scale)}
#' Sets the pixel value scale (units value = (raw value * scale) + offset) 
#' for \code{band}. Many raster formats do not implement this method.
#' Returns logical TRUE on success or FALSE if the scale could not be set.
#'
#' \code{$getOffset(band)}
#' Returns the pixel value offset (units value = (raw value * scale) + offset) 
#' for \code{band}.
#' This value (in combination with the \code{getScale()} value) can be used to 
#' transform raw pixel values into the units returned by \code{getUnitType()}.
#' Returns NA if an offset value is not defined for this \code{band}.
#'
#' \code{$setOffset(band, offset)}
#' Sets the pixel value offset (units value = (raw value * scale) + offset) 
#' for \code{band}. Many raster formats do not implement this method.
#' Returns logical TRUE on success or FALSE if the offset could not be set.
#'
#' \code{$getMetadata(band, domain)}
#' Returns a character vector of all metadata `name=value` pairs that exist in 
#' the specified \code{domain}, or \code{""} (empty string) if there are no 
#' metadata items in \code{domain} (metadata in the context of the GDAL 
#' Raster Data Model: \url{https://gdal.org/user/raster_data_model.html}).
#' Set \code{band = 0} to retrieve dataset-level metadata, or to an integer 
#' band number to retrieve band-level metadata.
#' Set \code{domain = ""} (empty string) to retrieve metadata in the 
#' default domain.
#'
#' \code{$getMetadataItem(band, mdi_name, domain)}
#' Returns the value of a specific metadata item named \code{mdi_name} in the 
#' specified \code{domain}, or \code{""} (empty string) if no matching item 
#' is found.
#' Set \code{band = 0} to retrieve dataset-level metadata, or to an integer 
#' band number to retrieve band-level metadata.
#' Set \code{domain = ""} (empty string) to retrieve an item in the 
#' default domain.
#'
#' \code{$setMetadataItem(band, mdi_name, mdi_value, domain)}
#' Sets the value (\code{mdi_value}) of a specific metadata item named
#' \code{mdi_name} in the specified \code{domain}.
#' Set \code{band = 0} to set dataset-level metadata, or to an integer 
#' band number to set band-level metadata.
#' Set \code{domain = ""} (empty string) to set an item in the default domain.
#'
#' \code{$read(band, xoff, yoff, xsize, ysize, out_xsize, out_ysize)}
#' Reads a region of raster data from \code{band}. The method takes care of
#' pixel decimation / replication if the output size
#' (\code{out_xsize * out_ysize}) is different than the size of the region 
#' being accessed (\code{xsize * ysize}).
#' \code{xoff} is the pixel (column) offset to the top left corner of the
#' region of the band to be accessed (zero to start from the left side).
#' \code{yoff} is the line (row) offset to the top left corner of the region of
#' the band to be accessed (zero to start from the top).
#' \emph{Note that raster row/column offsets use 0-based indexing.}
#' \code{xsize} is the width in pixels of the region to be accessed.
#' \code{ysize} is the height in pixels of the region to be accessed.
#' \code{out_xsize} is the width of the output array into which the desired 
#' region will be read (typically the same value as xsize).
#' \code{out_ysize} is the height of the output array into which the desired 
#' region will be read (typically the same value as ysize).
#' Returns a numeric or complex vector containing the values that were read.
#' It is organized in left to right, top to bottom pixel order. 
#' NA will be returned in place of the nodata value if the 
#' raster dataset has a nodata value defined for this band.
#' Data are read as R `integer` type when possible for the raster data type
#' (Byte, Int8, Int16, UInt16, Int32), otherwise as type `double` (UInt32, 
#' Float32, Float64).
#' No rescaling of the data is performed (see \code{$getScale()} and 
#' \code{$getOffset()} above).
#' An error is raised if the read operation fails.
#'
#' \code{$write(band, xoff, yoff, xsize, ysize, rasterData)}
#' Writes a region of raster data to \code{band}.
#' \code{xoff} is the pixel (column) offset to the top left corner of the
#' region of the band to be accessed (zero to start from the left side).
#' \code{yoff} is the line (row) offset to the top left corner of the region of
#' the band to be accessed (zero to start from the top).
#' \emph{Note that raster row/column offsets use 0-based indexing.}
#' \code{xsize} is the width in pixels of the region to write.
#' \code{ysize} is the height in pixels of the region to write.
#' \code{rasterData} is a numeric or complex vector containing values to write.
#' It is organized in left to right, top to bottom pixel order. NA in 
#' \code{rasterData} should be replaced with a suitable nodata value prior to
#' writing (see \code{$getNoDataValue()} and \code{$setNoDataValue()} above).
#' An error is raised if the operation fails (no return value).
#'
#' \code{$fillRaster(band, value, ivalue)}
#' Fills \code{band} with a constant value. Used to clear a band to a specified
#' default value.
#' \code{value} is the fill value (real component).
#' \code{ivalue} is the imaginary component of fill value for a raster with
#' complex data type. Set \code{ivalue = 0} for real data types. No return 
#' value, called for side effects.
#'
#' \code{$getChecksum(band, xoff, yoff, xsize, ysize)}
#' Returns a 16-bit integer (0-65535) checksum from a region of raster data 
#' on `band`.
#' Floating point data are converted to 32-bit integer so decimal portions of
#' such raster data will not affect the checksum. Real and imaginary 
#' components of complex bands influence the result.
#' \code{xoff} is the pixel (column) offset of the window to read.
#' \code{yoff} is the line (row) offset of the window to read.
#' \emph{Raster row/column offsets use 0-based indexing.}
#' \code{xsize} is the width in pixels of the window to read.
#' \code{ysize} is the height in pixels of the window to read.
#'
#' \code{$close()}
#' Closes the GDAL dataset (no return value, called for side effects). 
#' Calling \code{$close()} results in proper cleanup, and flushing of any 
#' pending writes. Forgetting to close a dataset opened in update mode on some 
#' formats such as GTiff could result in being unable to open it afterwards. 
#' The `GDALRaster` object is still available after calling \code{$close()}. 
#' The dataset can be re-opened on the existing \code{filename} with 
#' \code{$open(read_only=TRUE)} or \code{$open(read_only=FALSE)}.
#'
#' @note
#' The `$read()` method will perform automatic resampling if the 
#' specified output size (`out_xsize * out_ysize`) is different than 
#' the size of the region being read (`xsize * ysize`). In that case, the 
#' GDAL_RASTERIO_RESAMPLING configuration option could also be defined to 
#' override the default resampling to one of BILINEAR, CUBIC, CUBICSPLINE, 
#' LANCZOS, AVERAGE or MODE (see [set_config_option()]).
#'
#' @seealso
#' Package overview in [`help("gdalraster-package")`][gdalraster-package]
#'
#' [create()], [createCopy()], [rasterFromRaster()], [rasterToVRT()]
#'
#' [fillNodata()], [warp()], [plot_raster()]
#'
#' `read_ds()` is a convenience wrapper for `GDALRaster$read()`.
#'
#' @examples
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds <- new(GDALRaster, lcp_file, read_only=TRUE)
#'
#' ## print information about the dataset to the console
#' ds$info()
#'
#' ## retrieve the raster format name
#' ds$getDriverShortName()
#' ds$getDriverLongName()
#'
#' ## retrieve dataset parameters
#' ds$getRasterXSize()
#' ds$getRasterYSize()
#' ds$getGeoTransform()
#' ds$getProjectionRef()
#' ds$getRasterCount()
#' ds$bbox()
#' ds$res()
#' ds$dim()
#'
#' ## retrieve some band-level parameters
#' ds$getBlockSize(band=1)
#' ds$getOverviewCount(band=1)
#' ds$getDataTypeName(band=1)
#' # LCP format does not support an intrinsic nodata value so this returns NA:
#' ds$getNoDataValue(band=1)
#'
#' ## LCP driver reports several dataset- and band-level metadata
#' ## see the format description at https://gdal.org/drivers/raster/lcp.html
#' ## set band=0 to retrieve dataset-level metadata
#' ## set domain="" (empty string) for the default metadata domain
#' ds$getMetadata(band=0, domain="")
#'
#' ## retrieve metadata for a band as a vector of name=value pairs
#' ds$getMetadata(band=4, domain="")
#'
#' ## retrieve the value of a specific metadata item
#' ds$getMetadataItem(band=2, mdi_name="SLOPE_UNIT_NAME", domain="")
#'
#' ## read one row of pixel values from band 1 (elevation)
#' ## raster row/column index are 0-based
#' ## the upper left corner is the origin
#' ## read the tenth row:
#' ncols <- ds$getRasterXSize()
#' rowdata <- ds$read(band=1, xoff=0, yoff=9, 
#'                     xsize=ncols, ysize=1, 
#'                     out_xsize=ncols, out_ysize=1)
#' head(rowdata)
#'
#' ds$close()
#'
#' ## create a new raster using lcp_file as a template
#' new_file <- paste0(tempdir(), "/", "storml_newdata.tif")
#' rasterFromRaster(srcfile = lcp_file,
#'                  dstfile = new_file,
#'                  nbands = 1,
#'                  dtName = "Byte",
#'                  init = -9999)
#' ds_new <- new(GDALRaster, new_file, read_only=FALSE)
#'
#' ## write random values to all pixels
#' set.seed(42)
#' ncols <- ds_new$getRasterXSize()
#' nrows <- ds_new$getRasterYSize()
#' for (row in 0:(nrows-1)) {
#'     rowdata <- round(runif(ncols, 0, 100))
#'     ds_new$write(band=1, xoff=0, yoff=row, xsize=ncols, ysize=1, rowdata)
#' }
#'
#' ## re-open in read-only mode when done writing
#' ## this will ensure flushing of any pending writes (implicit $close)
#' ds_new$open(read_only=TRUE)
#'
#' ## getStatistics returns min, max, mean, sd, and sets stats in the metadata
#' ds_new$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#' ds_new$getMetadataItem(band=1, "STATISTICS_MEAN", "")
#'
#' ## close the dataset for proper cleanup
#' ds_new$close()
#'
#' \donttest{
#' ## using a GDAL Virtual File System handler '/vsicurl/'
#' ## see: https://gdal.org/user/virtual_file_systems.html
#' url <- "/vsicurl/https://raw.githubusercontent.com/"
#' url <- paste0(url, "usdaforestservice/gdalraster/main/sample-data/")
#' url <- paste0(url, "lf_elev_220_mt_hood_utm.tif")
#'
#' ds <- new(GDALRaster, url, read_only=TRUE)
#' plot_raster(ds, main="Mount Hood elevation (m)")
#' ds$close()
#' }
NULL

Rcpp::loadModule("mod_GDALRaster", TRUE)
