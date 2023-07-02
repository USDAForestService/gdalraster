test_that("rasterFromRaster works", {
	lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
	slpp_file <- paste0(tempdir(), "/", "storml_slpp.tif")
	options = c("COMPRESS=LZW")
	rasterFromRaster(srcfile = lcp_file,
						dstfile = slpp_file,
						nbands = 1,
						dtName = "Int16",
						options = options,
						init = -32767)
	on.exit(unlink(slpp_file))
	ds <- new(GDALRaster, slpp_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 47771)
})
