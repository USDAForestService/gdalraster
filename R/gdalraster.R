#' @name GDALRaster-class
#'
#' @aliases
#' Rcpp_GDALRaster Rcpp_GDALRaster-class GDALRaster
#'
#' @title Class encapsulating a raster dataset and associated band objects
#'
#' @description
#' `GDALRaster` provides an interface for accessing a raster dataset via GDAL
#' and calling methods on the underlying `GDALDataset`, `GDALDriver` and
#' `GDALRasterBand` objects. See \url{https://gdal.org/en/stable/api/index.html} for
#' details of the GDAL Raster API.
#'
#' `GDALRaster` is a C++ class exposed directly to \R (via `RCPP_EXPOSED_CLASS`).
#' Fields and methods of the class are accessed using the `$` operator. **Note
#' that all arguments to class methods are required and must be given in the
#' order documented.** Naming the arguments is optional but may be preferred
#' for readability.
#'
#' @param filename Character string containing the file name of a raster
#' dataset to open, as full path or relative to the current working directory.
#' In some cases, `filename` may not refer to a local file system, but instead
#' contain format-specific information on how to access a dataset such
#' as database connection string, URL, /vsiPREFIX/, etc. (see GDAL
#' raster format descriptions:
#' \url{https://gdal.org/en/stable/drivers/raster/index.html}).
#' @param read_only Logical. `TRUE` to open the dataset read-only (the default),
#' or `FALSE` to open with write access.
#' @param open_options Optional character vector of `NAME=VALUE` pairs
#' specifying dataset open options.
#' @param shared Logical. `FALSE` to open the dataset without using shared
#' mode. Default is `TRUE` (see Note).
#' @returns An object of class `GDALRaster`, which contains a pointer to the
#' opened dataset.
#' Class methods that operate on the dataset are described in Details, along
#' with a set of writable fields for per-object settings. Values may be
#' assigned to the class fields as needed during the lifetime of the object
#' (i.e., by regular \code{<-} or \code{=} assignment).
#'
#' @section Usage (see Details):
#' \preformatted{
#' ## Constructors
#' # read-only by default:
#' ds <- new(GDALRaster, filename)
#' # for update access:
#' ds <- new(GDALRaster, filename, read_only = FALSE)
#' # to specify dataset open options:
#' ds <- new(GDALRaster, filename, read_only = TRUE|FALSE, open_options)
#' # to open without using shared mode:
#' new(GDALRaster, filename, read_only, open_options, shared = FALSE)
#'
#' ## Read/write fields (per-object settings)
#' ds$infoOptions
#' ds$quiet
#' ds$readByteAsRaw
#'
#' ## Methods
#' ds$getFilename()
#' ds$setFilename(filename)
#' ds$open(read_only)
#' ds$isOpen()
#' ds$getFileList()
#'
#' ds$info()
#' ds$infoAsJSON()
#'
#' ds$getDriverShortName()
#' ds$getDriverLongName()
#'
#' ds$getRasterXSize()
#' ds$getRasterYSize()
#' ds$getRasterCount()
#'
#' ds$addBand(dataType, options)
#'
#' ds$getGeoTransform()
#' ds$setGeoTransform(transform)
#'
#' ds$getProjection()
#' ds$getProjectionRef()
#' ds$setProjection(projection)
#'
#' ds$bbox()
#' ds$res()
#' ds$dim()
#' ds$apply_geotransform(col_row)
#' ds$get_pixel_line(xy)
#' ds$get_block_indexing(band)
#'
#' ds$getDescription(band)
#' ds$setDescription(band, desc)
#' ds$getBlockSize(band)
#' ds$getActualBlockSize(band, xblockoff, yblockoff)
#' ds$getOverviewCount(band)
#' ds$buildOverviews(resampling, levels, bands)
#' ds$getDataTypeName(band)
#' ds$getNoDataValue(band)
#' ds$setNoDataValue(band, nodata_value)
#' ds$deleteNoDataValue(band)
#' ds$getMaskFlags(band)
#' ds$getMaskBand(band)
#' ds$getUnitType(band)
#' ds$setUnitType(band, unit_type)
#' ds$getScale(band)
#' ds$setScale(band, scale)
#' ds$getOffset(band)
#' ds$setOffset(band, offset)
#' ds$getRasterColorInterp(band)
#' ds$setRasterColorInterp(band, col_interp)
#'
#' ds$getMinMax(band, approx_ok)
#' ds$getStatistics(band, approx_ok, force)
#' ds$clearStatistics()
#' ds$getHistogram(band, min, max, num_buckets, incl_out_of_range, approx_ok)
#' ds$getDefaultHistogram(band, force)
#'
#' ds$getMetadata(band, domain)
#' ds$setMetadata(band, metadata, domain)
#' ds$getMetadataItem(band, mdi_name, domain)
#' ds$setMetadataItem(band, mdi_name, mdi_value, domain)
#' ds$getMetadataDomainList(band)
#'
#' ds$read(band, xoff, yoff, xsize, ysize, out_xsize, out_ysize)
#' ds$write(band, xoff, yoff, xsize, ysize, rasterData)
#' ds$fillRaster(band, value, ivalue)
#'
#' ds$getColorTable(band)
#' ds$getPaletteInterp(band)
#' ds$setColorTable(band, col_tbl, palette_interp)
#' ds$clearColorTable(band)
#'
#' ds$getDefaultRAT(band)
#' ds$setDefaultRAT(band, df)
#'
#' ds$flushCache()
#'
#' ds$getChecksum(band, xoff, yoff, xsize, ysize)
#'
#' ds$close()
#' }
#' @section Details:
#' ## Constructors
#'
#' \code{new(GDALRaster, filename, read_only)}\cr
#' Returns an object of class `GDALRaster`. The `read_only` argument defaults
#' to `TRUE` if not specified.
#'
#' \code{new(GDALRaster, filename, read_only, open_options)}\cr
#' Alternate constructor for passing dataset `open_options`, a character
#' vector of `NAME=VALUE` pairs.
#' `read_only` is required for this form of the constructor, `TRUE` for
#' read-only access, or `FALSE` to open with write access.
#' Returns an object of class `GDALRaster`.
#'
#' \code{new(GDALRaster, filename, read_only, open_options, shared)}\cr
#' Alternate constructor for specifying the `shared` mode for dataset opening.
#' The `shared` argument defaults to `TRUE` but can be set to `FALSE` with this
#' constructor (see Note).
#' All arguments are required with this form of the constructor, but
#' `open_options` can be `NULL`. Returns an object of class `GDALRaster`.
#'
#' ## Read/write fields
#'
#' \code{$infoOptions}\cr
#' A character vector of command-line arguments to control the output of
#' \code{$info()} and \code{$infoAsJSON()} (see below).
#' Defaults to `character(0)`. Can be set to a vector of strings specifying
#' arguments to the \command{gdalinfo} command-line utility, e.g.,
#' `c("-nomd", "-norat", "-noct")`.
#' Restore the default by setting to empty string (`""`) or `character(0)`.
#'
#' \code{$quiet}\cr
#' A logical value, `FALSE` by default. This field can be set to `TRUE` which
#' will suppress various messages as well as progress reporting for potentially
#' long-running processes such as building overviews and computation of
#' statistics and histograms.
#'
#' \code{$readByteAsRaw}\cr
#' A logical value, `FALSE` by default. This field can be set to `TRUE` which
#' will affect the data type returned by the \code{$read()} method and the
#' [read_ds()] convenience function. When the underlying band data type is Byte
#' and `readByteAsRaw` is `TRUE` the output type will be raw rather than
#' integer. See also the `as_raw` argument to [read_ds()] to control this in a
#' non-persistent setting. If the underlying band data type is not Byte this
#' setting has no effect.
#'
#' ## Methods
#'
#' \code{$getFilename()}\cr
#' Returns a character string containing the `filename` associated with this
#' `GDALRaster` object (`filename` originally used to open the dataset). May
#' be a regular filename, database connection string, URL, etc.
#'
#' \code{$setFilename(filename)}\cr
#' Sets the `filename` if the underlying dataset does not already have an
#' associated filename. Explicitly setting the filename is an advanced
#' setting that should only be used when the user has determined that it is
#' needed. Writing certain virtual datasets to file is one potential use case
#' (e.g., a dataset returned by `autoCreateWarpedVRT()`).
#'
#' \code{$open(read_only)}\cr
#' (Re-)opens the raster dataset on the existing filename. Use this method to
#' open a dataset that has been closed using \code{$close()}. May be used to
#' re-open a dataset with a different read/write access (`read_only` set to
#' `TRUE` or `FALSE`). The method will first close an open dataset, so it is
#' not required to call \code{$close()} explicitly in this case.
#' No return value, called for side effects.
#'
#' \code{$isOpen()}\cr
#' Returns logical indicating whether the associated raster dataset is open.
#'
#' \code{$getFileList()}\cr
#' Returns a character vector of files believed to be part of this dataset.
#' If it returns an empty string (`""`) it means there is believed to be no
#' local file system files associated with the dataset (e.g., a virtual file
#' system). The returned filenames will normally be relative or absolute
#' paths depending on the path used to originally open the dataset.
#'
#' \code{$info()}\cr
#' Prints various information about the raster dataset to the console (no
#' return value, called for that side effect only).
#' Equivalent to the output of the \command{gdalinfo} command-line utility
#' (\command{gdalinfo filename}, if using the default `infoOptions`).
#' See the field \code{$infoOptions} above for setting the arguments to
#' `gdalinfo`.
#'
#' \code{$infoAsJSON()}\cr
#' Returns information about the raster dataset as a JSON-formatted string.
#' Equivalent to the output of the \command{gdalinfo} command-line utility
#' (\command{gdalinfo -json filename}, if using the default `infoOptions`).
#' See the field \code{$infoOptions} above for setting the arguments to
#' `gdalinfo`.
#'
#' \code{$getDriverShortName()}\cr
#' Returns the short name of the raster format driver.
#'
#' \code{$getDriverLongName()}\cr
#' Returns the long name of the raster format driver.
#'
#' \code{$getRasterXSize()}\cr
#' Returns the number of pixels along the x dimension.
#'
#' \code{$getRasterYSize()}\cr
#' Returns the number of pixels along the y dimension.
#'
#' \code{$getRasterCount()}\cr
#' Returns the number of raster bands on this dataset. For the methods
#' described below that operate on individual bands, the \code{band}
#' argument is the integer band number (1-based).
#'
#' \code{$addBand(dataType, options)}\cr
#' Adds a band to a dataset if the underlying format supports this action.
#' Most formats do not, but `"MEM"` and `"VRT"` are notable exceptions
#' that support adding bands. The added band will always be the last band.
#' `dataType` is a character string containing the data type name
#' (e.g., `"Byte"`, `"Int16"`, `"UInt16"`, `"Int32"`, `"Float32"`, etc).
#' The `options` argument is a character vector of NAME=VALUE option strings.
#' Supported options are format specific. Note that the `options` argument is
#' required but may be given as `NULL`. Returns logical \code{TRUE} on success
#' or \code{FALSE} if a band could not be added.
#'
#' \code{$getGeoTransform()}\cr
#' Returns the affine transformation coefficients for transforming between
#' pixel/line raster space (column/row) and projection coordinate space
#' (geospatial x/y). The return value is a numeric vector of length six.
#' See \url{https://gdal.org/en/stable/tutorials/geotransforms_tut.html}
#' for details of the affine transformation. \emph{With 1-based indexing
#' in \R}, the geotransform vector contains (in map units of the raster spatial
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
#' \code{$setGeoTransform(transform)}\cr
#' Sets the affine transformation coefficients on this dataset.
#' \code{transform} is a numeric vector of length six.
#' Returns logical \code{TRUE} on success or \code{FALSE} if the geotransform
#' could not be set.
#'
#' \code{$getProjection()}\cr
#' Returns the coordinate reference system of the raster as an OGC WKT
#' format string. Equivalent to \code{ds$getProjectionRef()}.
#'
#' \code{$getProjectionRef()}\cr
#' Returns the coordinate reference system of the raster as an OGC WKT
#' format string.
#' An empty string is returned when a projection definition is not available.
#'
#' \code{$setProjection(projection)}\cr
#' Sets the projection reference for this dataset.
#' \code{projection} is a string in OGC WKT format.
#' Returns logical \code{TRUE} on success or \code{FALSE} if the projection
#' could not be set.
#'
#' \code{$bbox()}\cr
#' Returns a numeric vector of length four containing the bounding box
#' (xmin, ymin, xmax, ymax).
#'
#' \code{$res()}\cr
#' Returns a numeric vector of length two containing the resolution
#' (pixel width, pixel height as positive values) for a non-rotated raster.
#' A warning is emitted and `NA` values returned if the raster has a rotated
#' geotransform (see method \code{$getGeoTransform()} above).
#'
#' \code{$dim()}\cr
#' Returns an integer vector of length three containing the raster dimensions
#' (xsize, ysize, number of bands).
#' Equivalent to:
#' ```
#' c(ds$getRasterXSize(), ds$getRasterYSize(), ds$getRasterCount())
#' ```
#'
#' \code{$apply_geotransform(col_row)}\cr
#' Applies geotransform coefficients to raster coordinates in pixel/line space
#' (column/row), converting into georeferenced (x/y) coordinates.
#' `col_row` is a numeric matrix of raster col/row coordinates (or two-column
#' data frame that will be coerced to numeric matrix).
#' Returns a numeric matrix of geospatial x/y coordinates.
#' See the stand-alone function of the same name ([apply_geotransform()]) for
#' more info and examples.
#'
#' \code{$get_pixel_line(xy)}\cr
#' Converts geospatial coordinates to pixel/line (raster column/row numbers).
#' `xy` is a numeric matrix of geospatial x,y coordinates in the same spatial
#' reference system as the raster (or two-column data frame that will be
#' coerced to numeric matrix). Returns an integer matrix of raster pixel/line.
#' See the stand-alone function of the same name ([get_pixel_line()]) for more
#' info and examples.
#'
#' \code{$get_block_indexing(band)}\cr
#' Helper method returning a numeric matrix with named columns: `xblockoff`,
#' `yblockoff`, `xoff`, `yoff`, `xsize`, `ysize`, `xmin`, `xmax`, `ymin`,
#' `ymax`. For the meanings of these names, refer to the following class
#' methods below: \code{$getBlockSize()}, \code{$getActualBlockSize} and
#' \code{$read()}.
#' All offsets are zero-based. The columns `xmin`, `xmax`, `ymin` and
#' `ymax` give the extent of each block in geospatial coordinates.
#' This method provides indexing values for the block layout of the given
#' `band` number. The returned matrix has number of rows equal to the number
#' of blocks comprising the band, with blocks ordered left to right, top
#' to bottom. The `xoff`/`yoff` values are pixel offsets to the start of a
#' block. The `xsize`/`ysize` values give the actual block sizes accounting
#' for potentially incomplete blocks along the right and bottom edges.
#'
#' \code{$getDescription(band)}\cr
#' Returns a string containing the description for \code{band}. An empty
#' string is returned if no description is set for the band.
#' Passing `band = 0` will return the dataset-level description.
#'
#' \code{$setDescription(band, desc)}\cr
#' Sets a description for \code{band}. \code{desc} is the character string
#' to set. No return value. (Passing `band = 0` can be used to set the
#' dataset-level description. Note that the dataset description is generally
#' the filename that was used to open the dataset. It usually should not be
#' changed by calling this method on an existing dataset.)
#'
#' \code{$getBlockSize(band)}\cr
#' Returns an integer vector of length two (xsize, ysize) containing the
#' "natural" block size of \code{band}. GDAL has a concept of the natural block
#' size of rasters so that applications can organize data access efficiently
#' for some file formats. The natural block size is the block size that is
#' most efficient for accessing the format. For many formats this is simply a
#' whole row in which case block xsize is the same as \code{$getRasterXSize()}
#' and block ysize is 1. However, for tiled images block size will typically
#' be the tile size. Note that the X and Y block sizes don't have to divide
#' the image size evenly, meaning that right and bottom edge blocks may be
#' incomplete.
#'
#' \code{$getActualBlockSize(band, xblockoff, yblockoff)}\cr
#' Returns an integer vector of length two (xvalid, yvalid) containing the
#' actual block size for a given block offset in \code{band}. Handles partial
#' blocks at the edges of the raster and returns the true number of pixels.
#' `xblockoff` is an integer value, the horizontal block offset for which to
#' calculate the number of valid pixels, with zero indicating the left most
#' block, 1 the next block, etc. `yblockoff` is likewise the vertical block
#' offset, with zero indicating the top most block, 1 the next block, etc.
#'
#' \code{$getOverviewCount(band)}\cr
#' Returns the number of overview layers (a.k.a. pyramids) available for
#' \code{band}.
#'
#' \code{$buildOverviews(resampling, levels, bands)}\cr
#' Build one or more raster overview images using the specified downsampling
#' algorithm.
#' \code{resampling} is a character string, one of `AVERAGE`,
#' `AVERAGE_MAGPHASE`, `RMS`, `BILINEAR`, `CUBIC`, `CUBICSPLINE`, `GAUSS`,
#' `LANCZOS`, `MODE`, `NEAREST` or `NONE`.
#' \code{levels} is an integer vector giving the list of overview decimation
#' factors to build (e.g., `c(2, 4, 8)`), or `0` to delete all overviews
#' (at least for external overviews (.ovr) and GTiff internal overviews).
#' \code{bands} is an integer vector giving a list of band numbers to build
#' overviews for, or `0` to build for all bands.
#' Note that for GTiff, overviews will be created internally if the dataset is
#' open in update mode, while external overviews (.ovr) will be created if the
#' dataset is open read-only.
#' External overviews created in GTiff format may be compressed using the
#' `COMPRESS_OVERVIEW` configuration option. All compression methods supported
#' by the GTiff driver are available
#' (e.g., `set_config_option("COMPRESS_OVERVIEW", "LZW")`).
#' Since GDAL 3.6, `COMPRESS_OVERVIEW` is honored when creating internal
#' overviews of GTiff files. The [GDAL documentation for
#' `gdaladdo`](https://gdal.org/en/stable/programs/gdaladdo.html) command-line utility
#' describes additional configuration for overview building.
#' See also [set_config_option()]. No return value, called for side effects.
#'
#' \code{$getDataTypeName(band)}\cr
#' Returns the name of the pixel data type for \code{band}. The possible data
#' types are:
#' \tabular{rl}{
#'  Unknown   \tab  Unknown or unspecified type\cr
#'  Byte      \tab  8-bit unsigned integer\cr
#'  Int8      \tab  8-bit signed integer (GDAL >= 3.7)\cr
#'  UInt16    \tab  16-bit unsigned integer\cr
#'  Int16     \tab  16-bit signed integer\cr
#'  UInt32    \tab  32-bit unsigned integer\cr
#'  Int32     \tab  32-bit signed integer\cr
#'  UInt64    \tab  64-bit unsigned integer (GDAL >= 3.5)\cr
#'  Int64     \tab  64-bit signed integer (GDAL >= 3.5)\cr
#'  Float32   \tab  32-bit floating point\cr
#'  Float64   \tab  64-bit floating point\cr
#'  CInt16    \tab  Complex Int16\cr
#'  CInt32    \tab  Complex Int32\cr
#'  CFloat32  \tab  Complex Float32\cr
#'  CFloat64  \tab  Complex Float64
#' }
#' Some raster formats including GeoTIFF ("GTiff") and Erdas Imagine .img
#' ("HFA") support sub-byte data types. Rasters can be created with these
#' data types by specifying the "NBITS=n" creation option where n=1...7 for
#' GTiff or n=1/2/4 for HFA. In these cases, \code{$getDataTypeName()} reports
#' the apparent type `"Byte"`. GTiff also supports n=9...15 (UInt16 type) and
#' n=17...31 (UInt32 type), and n=16 is accepted for Float32 to generate
#' half-precision floating point values.
#'
#' \code{$getNoDataValue(band)}\cr
#' Returns the nodata value for \code{band} if one exists.
#' This is generally a special value defined to mark pixels that are not
#' valid data. \code{NA} is returned if a nodata value is not defined for
#' \code{band}. Not all raster formats support a designated nodata value.
#'
#' \code{$setNoDataValue(band, nodata_value)}\cr
#' Sets the nodata value for \code{band}.
#' \code{nodata_value} is a numeric value to be defined as the nodata marker.
#' Depending on the format, changing the nodata value may or may not have an
#' effect on the pixel values of a raster that has just been created (often
#' not). It is thus advised to call \code{$fillRaster()} explicitly if the
#' intent is to initialize the raster to the nodata value. In any case,
#' changing an existing nodata value, when one already exists on an initialized
#' dataset, has no effect on the pixels whose values matched the previous
#' nodata value.
#' Returns logical \code{TRUE} on success or \code{FALSE} if the nodata value
#' could not be set.
#'
#' \code{$deleteNoDataValue(band)}\cr
#' Removes the nodata value for \code{band}.
#' This affects only the definition of the nodata value for raster formats
#' that support one (does not modify pixel values). No return value.
#' An error is raised if the nodata value cannot be removed.
#'
#' \code{$getMaskFlags(band)}\cr
#' Returns the status flags of the mask band associated with \code{band}.
#' Masks are represented as Byte bands with a value of zero indicating nodata
#' and non-zero values indicating valid data. Normally the value `255` will be
#' used for valid data pixels. GDAL supports external (.msk) mask bands, and
#' normal Byte alpha (transparency) band as mask (any value other than `0` to
#' be treated as valid data). But masks may not be regular raster bands on the
#' datasource, such as an implied mask from a band nodata value or the
#' `ALL_VALID` mask. See RFC 15: Band Masks for more details
#' (\url{https://gdal.org/en/stable/development/rfc/rfc15_nodatabitmask.html}.
#'
#' Returns a named list of GDAL mask flags and their logical values, with the
#' following definitions:
#' * `ALL_VALID`: There are no invalid pixels, all mask values will be `255`.
#' When used this will normally be the only flag set.
#' * `PER_DATASET`: The mask band is shared between all bands on the dataset.
#' * `ALPHA`: The mask band is actually an alpha band and may have values other
#' than `0` and `255`.
#' * `NODATA`: Indicates the mask is actually being generated from nodata
#' values (mutually exclusive of `ALPHA`).
#'
#' \code{$getMaskBand(band)}\cr
#' Returns the mask filename and band number associated with \code{band}.
#' The return value is a named list with two elements. The `MaskFile` element
#' gives the filename where the mask band is located, e.g., a file with the
#' same name as the main dataset but suffixed with .msk in the case of a GDAL
#' external mask file. `MaskFile` will be an empty string for the derived
#' `ALL_VALID` and `NODATA` masks, which internally are freestanding bands not
#' considered to be a part of a dataset. The `MaskBand` element gives the band
#' number for a mask that is a regular alpha band in the main dataset or
#' external mask file. `BandNumber` will be `0` for the `ALL_VALID` and `NODATA`
#' masks.
#'
#' \code{$getUnitType(band)}\cr
#' Returns the name of the unit type of the pixel values for \code{band}
#' (e.g., "m" or "ft").
#' An empty string (`""`) is returned if no units are available.
#'
#' \code{$setUnitType(band, unit_type)}\cr
#' Sets the name of the unit type of the pixel values for \code{band}.
#' `unit_type` should be one of empty string `""` (the default indicating it is
#' unknown), "m" indicating meters, or "ft" indicating feet, though other
#' nonstandard values are allowed.
#' Returns logical \code{TRUE} on success or \code{FALSE} if the unit type
#' could not be set.
#'
#' \code{$getScale(band)}\cr
#' Returns the pixel value scale (units value = (raw value * scale) + offset)
#' for \code{band}.
#' This value (in combination with the \code{$getOffset()} value) can be used to
#' transform raw pixel values into the units returned by \code{$getUnitType()}.
#' Returns \code{NA} if a scale value is not defined for this \code{band}.
#'
#' \code{$setScale(band, scale)}\cr
#' Sets the pixel value scale (units value = (raw value * scale) + offset)
#' for \code{band}. Many raster formats do not implement this method.
#' Returns logical \code{TRUE} on success or \code{FALSE} if the scale could
#' not be set.
#'
#' \code{$getOffset(band)}\cr
#' Returns the pixel value offset (units value = (raw value * scale) + offset)
#' for \code{band}.
#' This value (in combination with the \code{$getScale()} value) can be used to
#' transform raw pixel values into the units returned by \code{$getUnitType()}.
#' Returns \code{NA} if an offset value is not defined for this \code{band}.
#'
#' \code{$setOffset(band, offset)}\cr
#' Sets the pixel value offset (units value = (raw value * scale) + offset)
#' for \code{band}. Many raster formats do not implement this method.
#' Returns logical \code{TRUE} on success or \code{FALSE} if the offset could
#' not be set.
#'
#' \code{$getRasterColorInterp(band)}\cr
#' Returns a string describing the color interpretation for \code{band}.
#' The color interpretation values and their meanings are:
#' \tabular{rl}{
#'  Undefined  \tab Undefined\cr
#'  Gray       \tab Grayscale\cr
#'  Palette    \tab Paletted (see associated color table)\cr
#'  Red        \tab Red band of RGBA image\cr
#'  Green      \tab Green band of RGBA image\cr
#'  Blue       \tab Blue band of RGBA image\cr
#'  Alpha      \tab Alpha (0=transparent, 255=opaque)\cr
#'  Hue        \tab Hue band of HLS image\cr
#'  Saturation \tab Saturation band of HLS image\cr
#'  Lightness  \tab Lightness band of HLS image\cr
#'  Cyan       \tab Cyan band of CMYK image\cr
#'  Magenta    \tab Magenta band of CMYK image\cr
#'  Yellow     \tab Yellow band of CMYK image\cr
#'  Black      \tab Black band of CMYK image\cr
#'  YCbCr_Y    \tab Y Luminance\cr
#'  YCbCr_Cb   \tab Cb Chroma\cr
#'  YCbCr_Cr   \tab Cr Chroma
#' }
#'
#' \code{$setRasterColorInterp(band, col_interp)}\cr
#' Sets the color interpretation for \code{band}. See above for the list of
#' valid values for \code{col_interp} (passed as a string).
#'
#' \code{$getMinMax(band, approx_ok)}\cr
#' Returns a numeric vector of length two containing the min/max values for
#' \code{band}. If \code{approx_ok} is `TRUE` and the raster format knows these
#' values intrinsically then those values will be returned. If that doesn't
#' work, a subsample of blocks will be read to get an approximate min/max. If
#' the band has a nodata value it will be excluded from the minimum and
#' maximum. If \code{approx_ok} is `FALSE`, then all pixels will be read and
#' used to compute an exact range.
#'
#' \code{$getStatistics(band, approx_ok, force)}\cr
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
#'   STATISTICS_xxx metadata items). \code{NA}s will be returned if statistics
#'   cannot be obtained quickly.
#'
#' \code{$clearStatistics()}\cr
#' Clear statistics. Only implemented for now in PAM supported datasets
#' (Persistable Auxiliary Metadata via .aux.xml file). GDAL >= 3.2.
#'
#' \code{$getHistogram(band, min, max, num_buckets, incl_out_of_range, approx_ok)}\cr
#' Computes raster histogram for \code{band}. \code{min} is the lower bound of
#' the histogram. \code{max} is the upper bound of the histogram.
#' \code{num_buckets} is the number of buckets to use (bucket size is
#' \code{(max - min) / num_buckets}).
#' \code{incl_out_of_range} is a logical scalar: if `TRUE` values below the
#' histogram range will be mapped into the first bucket and values above will
#' be mapped into the last bucket, if `FALSE` out of range values are discarded.
#' \code{approx_ok} is a logical scalar: `TRUE` if an approximate histogram is
#' OK (generally faster), or `FALSE` for an exactly computed histogram.
#' Returns the histogram as a numeric vector of length \code{num_buckets}.
#'
#' \code{$getDefaultHistogram(band, force)}\cr
#' Returns a default raster histogram for \code{band}. In the GDAL API, this
#' method is overridden by derived classes (such as GDALPamRasterBand,
#' VRTDataset, HFADataset...) that may be able to fetch efficiently an already
#' stored histogram. \code{force} is a logical scalar: `TRUE` to force the
#' computation of a default histogram; or if `FALSE` and no default histogram
#' is available, a warning is emitted and the returned list has a 0-length
#' histogram vector.
#' Returns a list of length four containing named elements `min` (lower
#' bound), `max` (upper bound), `num_buckets` (number of buckets), and
#'`histogram` (a numeric vector of length `num_buckets`).
#'
#' \code{$getMetadata(band, domain)}\cr
#' Returns a character vector of all metadata `NAME=VALUE` pairs that exist in
#' the specified \code{domain}, or empty string (`""`) if there are no
#' metadata items in \code{domain} (metadata in the context of the GDAL
#' Raster Data Model: \url{https://gdal.org/en/stable/user/raster_data_model.html}).
#' Set \code{band = 0} to retrieve dataset-level metadata, or to an integer
#' band number to retrieve band-level metadata.
#' Set \code{domain = ""} (empty string) to retrieve metadata in the
#' default domain.
#'
#' \code{$setMetadata(band, metadata, domain)}\cr
#' Sets metadata in the specified \code{domain}. The \code{metadata} argument
#' is given as a character vector of `NAME=VALUE` pairs.
#' Pass \code{band = 0} to set dataset-level metadata, or pass an integer
#' band number to set band-level metadata.
#' Use \code{domain = ""} (empty string) to set an item in the default domain.
#' Returns logical \code{TRUE} on success or \code{FALSE} if metadata could
#' not be set.
#'
#' \code{$getMetadataItem(band, mdi_name, domain)}\cr
#' Returns the value of a specific metadata item named \code{mdi_name} in the
#' specified \code{domain}, or empty string (`""`) if no matching item
#' is found.
#' Set \code{band = 0} to retrieve dataset-level metadata, or to an integer
#' band number to retrieve band-level metadata.
#' Set \code{domain = ""} (empty string) to retrieve an item in the
#' default domain.
#'
#' \code{$setMetadataItem(band, mdi_name, mdi_value, domain)}\cr
#' Sets the value (\code{mdi_value}) of a specific metadata item named
#' \code{mdi_name} in the specified \code{domain}.
#' Pass \code{band = 0} to set dataset-level metadata, or pass an integer
#' band number to set band-level metadata.
#' Use \code{domain = ""} (empty string) to set an item in the default domain.
#' Returns logical \code{TRUE} on success or \code{FALSE} if metadata could
#' not be set.
#'
#' \code{$getMetadataDomainList(band)}\cr
#' Returns a character vector of metadata domains or empty string (`""`).
#' Set \code{band = 0} to retrieve dataset-level domains, or to an integer
#' band number to retrieve band-level domains.
#'
#' \code{$read(band, xoff, yoff, xsize, ysize, out_xsize, out_ysize)}\cr
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
#' \code{NA} will be returned in place of the nodata value if the
#' raster dataset has a nodata value defined for this band.
#' Data are read as \R integer type when possible for the raster data type
#' (`Byte`, `Int8`, `Int16`, `UInt16`, `Int32`), otherwise as type double
#' (`UInt32`, `Float32`, `Float64`).
#' No rescaling of the data is performed (see \code{$getScale()} and
#' \code{$getOffset()} above).
#' An error is raised if the read operation fails. See also the setting
#' \code{$readByteAsRaw} above.
#'
#' \code{$write(band, xoff, yoff, xsize, ysize, rasterData)}\cr
#' Writes a region of raster data to \code{band}.
#' \code{xoff} is the pixel (column) offset to the top left corner of the
#' region of the band to be accessed (zero to start from the left side).
#' \code{yoff} is the line (row) offset to the top left corner of the region of
#' the band to be accessed (zero to start from the top).
#' \emph{Note that raster row/column offsets use 0-based indexing.}
#' \code{xsize} is the width in pixels of the region to write.
#' \code{ysize} is the height in pixels of the region to write.
#' \code{rasterData} is a numeric or complex vector containing values to write.
#' It is organized in left to right, top to bottom pixel order. \code{NA} in
#' \code{rasterData} should be replaced with a suitable nodata value prior to
#' writing (see \code{$getNoDataValue()} and \code{$setNoDataValue()} above).
#' An error is raised if the operation fails (no return value).
#'
#' \code{$fillRaster(band, value, ivalue)}\cr
#' Fills `band` with a constant value. GDAL makes no guarantees about what
#' values the pixels in newly created files are set to, so this method can be
#' used to clear a band to a specified "default" value. The fill `value` is
#' passed as `numeric`, but this will be converted to the underlying raster
#' data type before writing to the file. The `ivalue` argument allows setting
#' the imaginary component of a complex value. Note that `ivalue` is a required
#' argument but can be set to `0` for real data types. No return value. An
#' error is raised if the operation fails.
#'
#' \code{$getColorTable(band)}\cr
#' Returns the color table associated with \code{band}, or \code{NULL} if
#' there is no associated color table. The color table is returned as an
#' integer matrix with five columns. To associate a color with a raster pixel,
#' the pixel value is used as a subscript into the color table. This means that
#' the colors are always applied starting at zero and ascending
#' (see \href{https://gdal.org/en/stable/user/raster_data_model.html#color-table}{GDAL
#' Color Table}).
#' Column 1 contains the pixel values. Interpretation of columns 2:5 depends
#' on the value of \code{$getPaletteInterp()} (see below).
#' For "RGB", columns 2:5 contain red, green, blue, alpha as 0-255 integer
#' values.
#'
#' \code{$getPaletteInterp(band)}\cr
#' If \code{band} has an associated color table, this method returns a
#' character string with the palette interpretation for columns 2:5 of the
#' table. An empty string (\code{""}) is returned if \code{band} does not have
#' an associated color table. The palette interpretation values and their
#' meanings are:
#' * Gray: column 2 contains grayscale values (columns 3:5 unused)
#' * RGB: columns 2:5 contain red, green, blue, alpha
#' * CMYK: columns 2:5 contain cyan, magenta, yellow, black
#' * HLS: columns 2:4 contain hue, lightness, saturation (column 5 unused)
#'
#' \code{$setColorTable(band, col_tbl, palette_interp)}\cr
#' Sets the raster color table for \code{band}
#' (see \href{https://gdal.org/en/stable/user/raster_data_model.html#color-table}{GDAL
#' Color Table}).
#' \code{col_tbl} is an integer matrix or data frame with either four or five
#' columns (see \code{$getColorTable()} above). Column 1 contains the pixel
#' values. Valid values are integers 0 and larger (note that GTiff format
#' supports color tables only for Byte and UInt16 bands). Negative values
#' will be skipped with a warning emitted. Interpretation of columns 2:5
#' depends on the value of \code{$getPaletteInterp()} (see above). For RGB,
#' columns 2:4 contain red, green, blue as 0-255 integer values, and an
#' optional column 5 contains alpha transparency values (defaults to 255
#' opaque).
#' \code{palette_interp} is a string, one of `Gray`, `RGB`, `CMYK` or `HLS`
#' (see \code{$getPaletteInterp()} above).
#' Returns logical \code{TRUE} on success or \code{FALSE} if the color table
#' could not be set.
#'
#' \code{$clearColorTable(band)}\cr
#' Clears the raster color table for \code{band}.
#' Returns logical \code{TRUE} on success or \code{FALSE} if the color table
#' could not be cleared, e.g., if this action is not supported by the driver.
#'
#' \code{$getDefaultRAT(band)}\cr
#' Returns the Raster Attribute Table for \code{band} as a data frame,
#' or \code{NULL} if there is no associated Raster Attribute Table. See the
#' stand-alone function [buildRAT()] for details of the Raster Attribute Table
#' format.
#'
#' \code{$setDefaultRAT(band, df)}\cr
#' Sets a default Raster Attribute Table for \code{band} from data frame `df`.
#' The input data frame will be checked for attribute `"GDALRATTableType"`
#' which can have values of `"thematic"` or `"athematic"` (for continuous data).
#' Columns of the data frame will be checked for attribute `"GFU"` (for "GDAL
#' field usage"). If the `"GFU"` attribute is missing, a value of `"Generic"`
#' will be used (corresponding to `GFU_Generic` in the GDAL API, for general
#' purpose field). Columns with other, specific field usage values should
#' generally be present in `df`, such as fields containing the set of unique
#' (discrete) pixel values (GFU `"MinMax"`), pixel counts (GFU `"PixelCount"`),
#' class names (GFU `"Name"`), color values (GFUs `"Red"`, "`Green"`, `"Blue"`),
#' etc. The data frame will also be checked for attributes `"Row0Min"` and
#' `"BinSize"` which can have numeric values that describe linear binning.
#' See the stand-alone function [buildRAT()] for details of the GDAL Raster
#' Attribute Table format and its representation as data frame.
#'
#' \code{$flushCache()}\cr
#' Flush all write cached data to disk. Any raster data written via GDAL calls,
#' but buffered internally will be written to disk. Using this method does not
#' preclude calling \code{$close()} to properly close the dataset and ensure
#' that important data not addressed by \code{$flushCache()} is written in the
#' file (see also \code{$open()} above). No return value, called for side
#' effects.
#'
#' \code{$getChecksum(band, xoff, yoff, xsize, ysize)}\cr
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
#' \code{$close()}\cr
#' Closes the GDAL dataset (no return value, called for side effects).
#' Calling \code{$close()} results in proper cleanup, and flushing of any
#' pending writes. Forgetting to close a dataset opened in update mode on some
#' formats such as GTiff could result in being unable to open it afterwards.
#' The `GDALRaster` object is still available after calling \code{$close()}.
#' The dataset can be re-opened on the existing \code{filename} with
#' \code{$open(read_only=TRUE)} or \code{$open(read_only=FALSE)}.
#'
#' @note
#' If a dataset object is opened with update access (`read_only = FALSE`), it
#' is not recommended to open a new dataset on the same underlying `filename`.
#'
#' Datasets are opened in shared mode by default. This allows the sharing of
#' `GDALDataset` handles for a dataset with other callers that open shared on
#' the same `filename`, if the dataset is opened from the same thread.
#' Functions in `gdalraster` that do processing will open input datasets in
#' shared mode. This provides potential efficiency for cases when an object of
#' class `GDALRaster` is already open in read-only mode on the same `filename`
#' (avoids overhead associated with initial dataset opening by using the
#' existing handle, and potentially makes use of existing data in the GDAL
#' block cache). Opening in shared mode can be disabled by specifying the
#' optional `shared` parameter in the class constructor.
#'
#' The \code{$read()} method will perform automatic resampling if the
#' specified output size (`out_xsize * out_ysize`) is different than
#' the size of the region being read (`xsize * ysize`). In that case, the
#' `GDAL_RASTERIO_RESAMPLING` configuration option could also be set to
#' override the default resampling to one of `BILINEAR`, `CUBIC`,
#' `CUBICSPLINE`, `LANCZOS`, `AVERAGE` or `MODE` (see [set_config_option()]).
#'
#' @seealso
#' Package overview in [`help("gdalraster-package")`][gdalraster-package]
#'
#' `vignette("raster-api-tutorial")`
#'
#' [read_ds()] is a convenience wrapper for `GDALRaster$read()`
#'
#' @examples
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds <- new(GDALRaster, lcp_file)
#' ds
#'
#' ## print information about the dataset to the console
#' ds$info()
#'
#' ## retrieve the raster format name
#' ds$getDriverShortName()
#' ds$getDriverLongName()
#'
#' ## retrieve a list of files composing the dataset
#' ds$getFileList()
#'
#' ## retrieve dataset parameters
#' ds$getRasterXSize()
#' ds$getRasterYSize()
#' ds$getGeoTransform()
#' ds$getProjection()
#' ds$getRasterCount()
#' ds$bbox()
#' ds$res()
#' ds$dim()
#'
#' ## retrieve some band-level parameters
#' ds$getDescription(band = 1)
#' ds$getBlockSize(band = 1)
#' ds$getOverviewCount(band = 1)
#' ds$getDataTypeName(band = 1)
#' # LCP format does not support an intrinsic nodata value so this returns NA:
#' ds$getNoDataValue(band = 1)
#'
#' ## LCP driver reports several dataset- and band-level metadata
#' ## see the format description at https://gdal.org/en/stable/drivers/raster/lcp.html
#' ## set band = 0 to retrieve dataset-level metadata
#' ## set domain = "" (empty string) for the default metadata domain
#' ds$getMetadata(band = 0, domain = "")
#'
#' ## retrieve metadata for a band as a vector of name=value pairs
#' ds$getMetadata(band = 4, domain = "")
#'
#' ## retrieve the value of a specific metadata item
#' ds$getMetadataItem(band = 2, mdi_name = "SLOPE_UNIT_NAME", domain = "")
#'
#' ## read one row of pixel values from band 1 (elevation)
#' ## raster row/column index are 0-based
#' ## the upper left corner is the origin
#' ## read the tenth row:
#' ncols <- ds$getRasterXSize()
#' rowdata <- ds$read(band = 1, xoff = 0, yoff = 9,
#'                    xsize = ncols, ysize = 1,
#'                    out_xsize = ncols, out_ysize = 1)
#' head(rowdata)
#'
#' ds$close()
#'
#' ## create a new raster using lcp_file as a template
#' new_file <- file.path(tempdir(), "storml_newdata.tif")
#' rasterFromRaster(srcfile = lcp_file,
#'                  dstfile = new_file,
#'                  nbands = 1,
#'                  dtName = "Byte",
#'                  init = -9999)
#'
#' ds_new <- new(GDALRaster, new_file, read_only = FALSE)
#'
#' ## write random values to all pixels
#' set.seed(42)
#' ncols <- ds_new$getRasterXSize()
#' nrows <- ds_new$getRasterYSize()
#' for (row in 0:(nrows - 1)) {
#'     rowdata <- round(runif(ncols, 0, 100))
#'     ds_new$write(band = 1,
#'                  xoff = 0,
#'                  yoff = row,
#'                  xsize = ncols,
#'                  ysize = 1,
#'                  rowdata)
#' }
#'
#' ## re-open in read-only mode when done writing
#' ## this will ensure flushing of any pending writes (implicit $close)
#' ds_new$open(read_only = TRUE)
#'
#' ## getStatistics returns min, max, mean, sd, and sets stats in the metadata
#' ds_new$getStatistics(band = 1, approx_ok = FALSE, force = TRUE)
#' ds_new$getMetadataItem(band = 1, "STATISTICS_MEAN", "")
#'
#' ## close the dataset for proper cleanup
#' ds_new$close()
#' \dontshow{deleteDataset(new_file)}
#' \donttest{
#' ## using a GDAL Virtual File System handler '/vsicurl/'
#' ## see: https://gdal.org/en/stable/user/virtual_file_systems.html
#' url <- "/vsicurl/https://raw.githubusercontent.com/"
#' url <- paste0(url, "usdaforestservice/gdalraster/main/sample-data/")
#' url <- paste0(url, "lf_elev_220_mt_hood_utm.tif")
#'
#' set_config_option("GDAL_HTTP_CONNECTTIMEOUT", "20")
#' set_config_option("GDAL_HTTP_TIMEOUT", "20")
#' if (http_enabled() && vsi_stat(url)) {
#'   ds <- new(GDALRaster, url)
#'   plot_raster(ds, legend = TRUE, main = "Mount Hood elevation (m)")
#'   ds$close()
#' }
#' set_config_option("GDAL_HTTP_CONNECTTIMEOUT", "")
#' set_config_option("GDAL_HTTP_TIMEOUT", "")
#' }
NULL

Rcpp::loadModule("mod_GDALRaster", TRUE)
