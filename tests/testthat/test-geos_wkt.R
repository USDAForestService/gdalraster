skip_if_not(has_geos())

test_that("geos functions work on wkt geometries", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	ds <- new(GDALRaster, elev_file, read_only=TRUE)
	bb <- bbox_to_wkt(ds$bbox())
	ds$close()

	bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2 
	5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5, 
	325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
	expect_true(.g_is_valid(bnd))

	pt_xy <- matrix(c(324171, 5103034.3), nrow=1, ncol=2)
	pt <- .g_create(pt_xy, "POINT")
	
	line_xy <- matrix(c(324171, 327711.7, 5103034.3, 5104475.9),
						nrow=2, ncol=2)
	line <- .g_create(line_xy, "LINESTRING")

	expect_true(.g_intersects(bb, pt))
	expect_false(.g_intersects(bnd, pt))

	expect_false(.g_equals(bb, bnd))

	expect_false(.g_disjoint(bb, bnd))

	expect_false(.g_touches(bb, bnd))

	expect_true(.g_contains(bb, bnd))

	expect_false(.g_within(bb, bnd))

	expect_true(.g_crosses(line, bnd))
	expect_false(.g_crosses(line, bb))

	expect_false(.g_overlaps(bb, bnd))

	expect_equal(round(bbox_from_wkt(.g_buffer(bnd, 100))),
					round(c(323694.2, 5102785.8, 326520.0, 5105029.4)))

	expect_equal(round(.g_area(.g_difference(bb, bnd))), 9731255)

	expect_equal(round(.g_area(.g_sym_difference(bb, bnd))), 9731255)

	expect_equal(round(.g_distance(pt, bnd), 2), round(215.0365, 2))

	expect_equal(round(.g_area(bnd)), 4039645)
	
	expect_equal(round(.g_length(line), 1), round(3822.927, 1))

	expect_equal(round(.g_centroid(bnd)), round(c(325134.9, 5103985.4)))
})
