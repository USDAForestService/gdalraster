# R functions for various processing with gdalraster
# Chris Toney <chris.toney at usda.gov>


#' List of default nodata values by raster data type
#'
#' These values are currently used in `gdalraster` when a nodata value is 
#' needed but has not been specified:
#' \preformatted{
#'     list("Byte"= 255, "UInt16"= 65535, "Int16"= -32767,
#'          "UInt32"= 4294967293, "Int32"= -2147483647, 
#'          "Float32"= -99999.0, 
#'          "Float64"= -99999.0)
#' }
#' @export
DEFAULT_NODATA <- list("Byte"= 255, "UInt16"= 65535, "Int16"= -32767,
						"UInt32"= 4294967293, "Int32"= -2147483647, 
						"Float32"= -99999.0, 
						"Float64"= -99999.0)

#' @noRd
.getGDALformat <- function(file) {
# Only for guessing common output formats
	file <- as.character(file)
	if (endsWith(file, ".img")) {
		return("HFA")
	}
	if (endsWith(file, ".tif")) {
		return("GTiff")
	}
	if (endsWith(file, ".vrt")) {
		return("VRT")
	}
	return(NULL)
}

#' Get a pixel or line offset for a north-up raster
#' @param coord A georeferenced x or y
#' @param origin Raster xmin or ymax
#' @param gt_pixel_size Geotransform pixel x or y size (negative for y)
#' @returns pixel (column) or line (row) offset for coord x or y
#' @noRd
.getOffset <- function(coord, origin, gt_pixel_size) {
	(coord-origin)/gt_pixel_size
}


#' @noRd
.VRT_KERNEL_TEMPLATE <- 
"<KernelFilteredSource>
  <SourceFilename relativeToVRT=\"%d\">%s</SourceFilename><SourceBand>%d</SourceBand>
  <SrcRect xOff=\"%d\" yOff=\"%d\" xSize=\"%d\" ySize=\"%d\"/>
  <DstRect xOff=\"0\" yOff=\"0\" xSize=\"%d\" ySize=\"%d\"/>
  <Kernel normalized=\"%d\">
    <Size>%d</Size>
    <Coefs>%s</Coefs>
  </Kernel>
</KernelFilteredSource>"


#' Create a raster from an existing raster as template
#' 
#' @description
#' `rasterFromRaster()` creates a new raster with spatial reference, 
#' extent and resolution taken from a template raster, without copying data. 
#' Optionally changes the format, number of bands, data type and nodata value, 
#' sets driver-specific dataset creation options, and initializes to a value.
#'
#' @param srcfile Source raster filename.
#' @param dstfile Output raster filename.
#' @param fmt Output raster format name (e.g., "GTiff" or "HFA"). Will attempt 
#' to guess from the output filename if \code{fmt} is not specified.
#' @param nbands Number of output bands.
#' @param dtName Output raster data type name.
#' (e.g., "Byte", "Int16", "UInt16", "Int32" or "Float32").
#' @param options Optional list of format-specific creation options in a
#' vector of "NAME=VALUE" pairs.
#' (e.g., \code{options = c("COMPRESS=LZW")} to set \code{LZW}
#' compression during creation of a GTiff file).
#' @param init Numeric value to initialize all pixels in the output raster.
#' @param dstnodata Numeric nodata value for the output raster.
#' @returns Returns the destination filename invisibly.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [create()], [createCopy()], [rasterToVRT()]
#'
#' @examples
#' ## band 2 in a FARSITE landscape file has slope degrees
#' ## verify this and convert slope degrees to slope percent in a new raster
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds_lcp <- new(GDALRaster, lcp_file, read_only=TRUE)
#' ds_lcp$info()
#' ds_lcp$getMetadata(band=2, domain="")
#' 
#' slpp_file <- paste0(tempdir(), "/", "storml_slpp.tif")
#' options = c("COMPRESS=LZW")
#' rasterFromRaster(srcfile = lcp_file,
#'                  dstfile = slpp_file,
#'                  nbands = 1,
#'                  dtName = "Int16",
#'                  options = options,
#'                  init = -32767)
#' ds_slp <- new(GDALRaster, slpp_file, read_only=FALSE)
#' 
#' ## slpp_file is initialized to -32767 and nodata value set
#' ds_slp$getNoDataValue(band=1)
#' 
#' ## extent and cell size are the same as lcp_file
#' ds_lcp$bbox()
#' ds_lcp$res()
#' ds_slp$bbox()
#' ds_slp$res()
#' 
#' ## convert slope degrees in lcp_file band 2 to slope percent in slpp_file
#' ## bring through LCP nodata -9999 to the output nodata value
#' ncols <- ds_slp$getRasterXSize()
#' nrows <- ds_slp$getRasterYSize()
#' for (row in 0:(nrows-1)) {
#'     rowdata <- ds_lcp$read(band=2, 
#'                            xoff=0, yoff=row,
#'                            xsize=ncols, ysize=1,
#'                            out_xsize=ncols, out_ysize=1)
#'     rowslpp <- tan(rowdata*pi/180) * 100
#'     rowslpp[rowdata==-9999] <- -32767
#'     dim(rowslpp) <- c(1, ncols)
#'     ds_slp$write(band=1, xoff=0, yoff=row, xsize=ncols, ysize=1, rowslpp)
#' }
#' 
#' ## min, max, mean, sd
#' ds_slp$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#' 
#' ds_slp$close()
#' ds_lcp$close()
#' @export
rasterFromRaster <- function(srcfile, dstfile, fmt=NULL, nbands=NULL,
						dtName=NULL, options=NULL, init=NULL, dstnodata=init) {
	
	if (is.null(fmt)) {
		fmt <- .getGDALformat(dstfile)
		if (is.null(fmt)) {
			stop("Use fmt argument to specify a GDAL raster format name.",
				call. = FALSE)
		}
	}
	
	src_ds <- new(GDALRaster, srcfile, read.only=TRUE)
	if (is.null(nbands)) {
		nbands <- src_ds$getRasterCount()
	}
	nrows <- src_ds$getRasterYSize()
	ncols <- src_ds$getRasterXSize()
	gt <- src_ds$getGeoTransform()
	if (is.null(dtName))
		dtName <- src_ds$getDataTypeName(1)
	srs <- src_ds$getProjectionRef()
	src_ds$close()

	create(fmt, dstfile, ncols, nrows, nbands, dtName, options)
	dst_ds <- new(GDALRaster, dstfile, FALSE)
	dst_ds$setGeoTransform(gt)
	dst_ds$setProjection(srs)
	if(!is.null(dstnodata))
		for (b in 1:nbands)
			dst_ds$setNoDataValue(b, dstnodata)

	if (!is.null(init)) {
		message("Initializing destination raster...")
		for (b in 1:nbands)
			dst_ds$fillRaster(b, init, 0)
		message("Done.")
	}
	
	dst_ds$close()
	return(invisible(dstfile))
}


#' Create a GDAL virtual raster
#'
#' @description
#' `rasterToVRT()` creates a virtual raster dataset (VRT format) derived from 
#' a source raster with options for virtual subsetting, virtually resampling 
#' the source data at a different pixel resolution, or applying a virtual 
#' kernel filter.
#'
#' @details
#' `rasterToVRT()` has similarities to the command-line utility `gdalbuildvrt` 
#' (\url{https://gdal.org/programs/gdalbuildvrt.html}) but is not a wrapper for 
#' it and does not build mosaics. `rasterToVRT()` is somewhat tailored for 
#' clipping and pixel-aligning various raster data in relation to vector 
#' polygon boundaries. It also supports VRT kernel filtering.
#'
#' A VRT dataset is saved as a plain-text file with extension .vrt. This file 
#' contains a description of the dataset in an XML format. The description 
#' includes the source raster filename which can be a full path 
#' (`relativeToVRT = FALSE`) or relative path (`relativeToVRT = TRUE`). 
#' For relative path, `rasterToVRT()` assumes that the .vrt file will be in 
#' the same directory as the source file and uses `basename(srcfile)`. The 
#' elements of the XML schema describe how the source data will be read, along 
#' with algorithms potentially applied and so forth. Documentation of the XML 
#' format for .vrt is at: 
#' \url{https://gdal.org/drivers/raster/vrt.html}.
#' 
#' Since .vrt is a small plain-text file it is fast to write and requires 
#' little storage space. Read performance is not degraded for certain simple 
#' operations (e.g., virtual clip without resampling). Reading will be 
#' slower for virtual resampling to a different pixel resolution or virtual 
#' kernel filtering since the operations are performed on-the-fly (but .vrt 
#' does not require the up front writing of a resampled or kernel-filtered 
#' raster to a regular format). VRT is sometimes useful as an intermediate 
#' raster in a series of processing steps, e.g., as a `tempfile` (the 
#' default).
#' 
#' GDAL VRT format has several capabilities and uses beyond those
#' covered by `rasterToVRT()`. See the URLs above for a full discussion.
#'
#' @note
#' Pixel alignment is specified in terms of the source raster pixels (i.e., 
#' `srcfile` of the virtual raster). The use case in mind is virtually 
#' clipping a raster to the bounding box of a vector polygon and keeping 
#' pixels aligned with `srcfile` (`src_align = TRUE`). `src_align` would be 
#' set to `FALSE` if the intent is "target alignment". For example, if  
#' `subwindow` is the bounding box of another raster with a different layout, 
#' then also setting `resolution` to the pixel resolution of the target raster 
#' and `src_align = FALSE` will result in a virtual raster pixel-aligned with 
#' the target (i.e., pixels in the virtual raster are no longer aligned with 
#' its `srcfile`). Resampling defaults to `nearest` if not specified. 
#' Examples for both cases of `src_align` are given below.
#' 
#' `rasterToVRT()` assumes `srcfile` is a north-up raster.
#' Requires package `xml2`.
#'
#' @param srcfile Source raster filename.
#' @param relativeToVRT Logical. Indicates whether the source filename should 
#' be interpreted as relative to the .vrt file (`TRUE`) or not relative
#' to the .vrt file (`FALSE`, the default). If `TRUE`, the .vrt 
#' file is assumed to be in the same directory as `srcfile` and
#' `basename(srcfile)` is used in the .vrt file. Use `TRUE` if the .vrt file 
#' will always be stored in the same directory with `srcfile`.
#' @param vrtfile Output VRT filename.
#' @param resolution A numeric vector of length two (xres, yres). The pixel
#' size must be expressed in georeferenced units. Both must be positive values.
#' The source pixel size is used if `resolution` is not specified.
#' @param subwindow A numeric vector of length four (xmin, ymin, xmax, ymax).
#' Selects `subwindow` of the source raster with corners given in 
#' georeferenced coordinates (in the source CRS).
#' If not given, the upper left corner of the VRT will be the 
#' same as source, and the VRT extent will be the same or larger than source 
#' depending on `resolution`.
#' @param src_align Logical.
#'   * `TRUE`: the upper left corner of the VRT extent will be set to the 
#'   upper left corner of the source pixel that contains `subwindow` xmin, ymax. 
#'   The VRT will be pixel-aligned with source if the VRT `resolution` is the 
#'   same as the source pixel size, otherwise VRT extent will be the minimum 
#'   rectangle that contains `subwindow` for the given pixel size. Use 
#'   Usually `src_align=TRUE` when selecting a raster minimum bounding box 
#'   for a vector polygon.
#'   * `FALSE`: the VRT upper left corner will be exactly `subwindow` 
#'   xmin, ymax, and the VRT extent will be the minimum rectangle that contains 
#'   `subwindow` for the given pixel size. If `subwindow` is not given, the 
#'   source raster extent is used in which case `src_align=FALSE` has no effect.
#'   Use `src_align=FALSE` to pixel-align two rasters of different sizes, i.e.,
#'   when the intent is target alignment.
#' @param resampling The resampling method to use if xsize, ysize of the VRT is
#' different than the size of the underlying source rectangle (in number of
#' pixels). The values allowed are nearest, bilinear, cubic, cubicspline, 
#' lanczos, average and mode (as character).
#' @param krnl A filtering kernel specified as pixel coefficients.
#' `krnl` is a array with dimensions (size, size), where
#' size must be an odd number. `krnl` can also be given as a vector with
#' length size x size. For example, a 3x3 average filter is given by:
#' \preformatted{
#' krnl <- c(
#' 0.11111, 0.11111, 0.11111,
#' 0.11111, 0.11111, 0.11111,
#' 0.11111, 0.11111, 0.11111)
#' }
#' A kernel cannot be applied to sub-sampled or over-sampled data.
#' @param normalized Logical. Indicates whether the kernel is normalized.
#' Defaults to `TRUE`.
#' @returns Returns the VRT filename invisibly.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [bbox_from_wkt()]
#'
#' [warp()] can write VRT for virtual reprojection
#'
#' @examples
#' ### resample
#'
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' ds <- new(GDALRaster, evt_file, TRUE)
#' ds$res()
#' ds$bbox()
#' ds$close()
#' 
#' ## use combine() with one input to get a table of pixel counts for  
#' ## the raster value
#' vat <- combine(evt_file)
#' print(vat[-1]) # drop the cmbid in this case
#' sum(vat$count)
#' 
#' ## resample at 90-m resolution
#' ## EVT is thematic vegetation type so use a majority value
#' vrt_file <- rasterToVRT(evt_file,
#'                         resolution=c(90,90),
#'                         resampling="mode")
#' 
#' ## .vrt is a small xml file pointing to the source raster
#' file.size(vrt_file)
#' 
#' vat90m <- combine(vrt_file, var.names=c("evt90m"))
#' print(vat90m[-1])
#' sum(vat90m$count)
#' 
#' ds <- new(GDALRaster, vrt_file, TRUE)
#' ds$res()
#' ds$bbox()
#' ds$close()
#'
#'
#' ### clip
#' 
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' ds_evt <- new(GDALRaster, evt_file, TRUE)
#' ds_evt$bbox()
#' 
#' ## WKT string for a boundary within the EVT extent
#' bnd = "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2 
#' 5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5, 
#' 325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
#' 
#' ## src_align = TRUE
#' vrt_file <- rasterToVRT(evt_file,
#'                         subwindow = bbox_from_wkt(bnd),
#'                         src_align=TRUE)
#' ds_vrt <- new(GDALRaster, vrt_file, TRUE)
#' 
#' ## VRT is a virtual clip, pixel-aligned with the EVT raster
#' bbox_from_wkt(bnd)
#' ds_vrt$bbox()
#' ds_vrt$res()
#' 
#' ## src_align = FALSE
#' vrt_file <- rasterToVRT(evt_file,
#'                         subwindow = bbox_from_wkt(bnd),
#'                         src_align=FALSE)
#' ds_vrt_noalign <- new(GDALRaster, vrt_file, TRUE)
#' 
#' ## VRT upper left corner (xmin, ymax) is exactly bnd xmin, ymax
#' ds_vrt_noalign$bbox()
#' ds_vrt_noalign$res()
#' 
#' ds_vrt$close()
#' ds_vrt_noalign$close()
#' ds_evt$close()
#'
#'
#' ### subset and pixel align two rasters
#' 
#' ## FARSITE landscape file for the Storm Lake area
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds_lcp <- new(GDALRaster, lcp_file, read_only=TRUE)
#' 
#' ## Landsat band 5 file covering the Storm Lake area
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#' ds_b5 <- new(GDALRaster, b5_file, read_only=TRUE)
#' 
#' ds_lcp$bbox()  # 323476.1 5101872.0  327766.1 5105082.0
#' ds_lcp$res()   # 30 30
#' 
#' ds_b5$bbox()   # 323400.9 5101815.8  327870.9 5105175.8
#' ds_b5$res()    # 30 30
#' 
#' ## src_align = FALSE because we need target alignment in this case:
#' vrt_file <- rasterToVRT(b5_file,
#'                         resolution = ds_lcp$res(),
#'                         subwindow = ds_lcp$bbox(),
#'                         src_align = FALSE)
#' ds_b5vrt <- new(GDALRaster, vrt_file, TRUE)
#'
#' ds_b5vrt$bbox() # 323476.1 5101872.0  327766.1 5105082.0
#' ds_b5vrt$res()  # 30 30
#' 
#' ## read the the Landsat file pixel-aligned with the LCP file 
#' ## summarize band 5 reflectance where FBFM = 165 
#' ## LCP band 4 contains FBFM (a classification of fuel beds):
#' ds_lcp$getMetadata(band=4, domain="")
#' 
#' ## verify Landsat nodata (0):
#' ds_b5vrt$getNoDataValue(band=1)
#' ## will be read as NA and omitted from stats
#' rs <- new(RunningStats, na_rm=TRUE)
#' 
#' ncols <- ds_lcp$getRasterXSize()
#' nrows <- ds_lcp$getRasterYSize()
#' for (row in 0:(nrows-1)) {
#'     row_fbfm <- ds_lcp$read(band=4, xoff=0, yoff=row,
#'                             xsize=ncols, ysize=1,
#'                             out_xsize=ncols, out_ysize=1)
#'     row_b5 <- ds_b5vrt$read(band=1, xoff=0, yoff=row,
#'                             xsize=ncols, ysize=1,
#'                             out_xsize=ncols, out_ysize=1)
#' 	   rs$update(row_b5[row_fbfm == 165])
#' }
#' rs$get_count()
#' rs$get_mean()
#' rs$get_min()
#' rs$get_max()
#' rs$get_sum()
#' rs$get_var()
#' rs$get_sd()
#' 
#' ds_b5vrt$close()
#' ds_lcp$close()
#' ds_b5$close()
#' @export
rasterToVRT <- function(srcfile, relativeToVRT = FALSE, 
				vrtfile = tempfile("tmprast", fileext=".vrt"), 
				resolution = NULL, 
				subwindow = NULL, 
				src_align = TRUE, 
				resampling = "nearest",
				krnl = NULL,
				normalized = TRUE) {

	if (!requireNamespace("xml2", quietly = TRUE))
		stop("rasterToVRT() requires package \"xml2.\"", call. = FALSE)

	if (relativeToVRT) relativeToVRT = 1 else relativeToVRT = 0
	if (normalized) normalized = 1 else normalized = 0

	src_ds <- new(GDALRaster, srcfile, read.only=TRUE)
	src_gt <- src_ds$getGeoTransform()
	src_bbox <- src_ds$bbox()
	src_xmin <- src_bbox[1]
	src_ymin <- src_bbox[2]
	src_xmax <- src_bbox[3]
	src_ymax <- src_bbox[4]
	src_xres <- src_gt[2]
	src_yres <- src_gt[6]
	src_bands <- src_ds$getRasterCount()
	src_ds$close()
	
	if (!is.null(resolution) && !is.null(krnl)) {
		if (src_xres != resolution[1] || src_yres != resolution[2])
			stop("Cannot apply a kernel to sub-sampled or over-sampled data.",
					call. = FALSE)
	}
	
	if (!is.null(krnl)) {
		if ((length(krnl) %% 2) == 0)
			stop("Kernel size must be an odd number.", call. = FALSE)
	}
	
	tmp_vrtfile <- tempfile("src", fileext=".vrt")
	createCopy("VRT", tmp_vrtfile, srcfile)
	
	if (is.null(resolution)) {
		vrt_xres <- src_xres
		vrt_yres <- src_yres
	}
	else {
		vrt_xres <- resolution[1]
		vrt_yres <- -resolution[2]
	}

	# src offsets
	if (is.null(subwindow)) {
		subwindow <- c(src_xmin,src_ymin,src_xmax,src_ymax)
	}
	else {
		if ( !.g_within(bbox_to_wkt(subwindow), bbox_to_wkt(src_bbox)) )
			stop("Subwindow is not completely within source raster extent.",
					call. = FALSE)
	}
	src_xoff <- floor(.getOffset(subwindow[1], src_xmin, src_xres))
	src_yoff <- floor(.getOffset(subwindow[4], src_ymax, src_yres))
	
	#get vrt geotransform and size (assuming a north-up raster)
	if (src_align) {
		#lay out the vrt raster so it is aligned with ul corner
		#of ul src pixel
		vrt_xmin <- src_xmin + src_xoff * src_xres
		vrt_ymax <- src_ymax + src_yoff * src_yres
	}
	else {
		#lay out the vrt raster with origin at the subwindow origin
		vrt_xmin <- subwindow[1]
		vrt_ymax <- subwindow[4]
	}
	vrt_ncols <- ceiling(.getOffset(subwindow[3], vrt_xmin, vrt_xres))
	vrt_xmax <- (vrt_xmin + vrt_ncols * vrt_xres)
	vrt_nrows <- ceiling(.getOffset(subwindow[2], vrt_ymax, vrt_yres))
	vrt_ymin <- (vrt_ymax + vrt_nrows * vrt_yres)
	vrt_gt <- c(vrt_xmin, vrt_xres, 0, vrt_ymax, 0, vrt_yres)
	srcwin_xsize <- ceiling(.getOffset(vrt_xmax, vrt_xmin, src_xres))
	srcwin_ysize <- ceiling(.getOffset(vrt_ymin, vrt_ymax, src_yres))

	x <- xml2::read_xml(tmp_vrtfile)
	
	xds <- xml2::xml_find_first(x, "/VRTDataset")
	xml2::xml_attrs(xds) <- c("rasterXSize" = as.character(vrt_ncols),
							"rasterYSize" = as.character(vrt_nrows))
	
	xgt <- xml2::xml_find_first(x, "/VRTDataset/GeoTransform")
	xml2::xml_text(xgt) <- paste(vrt_gt, collapse=", ")
	
	xpath <- "/VRTDataset/VRTRasterBand/SimpleSource/SrcRect"
	xsrcrect <- xml2::xml_find_all(x, xpath)
	xml2::xml_attrs(xsrcrect) <- c("xOff" = as.character(src_xoff),
								"yOff" = as.character(src_yoff),
								"xSize" = as.character(srcwin_xsize),
								"ySize" = as.character(srcwin_ysize))
	
	xpath <- "/VRTDataset/VRTRasterBand/SimpleSource/DstRect"
	xdstrect <- xml2::xml_find_all(x, xpath)
	xml2::xml_attrs(xdstrect) <- c("xOff" = "0",
								"yOff" = "0",
								"xSize" = as.character(vrt_ncols),
								"ySize" = as.character(vrt_nrows))
	
	if (vrt_ncols != srcwin_xsize || vrt_nrows != srcwin_ysize) {
		xpath <- "/VRTDataset/VRTRasterBand/SimpleSource"
		xssrc <- xml2::xml_find_all(x, xpath)
		xml2::xml_attr(xssrc, "resampling") <- resampling
	}
	
	xpath <- "/VRTDataset/VRTRasterBand/SimpleSource/SourceFilename"
	xfn <- xml2::xml_find_all(x, xpath)
	if (relativeToVRT) {
		#this assumes the VRT file will be in the same directory as source file
		xml2::xml_attr(xfn, "relativeToVRT") <- "1"
		xml2::xml_text(xfn) <- basename(srcfile)
	}
	else {
		xml2::xml_attr(xfn, "relativeToVRT") <- "0"
		xml2::xml_text(xfn) <- srcfile
	}
	
	# clear statistics metadata if present
	xsrcstats <- xml2::xml_find_all(x, "//MDI[starts-with(@key, 'STATISTICS_')]")
	if (length(xsrcstats) != 0) {
		xml2::xml_text(xsrcstats) <- rep("", length(xsrcstats))
	}
	
	xml2::write_xml(x, vrtfile, options=c("format", "no_declaration"))
	
	if (!is.null(krnl)) {
		vrt_ds <- new(GDALRaster, vrtfile, read_only=FALSE)
		if (relativeToVRT) srcfile <- basename(srcfile)
		for (band in 1:src_bands) {
			krnl_xml <- enc2utf8( sprintf(
				.VRT_KERNEL_TEMPLATE,
				relativeToVRT,
				srcfile,
				band,
				src_xoff,
				src_yoff,
				srcwin_xsize,
				srcwin_ysize,
				vrt_ncols,
				vrt_nrows,
				normalized,
				sqrt(length(krnl)),
				paste(krnl, collapse=" ")) )
				
			vrt_ds$.setMetadataItem(band, "source_0", krnl_xml, "vrt_sources")
		}
		vrt_ds$close()
	}

	return(invisible(vrtfile))
}


#' Raster calculation
#' 
#' @description
#' `calc()` evaluates an R expression for each pixel in a raster layer or 
#' stack of layers. Each layer is defined by a raster filename, band number, 
#' and a variable name to use in the R expression. If not specified, band 
#' defaults to 1 for each input raster. 
#' Variable names default to `LETTERS` if not specified 
#' (`A` (layer 1), `B` (layer 2), ...).
#' All of the input layers must have the same extent and cell size.
#' The projection will be read from the first raster in the list 
#' of inputs.
#' Individual pixel coordinates are also available as variables in the 
#' R expression, as either x/y in the raster projected coordinate system or 
#' inverse projected longitude/latitude.
#'
#' @details
#' The variables in `expr` are vectors of length raster Xsize 
#' (rows of a raster layer). 
#' The expression should return a vector also of length raster Xsize 
#' (an output row). 
#' Two special variable names are available in `expr` by default: 
#' `pixelX` and `pixelY` provide the pixel center coordinate in 
#' projection units. If `usePixelLonLat = TRUE`, the pixel x/y coordinates 
#' will also be inverse projected to longitude/latitude and available 
#' in `expr` as `pixelLon` and `pixelLat` (in the same geographic 
#' coordinate system used by the input projection, which is read from the 
#' first input raster).
#'
#' To refer to specific bands in a multi-band file, repeat the filename in 
#' `rasterfiles` and specify corresponding band numbers in `bands`, along with
#' optional variable names in `var.names`, for example,
#' \preformatted{
#' rasterfiles = c("multiband.tif", "multiband.tif")
#' bands = c(4, 5)
#' var.names = c("B4", "B5")
#' }
#'
#' Output will be written to `dstfile`. To update a file that already 
#' exists, set `write_mode = "update"` and set `out_band` to an existing 
#' band number in `dstfile` (new bands cannot be created in `dstfile`).
#' 
#' @param expr An R expression as a character string (e.g., `"A + B"`).
#' @param rasterfiles Character vector of source raster filenames.
#' @param bands Integer vector of band numbers to use for each raster layer.
#' @param var.names Character vector of variable names to use for each raster 
#' layer.
#' @param dstfile Character filename of output raster.
#' @param fmt Output raster format name (e.g., "GTiff" or "HFA"). Will attempt 
#' to guess from the output filename if not specified.
#' @param dtName Character name of output data type (e.g., Byte, Int16, 
#' UInt16, Int32, UInt32, Float32).
#' @param out_band Integer band number in `dstfile` for writing output.
#' @param options Optional list of format-specific creation options in a
#' vector of "NAME=VALUE" pairs
#' (e.g., \code{options = c("COMPRESS=LZW")} to set \code{LZW}
#' compression during creation of a GTiff file).
#' @param nodata_value Numeric value to assign if `expr` returns NA.
#' @param setRasterNodataValue Logical. `TRUE` will attempt to set the raster 
#' format nodata value to `nodata_value`, or `FALSE` not to set a raster 
#' nodata value.
#' @param usePixelLonLat Logical. If `TRUE`, `pixelX` and `pixelY` will be 
#' inverse projected to geographic coordinates and available as `pixelLon` and 
#' `pixelLat` in `expr` (adds computation time).
#' @param write_mode Character. Name of the file write mode for output. 
#' One of:
#'   * `safe` - execution stops if `dstfile` already exists (no output written)
#'   * `overwrite` - if `dstfile` exists if will be overwritten with a new file
#'   * `update` - if `dstfile` exists, will attempt to open in update mode 
#'   and write output to `out_band`
#' @returns Returns the output filename invisibly.
#'
#' @seealso
#' [`GDALRaster-class`][GDALRaster], [combine()], [rasterToVRT()]
#'
#' @examples
#' ### Using pixel longitude/latitude
#'
#' ## Hopkins bioclimatic index (HI) as described in:
#' ## Bechtold, 2004, West. J. Appl. For. 19(4):245-251.
#' ## Integrates elevation, latitude and longitude into an index of the 
#' ## phenological occurrence of springtime. Here it is relativized to 
#' ## mean values for an eight-state region in the western US.
#' ## Positive HI means spring is delayed by that number of days relative 
#' ## to the reference position, while negative values indicate spring is
#' ## advanced. The original equation had elevation units as feet, so 
#' ## converting m to ft in `expr`.
#' 
#' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#'
#' ## expression to calculate HI
#' expr <- "round( ((ELEV_M * 3.281 - 5449) / 100) + 
#'                 ((pixelLat - 42.16) * 4) + 
#'                 ((-116.39 - pixelLon) * 1.25) )"
#' 
#' ## calc() writes to a tempfile by default
#' hi_file <- calc(expr = expr, 
#'                 rasterfiles = elev_file, 
#'                 var.names = c("ELEV_M"), 
#'                 dtName = "Int16",
#'                 nodata_value = -32767, 
#'                 setRasterNodataValue = TRUE,
#'                 usePixelLonLat = TRUE)
#' 
#' ds <- new(GDALRaster, hi_file, read_only=TRUE)
#' ## min, max, mean, sd
#' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#' ds$close()
#'
#'
#' ### Calculate normalized difference vegetation index (NDVI)
#' 
#' ## Landast band 4 (red) and band 5 (near infrared):
#' b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
#' b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
#'
#' ## is nodata value set
#' ds <- new(GDALRaster, b4_file, read_only=TRUE)
#' ds$getNoDataValue(band=1)   # 0
#' ds$close()
#' ds <- new(GDALRaster, b5_file, read_only=TRUE)
#' ds$getNoDataValue(band=1)   # 0
#' ds$close()
#'
#' ## 0 will be read as NA so don't need to handle zeros in expr
#' expr <- "(B5-B4)/(B5+B4)"
#' ndvi_file <- calc(expr = expr,
#'                   rasterfiles = c(b4_file, b5_file),
#'                   var.names = c("B4", "B5"),
#'                   dtName = "Float32",
#'                   nodata_value = -32767,
#'                   setRasterNodataValue = TRUE)
#' 
#' ds <- new(GDALRaster, ndvi_file, read_only=TRUE)
#' ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
#' ds$close()
#'
#'
#' ### Reclassify a variable by rule set
#' 
#' ## Combine two raster layers and look for specific combinations. Then 
#' ## recode to a new value by rule set.
#' ##
#' ## Based on example in:
#' ##   Stratton, R.D. 2009. Guidebook on LANDFIRE fuels data acquisition, 
#' ##   critique, modification, maintenance, and model calibration.
#' ##   Gen. Tech. Rep. RMRS-GTR-220. U.S. Department of Agriculture, 
#' ##   Forest Service, Rocky Mountain Research Station. 54 p.
#' ## Context: Refine national-scale fuels data to improve fire simulation
#' ##   results in localized applications.
#' ## Issue: Areas with steep slopes (40+ degrees) were mapped as
#' ##   GR1 (101; short, sparse dry climate grass) and 
#' ##   GR2 (102; low load, dry climate grass) but were not carrying fire.
#' ## Resolution: After viewing these areas in Google Earth,
#' ##   NB9 (99; bare ground) was selected as the replacement fuel model.
#' 
#' ## look for combinations of slope >= 40 and FBFM 101 or 102
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' rasterfiles <- c(lcp_file, lcp_file)
#' var.names <- c("SLP", "FBFM")
#' bands <- c(2, 4)
#' df <- combine(rasterfiles, var.names, bands)
#' df_subset <- subset(df, SLP >= 40 & FBFM %in% c(101,102))
#' print(df_subset)       # twelve combinations meet the criteria
#' sum(df_subset$count)   # 85 total pixels
#' 
#' ## recode these pixels to 99 (bare ground)
#' ## the LCP driver does not support in-place write so make a copy as GTiff
#' tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
#' createCopy("GTiff", tif_file, lcp_file)
#' 
#' expr <- "ifelse( SLP >= 40 & FBFM %in% c(101,102), 99, FBFM)"
#' calc(expr = expr,
#'      rasterfiles = c(lcp_file, lcp_file),
#'      bands = c(2, 4),
#'      var.names = c("SLP", "FBFM"),
#'      dstfile = tif_file,
#'      out_band = 4,
#'      write_mode = "update")
#' 
#' ## verify the ouput
#' rasterfiles <- c(tif_file, tif_file)
#' var.names <- c("SLP", "FBFM")
#' bands <- c(2, 4)
#' df <- combine(rasterfiles, var.names, bands)
#' df_subset <- subset(df, SLP >= 40 & FBFM %in% c(101,102))
#' print(df_subset)
#' sum(df_subset$count)
#' 
#' ## if LCP file format is needed: createCopy(tif_file, <new_lcp_file>)
#' @export
calc <- function(expr, 
					rasterfiles, 
					bands = NULL, 
					var.names = NULL,
					dstfile = tempfile("rastcalc", fileext=".tif"),
					fmt = NULL,
					dtName = "Int16", 
					out_band = NULL,
					options = NULL,
					nodata_value = NULL, 
					setRasterNodataValue = FALSE,
					usePixelLonLat = FALSE,
					write_mode = "safe")
{
	
	calc_expr <- parse(text=expr)

	if ( !all(file.exists(rasterfiles)) ) {
		message( rasterfiles[which(!file.exists(rasterfiles))] )
		stop("File not found.", call. = FALSE)
	}
	
	if (write_mode == "safe" && file.exists(dstfile))
		stop("The output file already exists and write_mode is 'safe'.", 
			call. = FALSE)
	
	if (write_mode == "update" && is.null(out_band))
		stop("out_band must be specified for update mode.", call. = FALSE)

	if (write_mode == "update") {
		update_mode = TRUE
	}
	else if (write_mode == "safe" || write_mode == "overwrite") {
		if (!is.null(out_band))
			if (out_band != 1)
				stop("out_band other than 1 requires 'update' mode.",
					call. = FALSE)
		update_mode <- FALSE
		out_band <- 1
	}
	else {
		stop("Unknown write_mode.", call. = FALSE)
	}

	nrasters <- length(rasterfiles)

	if (!is.null(bands)) {
		if (length(bands) != nrasters) {
			stop("List of band numbers must be same length as raster list.",
				call. = FALSE)
		}
	}
	else {
		bands <- rep(1, nrasters)
	}
	
	if (!is.null(var.names)) {
		if (length(var.names) != nrasters) {
			stop("List of variable names must be same length as raster list.",
				call. = FALSE)
		}
	}
	else {
		var.names <- LETTERS[1:nrasters]
	}
	
	if (is.null(fmt)) {
		fmt <- .getGDALformat(dstfile)
		if (is.null(fmt)) {
			stop("Use fmt argument to specify a GDAL raster format code.",
				call. = FALSE)
		}
	}
	
	if (is.null(nodata_value)) {
		nodata_value <- DEFAULT_NODATA[[dtName]]
		if (is.null(nodata_value)) {
			stop("Default nodata value unavailable for output data type.",
				call. = FALSE)
		}
	}
	
	# use first raster as reference
	ref <- new(GDALRaster, rasterfiles[1], TRUE)
	nrows <- ref$getRasterYSize()
	ncols <- ref$getRasterXSize()
	cellsizeX <- ref$res()[1]
	cellsizeY <- ref$res()[2]
	xmin <- ref$bbox()[1]
	ymax <- ref$bbox()[4]
	srs <- ref$getProjectionRef()
	ref$close()
	
	if(nrasters > 1) {
		for(r in rasterfiles) {
			ds <- new(GDALRaster, r, TRUE)
			if(ds$getRasterYSize() != nrows || ds$getRasterXSize() != ncols) {
				message(rasterfiles[r])
				stop("All input rasters must have the same dimensions.", 
					call. = FALSE)
			}
			ds$close()
		}
	}
	
	if (update_mode) {
		# write to an existing raster
		dst_ds <- new(GDALRaster, dstfile, read_only=FALSE)
	}
	else {
		#create the output raster
		dstnodata <- NULL
		if(setRasterNodataValue) 
			dstnodata <- nodata_value
		rasterFromRaster(rasterfiles[1], 
							dstfile,
							fmt, 
							nbands = 1, 
							dtName = dtName,
							options = options, 
							dstnodata = dstnodata)
		dst_ds <- new(GDALRaster, dstfile, read_only=FALSE)
	}
	
	
	# list of GDALRaster objects for each raster layer
	ds_list <- list()
	for (r in 1:nrasters)
		ds_list[[r]] <- new(GDALRaster, rasterfiles[r], read_only=TRUE)
	
	x <- seq(from = xmin + (cellsizeX/2), by = cellsizeX, length.out = ncols)
	assign("pixelX", x)
	
	process_row <- function(row) {
		y <- rep( (ymax - (cellsizeY/2) - (cellsizeY*row)), ncols )
		assign("pixelY", y)
		
		if(usePixelLonLat) {
			lonlat <- inv_project(cbind(x,y), srs)
			assign("pixelLon", lonlat[,1])
			assign("pixelLat", lonlat[,2])
		}
		
		for (r in 1:nrasters) {
			inrow <- ds_list[[r]]$read(band = bands[r], 
										xoff = 0,
										yoff = row,
										xsize = ncols,
										ysize = 1,
										out_xsize = ncols,
										out_ysize = 1)
			assign(var.names[r], inrow)
		}
		
		outrow <- eval(calc_expr)
		if (length(outrow) != ncols) {
			dst_ds$close()
			for (r in 1:nrasters)
				ds_list[[r]]$close()
			stop("Result vector is the wrong size.", call. = FALSE)
		}
		outrow <- ifelse(is.na(outrow), nodata_value, outrow)
		dim(outrow) <- c(1, ncols)
		dst_ds$write(band = out_band,
					offx = 0,
					offy = row,
					xsize = ncols,
					ysize = 1,
					outrow)
					
		setTxtProgressBar(pb, row+1)
		return()
	}
	
	message(paste("Calculating from", nrasters, "input layer(s)..."))
	pb <- txtProgressBar(min=0, max=nrows)
	lapply(0:(nrows-1), process_row)
	close(pb)

	message(paste("Output written to:", dstfile))
	dst_ds$close()
	for (r in 1:nrasters)
		ds_list[[r]]$close()
		
	invisible(dstfile)
}


#' Raster overlay for unique combinations
#' 
#' @description
#' `combine()` overlays multiple rasters so that a unique ID is assigned to 
#' each unique combination of input values. The input raster layers  
#' typically have integer data types (floating point will be coerced to 
#' integer by truncation), and must have the same projection, extent and cell 
#' size. Pixel counts for each unique combination are obtained, and 
#' combination IDs are optionally written to an output raster.
#'
#' @details
#' To specify input raster layers that are bands of a multi-band 
#' raster file, repeat the filename in `rasterfiles` and provide the 
#' corresponding band numbers in `bands`. For example:
#' \preformatted{
#' rasterfiles <- c("multi-band.tif", "multi-band.tif", "other.tif")
#' bands <- c(4, 5, 1)
#' var.names <- c("multi_b4", "multi_b5", "other")
#' }
#'
#' [rasterToVRT()] provides options for virtual clipping, resampling and pixel 
#' alignment, which may be helpful here if the input rasters are not already 
#' aligned on a common extent and cell size.
#'
#' If an output raster of combination IDs is written, the user should verify 
#' that the number of combinations obtained did not exceed the range of the 
#' output data type. Combination IDs are sequential integers starting at 1.
#' Typical output data types are the unsigned types: 
#' Byte (0 to 255), UInt16 (0 to 65,535) and UInt32 (the default, 0 to 
#' 4,294,967,295).
#' 
#' `combine()` can also run on a single raster layer to obtain a table of 
#' pixel values and their counts.
#'
#' @param rasterfiles Character vector of raster filenames to combine.
#' @param var.names Character vector of `length(rasterfiles)` containing 
#' variable names for each raster layer. Defaults will be assigned if 
#' `var.names` are omitted.
#' @param bands Numeric vector of `length(rasterfiles)` containing the band 
#' number to use for each raster in `rasterfiles`. Band 1 will be used for 
#' each input raster if `bands` are not specified.
#' @param dstfile Character. Optional output raster filename for writing the 
#' per-pixel combination IDs. The output raster will be created (and 
#' overwritten if it already exists).
#' @param fmt Character. Output raster format name (e.g., "GTiff" or "HFA").
#' @param dtName Character. Output raster data type name. Combination IDs are 
#' sequential integers starting at 1. The data type for the output raster 
#' should be large enough to accommodate the potential number of unique 
#' combinations of the input values 
#' (e.g., "UInt16" or the default "UInt32").
#' @param options Optional list of format-specific creation options in a
#' vector of "NAME=VALUE" pairs.
#' (e.g., \code{options = c("COMPRESS=LZW")} to set \code{LZW}
#' compression during creation of a GTiff file).
#' @returns A data frame with column `cmbid` containing the combination IDs, 
#' column `count` containing the pixel counts for each combination, 
#' and `length(rasterfiles)` columns named `var.names` containing the integer 
#' values comprising each unique combination.
#'
#' @seealso
#' [`CmbTable-class`][CmbTable], [`GDALRaster-class`][GDALRaster], [calc()],
#' [rasterToVRT()]
#'
#' @examples
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
#' evh_file <- system.file("extdata/storml_evh.tif", package="gdalraster")
#' rasterfiles <- c(evt_file, evc_file, evh_file)
#' var.names <- c("veg_type", "veg_cov", "veg_ht")
#' df <- combine(rasterfiles, var.names)
#' head(df)
#'
#' ## combine two bands from a multi-band file and write the combination IDs 
#' ## to an output raster
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' rasterfiles <- c(lcp_file, lcp_file)
#' bands <- c(4, 5)
#' var.names <- c("fbfm", "tree_cov")
#' cmb_file <- paste0(tempdir(), "/", "fbfm_cov_cmbid.tif")
#' options <- c("COMPRESS=LZW")
#' df <- combine(rasterfiles, var.names, bands, cmb_file, options = options)
#' head(df)
#' ds <- new(GDALRaster, cmb_file, TRUE)
#' ds$info()
#' ds$close()
#' @export
combine <- function(rasterfiles, var.names=NULL, bands=NULL, 
					dstfile=NULL, fmt=NULL, dtName="UInt32", options=NULL) {

	if ( (!is.null(dstfile)) && (is.null(fmt)) ) {
		fmt <- .getGDALformat(dstfile)
		if (is.null(fmt)) {
			stop("Use fmt argument to specify a GDAL raster format name.")
		}
	}
	
	if (is.null(dstfile))
		dstfile <- ""
	
	if (is.null(fmt))
		fmt <- ""
	
	nrasters <- length(rasterfiles)
	if (is.null(var.names)) {
		for (n in 1:nrasters) 
			var.names[n] <- tools::file_path_sans_ext(basename(rasterfiles[n]))
	}
	
	if (is.null(bands)) {
		bands <- rep(1,nrasters)
	}
	
	ref_ds <- new(GDALRaster, rasterfiles[1], read.only=TRUE)
	ref_nrows <- ref_ds$getRasterYSize()
	ref_ncols <- ref_ds$getRasterXSize()
	ref_gt <- ref_ds$getGeoTransform()
	ref_res <- ref_ds$res()
	ref_ds$close()
	
	if (nrasters > 1) {
		for (i in 2:length(rasterfiles)) {
			ds <- new(GDALRaster, rasterfiles[i], read.only=TRUE)
			if ( (ds$getRasterYSize() != ref_nrows) ||
					(ds$getRasterXSize() != ref_ncols) )
				stop("All input rasters must have the same extent and 
					cell size.", call. = FALSE)
			if ( !all(ds$res() == ref_res) )
				stop("All input rasters must have the same extent and 
					cell size.", call. = FALSE)
			ds$close()
		}
	}

	df <- .combine(rasterfiles, var.names, bands, dstfile, fmt, dtName, options)
	return(df)
}
