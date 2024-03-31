test_that("geos_version returns a list of length 4", {
    expect_length(geos_version(), 4)
})

test_that("bbox intersect/union return correct values", {
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

test_that("g_transform / bbox_transform return correct values", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file)
    bbox_wgs84 <- c(-113.28289, 46.04764, -113.22629, 46.07760)

    expect_error(g_transform("invalid WKT", ds$getProjectionRef(),
                             epsg_to_wkt(4326)))
    expect_error(g_transform(bbox_to_wkt(ds$bbox()), "invalid WKT",
                             epsg_to_wkt(4326)))
    expect_error(g_transform(bbox_to_wkt(ds$bbox()), ds$getProjectionRef(),
                             "invalid WKT"))

    bbox_test <- ds$bbox() |>
        bbox_to_wkt() |>
        g_transform(srs_from = ds$getProjectionRef(),
                    srs_to = epsg_to_wkt(4326)) |>
        bbox_from_wkt()

    expect_equal(bbox_test, bbox_wgs84, tolerance = 0.001)

    bbox_test <- bbox_transform(ds$bbox(), ds$getProjectionRef(),
                                epsg_to_wkt(4326))
    expect_equal(bbox_test, bbox_wgs84, tolerance = 0.001)
    ds$close()
})

test_that("geometry properties are correct", {
    expect_true(g_is_valid("POLYGON ((0 0, 10 10, 10 0, 0 0))"))
    expect_false(g_is_valid("POLYGON ((0 0, 10 10, 10 0, 0 1))"))

    expect_false(g_is_empty("POLYGON ((0 0, 10 10, 10 0, 0 0))"))
    expect_true(g_is_empty("POLYGON EMPTY"))

    bb <- c(323476.1, 5101872.0,  327766.1, 5105082.0)
    expect_equal(bbox_to_wkt(bb) |> g_name(), "POLYGON")
})

test_that("geometry binary predicates/ops return correct values", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file)
    bb <- ds$bbox() |> bbox_to_wkt()
    ds$close()

    bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
    5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
    325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"

    bnd_disjoint <- "POLYGON ((324457.6 5101227.0, 323899.7 5100778.3,
    323784.5 5099868.7, 324961.0 5099298.7, 326410.3 5100008.2, 326380.0
    5101160.4, 325288.4 5101342.2, 325288.4 5101342.2, 324457.6 5101227.0))"

    bnd_overlaps <- "POLYGON ((327381.9 5104541.2, 326824.0 5104092.5, 326708.8
    5103182.9, 327885.2 5102612.9, 329334.5 5103322.4, 329304.2 5104474.5,
    328212.7 5104656.4, 328212.7 5104656.4, 327381.9 5104541.2))"

    expect_true(g_intersects(bb, bnd_overlaps))
    expect_false(g_intersects(bb, bnd_disjoint))
    expect_false(g_equals(bnd, bnd_overlaps))
    expect_true(g_disjoint(bb, bnd_disjoint))
    expect_false(g_disjoint(bb, bnd_overlaps))
    expect_true(g_contains(bb, bnd))
    expect_false(g_contains(bnd, bb))
    expect_false(g_within(bb, bnd))
    expect_true(g_within(bnd, bb))
    expect_true(g_overlaps(bb, bnd_overlaps))
    expect_false(g_overlaps(bb, bnd_disjoint))
    expect_false(g_overlaps(bb, bnd))

    linestr <- "LINESTRING (324467.3 5104814.2, 323909.4 5104365.4)"
    expect_true(g_touches(bnd, linestr))
    expect_false(g_touches(bb, linestr))
    expect_false(g_crosses(bb, linestr))

    g1 <- g_intersection(bb, bnd_overlaps)
    g2 <- g_union(bb, bnd_overlaps)
    g3 <- g_sym_difference(bb, bnd_overlaps)
    expect_equal(g_area(g3), g_area(g_difference(g2, g1)), tolerance = 0.01)
    expect_true(g_difference(bnd, bb) |> g_is_empty())
    expect_false(g_difference(bnd_overlaps, bb) |> g_is_empty())
})

test_that("g_buffer returns correct values", {
    pt <- "POINT (0 0)"
    expect_error(g_buffer(wkt = "invalid WKT", dist = 10))
    bb <- bbox_from_wkt(g_buffer(wkt = pt, dist = 10))
    expect_equal(bb, c(-10, -10,  10,  10))
})

test_that("geometry measures are correct", {
    expect_equal(g_distance("POINT (0 0)", "POINT (5 12)"), 13)

    expect_equal(g_length("LINESTRING (0 0, 3 4)"), 5)

    bb <- c(323476.1, 5101872.0,  327766.1, 5105082.0)

    bb_area <- (bb[3] - bb[1]) * (bb[4] - bb[2])
    expect_equal(g_area(bbox_to_wkt(bb)), bb_area)

    expect_equal(g_centroid(bbox_to_wkt(bb)), c(325621.1, 5103477.0))
})
