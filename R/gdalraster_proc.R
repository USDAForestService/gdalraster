# R functions for various processing with gdalraster
# Chris Toney <chris.toney at usda.gov>

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
.getGDALformat <- function(file) {
# Only for guessing common output formats
# Could use GDALIdentifyDriver to be more comprehensive
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
#' (e.g., \code{options = c("COMPRESS=DEFLATE")} to set \code{DEFLATE}
#' compression during creation of a GTiff file).
#' @param init Numeric value to initialize all pixxels in the output raster.
#' @param dstnodata Numeric nodata value for the output raster.
#' @returns Returns the destination filename invisibly.
#' @seealso
#' [create()], [createCopy()], [rasterToVRT()]
#' @examples
#' ## band 2 in a FARSITE landscape file has slope degrees
#' ## verify this and convert slope degrees to slope percent in a new raster
#' lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
#' ds_lcp <- new(GDALRaster, lcp_file, read_only=TRUE)
#' ds_lcp$info()
#' ds_lcp$getMetadata(band=2, domain="")
#' 
#' slpp_file <- paste0(tempdir(), "/", "storml_slpp.tif")
#' options = c("COMPRESS=DEFLATE")
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
#' ncols <- ds_slp$getRasterXSize()
#' nrows <- ds_slp$getRasterYSize()
#' for (row in 0:(nrows-1)) {
#'     rowdata <- ds_lcp$read(band=2, 
#'                            xoff=0, yoff=row,
#'                            xsize=ncols, ysize=1,
#'                            out_xsize=ncols, out_ysize=1)
#'     rowslpp <- tan(rowdata*pi/180) * 100;
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
#' A VRT dataset is saved as a plain-text file with extension .vrt. This file 
#' contains a description of the dataset in an XML format. The description 
#' includes the source raster filename which can be a full path 
#' (`relativeToVRT = FALSE`) or the just the filename with no path 
#' (`relativeToVRT = TRUE`) in which case `rasterToVRT()` assumes that the 
#' .vrt file will be in the same directory as the source file. The different 
#' elements of the XML schema specify how the source data will be read, along 
#' with algorithms potentially applied and so forth. The full specification of 
#' the XML format for .vrt is at: 
#' \url{https://gdal.org/drivers/raster/vrt.html}.
#' 
#' Since .vrt is a small plain-text file it is fast to write and requires 
#' little storage space. Read performance is not degraded for certain simple 
#' operations (e.g., virtual clip without resampling). Reading will be 
#' slower for virtual resampling to a different pixel resolution or virtual 
#' kernel filtering since the operations are performed on-the-fly (but .vrt 
#' does not require the up front writing of a resampled or kernel-filtered 
#' raster to a regular format). VRT is sometimes useful as an intermediate 
#' raster in a series of processing steps, including as a `tempfile` (the 
#' default).
#' 
#' Note that VRT format has several capabilities and uses beyond  
#' those covered by `rasterToVRT()`. See the format description URL above for 
#' a full discussion. `warp()` can write to VRT format for virtual 
#' reprojection.
#' 
#' `rasterToVRT()` assumes `srcfile` is a north-up raster.
#' Requires package `xml2`.
#'
#' @param srcfile Source raster filename.
#' @param relativeToVRT Logical. Indicates whether the source filename should 
#' be interpreted as relative to the .vrt file (`TRUE`) or not relative
#' to the .vrt file (`FALSE`, the default). If `TRUE`, the .vrt 
#' file is assumed to be in the same directory as `srcfile` and
#' `basename(srcfile)` is used in the .vrt file.
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
#' @param align Logical.
#'   * `TRUE`: the upper left corner of the VRT extent will be set to the 
#'   upper left corner of the source pixel that contains `subwindow` xmin, ymax. 
#'   The VRT will be pixel-aligned with source if the VRT `resolution` is the 
#'   same as the source pixel size, otherwise VRT extent will be the minimum 
#'   rectangle that contains `subwindow` for the given pixel size.
#'   * `FALSE`: the VRT upper left corner will be exactly `subwindow` 
#'   xmin, ymax, and the VRT extent will be the minimum rectangle that contains 
#'   `subwindow` for the given pixel size. If `subwindow` is not given, the 
#'   source raster extent is used in which case `align=FALSE` has no effect.
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
#' @seealso
#' [bbox_from_wkt()], [warp()]
#' @examples
#' ## resample
#'
#' evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
#' ds <- new(GDALRaster, evt_file, TRUE)
#' ds$res()
#' ds$bbox()
#' ds$close()
#' 
#' ## using combine() with one input to get a table of pixel counts for each 
#' ## raster value
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
#' ## .vrt is a small plain-text xml file pointing to the source raster
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
#' ## clip
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
#' vrt_file <- rasterToVRT(evt_file,
#'                         subwindow = bbox_from_wkt(bnd),
#'                         align=TRUE)
#' ds_vrt <- new(GDALRaster, vrt_file, TRUE)
#' 
#' ## VRT is a virtual clip, pixel-aligned with the EVT raster
#' bbox_from_wkt(bnd)
#' ds_vrt$bbox()
#' ds_vrt$res()
#' 
#' vrt_file <- rasterToVRT(evt_file,
#'                         subwindow = bbox_from_wkt(bnd),
#'                         align=FALSE)
#' ds_vrt_noalign <- new(GDALRaster, vrt_file, TRUE)
#' 
#' ## VRT upper left corner (xmin, ymax) is exactly bnd xmin, ymax
#' ds_vrt_noalign$bbox()
#' ds_vrt_noalign$res()
#' 
#' ds_vrt$close()
#' ds_vrt_noalign$close()
#' ds_evt$close()
#' @export
rasterToVRT <- function(srcfile, relativeToVRT = FALSE, 
				vrtfile = tempfile("tmprast", fileext=".vrt"), 
				resolution = NULL, 
				subwindow = NULL, 
				align = TRUE, 
				resampling = "nearest",
				krnl = NULL,
				normalized = TRUE) {

	if (!requireNamespace("xml2", quietly = TRUE))
		stop("rasterToVRT() requires package \"xml2.\"", call. = FALSE)

	if (relativeToVRT) relativeToVRT = 1 else relativeToVRT = 0
	if (normalized) normalized = 1 else normalized = 0

	src_ds <- new(GDALRaster, srcfile, read.only=TRUE)
	src_nrows <- src_ds$getRasterYSize()
	src_ncols <- src_ds$getRasterXSize()
	src_gt <- src_ds$getGeoTransform()
	src_xmin <- src_ds$bbox()[1]
	src_ymin <- src_ds$bbox()[2]
	src_xmax <- src_ds$bbox()[3]
	src_ymax <- src_ds$bbox()[4]
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
		if (subwindow[1] < src_xmin || subwindow[3] > src_xmax || 
			subwindow[2] < src_ymin || subwindow[4] > src_ymax) {
			stop("Subwindow is not completely within source raster extent.",
					call. = FALSE)
		}
	}
	src_xoff <- floor(.getOffset(subwindow[1], src_xmin, src_xres))
	src_yoff <- floor(.getOffset(subwindow[4], src_ymax, src_yres))
	
	#get vrt geotransform and size (assuming a north-up raster)
	if (align) {
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
		vrt_ds <- new(GDALRaster, vrtfile, read_only=F)
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
#' should be large enough to accomodate the potential number of unique 
#' combinations of the input values 
#' (e.g., "UInt16" or the default "UInt32").
#' @param options Optional list of format-specific creation options in a
#' vector of "NAME=VALUE" pairs.
#' (e.g., \code{options = c("COMPRESS=DEFLATE")} to set \code{DEFLATE}
#' compression during creation of a GTiff file).
#' @returns A data frame with column `cmbid` containing the combination IDs, 
#' column `count` containing the pixel counts for each combination, 
#' and `length(rasterfiles)` columns named `var.names` containing the integer 
#' values comprising each unique combination.
#' @seealso
#' class [`CmbTable`][CmbTable], [rasterToVRT()]
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
#' options <- c("COMPRESS=DEFLATE")
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
	ref_ds$close()
	
	if (nrasters > 1) {
		for (i in 2:length(rasterfiles)) {
			ds <- new(GDALRaster, rasterfiles[i], read.only=TRUE)
			if ( (ds$getRasterYSize() != ref_nrows) ||
					(ds$getRasterXSize() != ref_ncols) )
				stop("All input rasters must have the same extent and 
					cell size.", call. = FALSE)
			if ( !all(ds$getGeoTransform() == ref_gt) )
				stop("All input rasters must have the same extent and 
					cell size.", call. = FALSE)
			ds$close()
		}
	}

	df <- .combine(rasterfiles, var.names, bands, dstfile, fmt, dtName, options)
	return(df)
}
