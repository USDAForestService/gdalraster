test_that("srs functions work", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	ds <- new(GDALRaster, elev_file, read_only=TRUE)
	srs <- ds$getProjectionRef()
	ds$close()
	expect_false(srs_is_geographic(srs))
	expect_true(srs_is_projected(srs))
	# EPSG:26912 - NAD83 / UTM zone 12N
	utm <- epsg_to_wkt(26912)
	expect_true(srs_is_same(srs, utm))
	
	expect_true(srs_is_same(
				srs_to_wkt("EPSG:4326"),
				srs_to_wkt("OGC:CRS84"))
				)
	expect_true(srs_is_same(
				srs_to_wkt("EPSG:4326"),
				srs_to_wkt("OGC:CRS84"),
				ignore_axis_mapping=TRUE)
				)
	expect_false(srs_is_same(
				srs_to_wkt("EPSG:4326"),
				srs_to_wkt("OGC:CRS84"),
				ignore_axis_mapping=TRUE,
				criterion="STRICT")
				)
	
	expect_equal(srs_to_wkt("NAD83"), epsg_to_wkt(4269))
	expect_equal(srs_to_wkt("NAD83", pretty=TRUE),
					epsg_to_wkt(4269, pretty=TRUE))
})
