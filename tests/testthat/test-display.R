test_that("plot_raster works", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	ds <- new(GDALRaster, elev_file, read_only=TRUE)
	ncols <- ds$getRasterXSize()
	nrows <- ds$getRasterYSize()
	r <- read_ds(ds)
	ds$close()
	expect_silent(plot_raster(r, xsize=ncols, ysize=nrows))
})
