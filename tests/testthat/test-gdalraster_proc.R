test_that(".getGDALformat works", {
	expect_equal(.getGDALformat("test.tif"), "GTiff")
	expect_equal(.getGDALformat("test.TIF"), "GTiff")
	expect_equal(.getGDALformat("test.img"), "HFA")
	expect_equal(.getGDALformat("test.vrt"), "VRT")
	expect_equal(.getGDALformat("test.VRT"), "VRT")
	expect_null(.getGDALformat("test.unknown"))
})

test_that("calc writes correct results", {
	# bioclimatic index
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	expr <- "round( ((ELEV_M * 3.281 - 5449) / 100) + 
				((pixelLat - 42.16) * 4) + 
				((-116.39 - pixelLon) * 1.25) )"
	hi_file <- calc(expr = expr, 
					rasterfiles = elev_file, 
					var.names = "ELEV_M", 
					dtName = "Int16",
					setRasterNodataValue = TRUE,
					usePixelLonLat = TRUE)
	on.exit(unlink(hi_file))
	ds <- new(GDALRaster, hi_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 2978)

	# NDVI
	b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
	b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
	expr <- "((B5-B4)/(B5+B4)) * 1000" # rescale for checksum
	ndvi_file <- calc(expr = expr,
					rasterfiles = c(b4_file, b5_file),
					var.names = c("B4", "B5"),
					dtName = "Float32",
					nodata_value = -32767,
					setRasterNodataValue = TRUE)
	on.exit(unlink(ndvi_file))
	ds <- new(GDALRaster, ndvi_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 63632)
	
	# recode
	lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
	tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
	on.exit(unlink(tif_file))
	createCopy("GTiff", tif_file, lcp_file)
	expr <- "ifelse( SLP >= 40 & FBFM %in% c(101,102), 99, FBFM)"
	calc(expr = expr,
			rasterfiles = c(lcp_file, lcp_file),
			bands = c(2, 4),
			var.names = c("SLP", "FBFM"),
			dstfile = tif_file,
			out_band = 4,
			write_mode = "update")
	ds <- new(GDALRaster, tif_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 28017)
	
	# test errors from input validation
	expr <- "((B5-B4)/(B5+B4))"
	expect_error(calc(expr = expr,
					rasterfiles = c(b4_file, paste0(b5_file, ".error")),
					var.names = c("B4", "B5"),
					dtName = "Float32",
					nodata_value = -32767,
					setRasterNodataValue = TRUE))

	expect_error(calc(expr = expr,
					rasterfiles = c(b4_file, b5_file),
					bands = 1,
					var.names = c("B4", "B5"),
					dtName = "Float32",
					nodata_value = -32767,
					setRasterNodataValue = TRUE))	

	expect_error(calc(expr = expr,
					rasterfiles = c(b4_file, b5_file),
					var.names = c("B4", "B5", "err"),
					dtName = "Float32",
					nodata_value = -32767,
					setRasterNodataValue = TRUE))	

	expect_error(calc(expr = expr,
					rasterfiles = c(b4_file, b5_file),
					dtName = "Float32",
					nodata_value = -32767,
					setRasterNodataValue = TRUE))

	expect_error(calc(expr = expr,
					rasterfiles = c(b4_file, b5_file),
					var.names = c("B4", "B5"),
					dstfile = "unknown_file_type.err",
					dtName = "Float32",
					nodata_value = -32767,
					setRasterNodataValue = TRUE))

	expect_error(calc(expr = expr,
					rasterfiles = c(b4_file, b5_file),
					var.names = c("B4", "B5"),
					dtName = "Int64",
					setRasterNodataValue = TRUE))

	expect_error(calc(expr = expr,
					rasterfiles = c(b4_file, elev_file),
					var.names = c("B4", "B5"),
					dtName = "Float32",
					nodata_value = -32767,
					setRasterNodataValue = TRUE))
})

test_that("combine writes correct output", {
	lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
	rasterfiles <- c(lcp_file, lcp_file)
	bands <- c(4, 5)
	var.names <- c("fbfm", "tree_cov")
	cmb_file <- paste0(tempdir(), "/", "fbfm_cov_cmbid.tif")
	on.exit(unlink(cmb_file))
	df <- combine(rasterfiles, var.names, bands, cmb_file)
	expect_equal(nrow(df), 29)
	df <- NULL
	ds <- new(GDALRaster, cmb_file, TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 43024)
	
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	df <- combine(evt_file)
	expect_equal(nrow(df), 24)
})

test_that("rasterFromRaster works", {
	lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
	slpp_file <- paste0(tempdir(), "/", "storml_slpp.tif")
	on.exit(unlink(slpp_file))
	options = c("COMPRESS=LZW")
	rasterFromRaster(srcfile = lcp_file,
						dstfile = slpp_file,
						nbands = 1,
						dtName = "Int16",
						options = options,
						init = -32767)
	ds <- new(GDALRaster, slpp_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 47771)
	
	slpp_file_img <- paste0(tempdir(), "/", "storml_slpp.img")
	on.exit(unlink(slpp_file_img))
	options = c("COMPRESSED=YES")
	rasterFromRaster(srcfile = lcp_file,
						dstfile = slpp_file_img,
						nbands = 1,
						dtName = "Int16",
						options = options,
						init = -32767)
	ds <- new(GDALRaster, slpp_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 47771)
	
	slpp_file_envi <- paste0(tempdir(), "/", "storml_slpp")
	on.exit(unlink(slpp_file_envi))
	on.exit(unlink(paste0(slpp_file_envi, ".hdr")))
	# error on unknown file format:
	expect_error(rasterFromRaster(srcfile = lcp_file,
						dstfile = slpp_file_envi,
						nbands = 1,
						dtName = "Int16",
						init = -32767))
	rasterFromRaster(srcfile = lcp_file,
						dstfile = slpp_file_envi,
						fmt = "ENVI",
						nbands = 1,
						dtName = "Int16",
						init = -32767)
	ds <- new(GDALRaster, slpp_file_envi, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 47771)
})

test_that("rasterToVRT works", {
	## resample
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	vrt_file <- rasterToVRT(evt_file,
							resolution=c(90,90),
							resampling="mode")
	on.exit(unlink(vrt_file))
	ds <- new(GDALRaster, vrt_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 23684)
	
	## clip raster to polygon extent
	bnd = "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2 
	5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5, 
	325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
	
	# src_align = TRUE
	vrt_file <- rasterToVRT(evt_file,
							subwindow = bbox_from_wkt(bnd),
							src_align=TRUE)
	on.exit(unlink(vrt_file))
	ds <- new(GDALRaster, vrt_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 18148)
	
	# src_align = FALSE
	vrt_file <- rasterToVRT(evt_file,
							subwindow = bbox_from_wkt(bnd),
							src_align=FALSE)
	on.exit(unlink(vrt_file))
	ds <- new(GDALRaster, vrt_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 17631)
	
	# subwindow outside raster extent
	expect_error(rasterToVRT(evt_file,
				subwindow = bbox_from_wkt(.g_buffer(bnd, 10000)),
				src_align=TRUE))

	## subset and pixel align two rasters
	lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
	ds_lcp <- new(GDALRaster, lcp_file, read_only=TRUE)
	b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
	vrt_file <- rasterToVRT(b5_file,
							resolution = ds_lcp$res(),
							subwindow = ds_lcp$bbox(),
							src_align = FALSE)
	on.exit(unlink(vrt_file))
	ds_lcp$close()
	ds <- new(GDALRaster, vrt_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 49589)	
	
	## kernel filter
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	krnl <- c(0.11111, 0.11111, 0.11111,
			  0.11111, 0.11111, 0.11111,
			  0.11111, 0.11111, 0.11111)
	vrt_file <- rasterToVRT(elev_file, krnl=krnl)
	on.exit(unlink(vrt_file))
	ds <- new(GDALRaster, vrt_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 46590)
	
	expect_error(rasterToVRT(elev_file, resolution=c(90,90), krnl=krnl))
})

