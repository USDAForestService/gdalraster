skip_if_not(has_geos())

test_that("geos functions work on wkt geometries", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file, read_only=TRUE)
    bb <- bbox_to_wkt(ds$bbox())
    ds$close()

    bad_bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
    5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
    325298.1 5104929.4))"
    expect_false(.g_is_valid(bad_bnd))

    bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
    5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
    325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
    expect_true(.g_is_valid(bnd))

    x <- c(324467.3, 323909.4, 323794.2)
    y <- c(5104814.2, 5104365.4, 5103455.8)
    pt_xy <- cbind(x, y)
    expect_error(.g_create(pt_xy, "POLYGON"))

    if (as.integer(gdal_version()[2]) >= 3050000) {
        # OGR_GEOMETRY_ACCEPT_UNCLOSED_RING config option added at 3.5.0
        x <- c(324467.3, 323909.4, 323794.2, 324970.7)
        y <- c(5104814.2, 5104365.4, 5103455.8, 5102885.8)
        pt_xy <- cbind(x, y)
        expect_error(.g_create(pt_xy, "POLYGON"))
    }

    x <- c(324467.3, 323909.4, 323794.2, 324970.7, 326420.0, 326389.6, 325298.1, 325298.1, 324467.3)
    y <- c(5104814.2, 5104365.4, 5103455.8, 5102885.8, 5103595.3, 5104747.5, 5104929.4, 5104929.4, 5104814.2)
    pt_xy <- cbind(x, y)
    expect_true(startsWith(.g_create(pt_xy, "POLYGON"), "POLYGON"))

    pt_xy <- matrix(c(324171, 5103034.3), nrow=1, ncol=2)
    expect_error(.g_create(pt_xy, "POLYGON"))
    pt <- .g_create(pt_xy, "POINT")

    line_xy <- matrix(c(324171, 327711.7, 5103034.3, 5104475.9),
                        nrow=2, ncol=2)
    expect_error(.g_create(line_xy, "POINT"))
    line <- .g_create(line_xy, "LINESTRING")

    expect_true(.g_intersects(bb, pt))
    expect_false(.g_intersects(bnd, pt))
    expect_error(.g_intersects("invalid WKT", pt))
    expect_error(.g_intersects(bnd, "invalid WKT"))

    expect_false(.g_equals(bb, bnd))
    expect_error(.g_equals("invalid WKT", bnd))
    expect_error(.g_equals(bb, "invalid WKT"))

    expect_false(.g_disjoint(bb, bnd))
    expect_error(.g_disjoint("invalid WKT", bnd))
    expect_error(.g_disjoint(bb, "invalid WKT"))

    expect_false(.g_touches(bb, bnd))
    expect_error(.g_touches("invalid WKT", bnd))
    expect_error(.g_touches(bb, "invalid WKT"))

    expect_true(.g_contains(bb, bnd))
    expect_error(.g_contains("invalid WKT", bnd))
    expect_error(.g_contains(bb, "invalid WKT"))

    expect_false(.g_within(bb, bnd))
    expect_error(.g_within("invalid WKT", bnd))
    expect_error(.g_within(bb, "invalid WKT"))

    expect_true(.g_crosses(line, bnd))
    expect_error(.g_crosses("invalid WKT", bnd))
    expect_error(.g_crosses(line, "invalid WKT"))
    expect_false(.g_crosses(line, bb))

    expect_false(.g_overlaps(bb, bnd))
    expect_error(.g_overlaps("invalid WKT", bnd))
    expect_error(.g_overlaps(bb, "invalid WKT"))

    expect_equal(round(bbox_from_wkt(.g_buffer(bnd, 100))),
            round(c(323694.2, 5102785.8, 326520.0, 5105029.4)))
    expect_error(.g_buffer("invalid WKT", 100))

    expect_equal(round(.g_area(bnd)), 4039645)
    expect_equal(.g_area(.g_intersection(bb, bnd)), .g_area(bnd))
    expect_equal(.g_area(.g_union(bb, bnd)), .g_area(bb))
    expect_equal(round(.g_area(.g_difference(bb, bnd))), 9731255)
    expect_equal(round(.g_area(.g_sym_difference(bb, bnd))), 9731255)
    expect_error(.g_area("invalid WKT"))

    expect_equal(round(.g_distance(pt, bnd), 2), round(215.0365, 2))
    expect_error(.g_distance("invalid WKT"))

    expect_equal(round(.g_length(line), 1), round(3822.927, 1))
    expect_error(.g_length("invalid WKT"))

    expect_equal(round(.g_centroid(bnd)), round(c(325134.9, 5103985.4)))
    expect_error(.g_centroid("invalid WKT"))

    expect_error(.g_intersection("invalid WKT", bnd))
    expect_error(.g_intersection(bb, "invalid WKT"))
    expect_error(.g_union("invalid WKT", bnd))
    expect_error(.g_union(bb, "invalid WKT"))
    expect_error(.g_difference("invalid WKT", bnd))
    expect_error(.g_difference(bb, "invalid WKT"))
    expect_error(.g_sym_difference("invalid WKT", bnd))
    expect_error(.g_sym_difference(bb, "invalid WKT"))
})
