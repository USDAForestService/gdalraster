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
					nodata_value = -32767, 
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
})
