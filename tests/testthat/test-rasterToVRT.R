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
})
