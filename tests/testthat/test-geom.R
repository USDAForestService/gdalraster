skip_if_not(has_geos())

test_that("intersect/union return correct values", {
	bbox_list <-list()
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	ds <- new(GDALRaster, elev_file, read_only=TRUE)
	bbox_list[[1]] <- ds$bbox()
	ds$close()
	b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
	ds <- new(GDALRaster, b5_file, read_only=TRUE)
	bbox_list[[2]] <- ds$bbox()
	ds$close()
	bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2 
	5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5, 
	325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
	bbox_list[[3]] <- bbox_from_wkt(bnd)
	expect_equal(bbox_intersect(bbox_list),
					c(323794.2, 5102885.8, 326420.0, 5104929.4))
	expect_equal(bbox_union(bbox_list),
					c(323400.9, 5101815.8, 327870.9, 5105175.8))
	expect_equal(bbox_intersect(c(elev_file, b5_file)),
					c(323476.1, 5101872.0, 327766.1, 5105082.0))
	expect_equal(bbox_union(c(elev_file, b5_file)),
					c(323400.9, 5101815.8, 327870.9, 5105175.8))
	expect_equal(bbox_from_wkt(bbox_union(c(elev_file, b5_file), as_wkt=TRUE)),
					c(323400.9, 5101815.8, 327870.9, 5105175.8))
})
