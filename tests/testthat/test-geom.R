test_that("geos_version returns a list of length 4", {
    expect_length(geos_version(), 4)
})

test_that("geos internal functions work on wkt geometries", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file, read_only=TRUE)
    bb <- bbox_to_wkt(ds$bbox())
    ds$close()

    bad_bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
    5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
    325298.1 5104929.4))"
    expect_false(g_is_valid(bad_bnd))

    bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
    5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
    325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
    expect_true(g_is_valid(bnd))

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

    expect_true(.g_create(pt_xy, "POLYGON") |> g_is_valid())
    expect_true((.g_create(pt_xy, "POLYGON") |> g_area()) > 0)

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

    expect_equal(round(bbox_from_wkt(g_buffer(bnd, 100, as_wkb = FALSE))),
                 round(c(323694.2, 5102785.8, 326520.0, 5105029.4)))
    expect_error(g_buffer("invalid WKT", 100))

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

test_that("WKB/WKT conversion functions work", {
    # test round trip on point and multipolygon
    g1 <- "POINT (1 5)"
    g2 <- "MULTIPOLYGON (((5 5,0 0,0 10,5 5)),((5 5,10 10,10 0,5 5)))"

    expect_true(g_equals(g1, g_wk2wk(g1) |> g_wk2wk()))
    expect_true(g_equals(g2, g_wk2wk(g2) |> g_wk2wk()))

    # character vector of WKT strings to list of WKB raw vectors
    wkb_list <- g_wk2wk(c(g1, g2))
    expect_length(wkb_list, 2)
    expect_true(is.raw(wkb_list[[1]]))
    expect_true(is.raw(wkb_list[[2]]))

    # list of WKB raw vectors to character vector of WKT strings
    wkt_vec <- g_wk2wk(wkb_list)
    expect_length(wkt_vec, 2)
    expect_true(is.character(wkt_vec))
    expect_true(g_equals(wkt_vec[1], g1))
    expect_true(g_equals(wkt_vec[2], g2))

    # test with first element a length-0 raw vector
    wkb_list[[1]] <- raw()
    rm(wkt_vec)
    expect_warning(wkt_vec <- g_wk2wk(wkb_list))
    expect_length(wkt_vec, 2)
    expect_equal(wkt_vec[1], "")
    expect_true(g_equals(wkt_vec[2], g2))

    # test with first element not a raw vector
    wkb_list[[1]] <- g1
    rm(wkt_vec)
    expect_warning(wkt_vec <- g_wk2wk(wkb_list))
    expect_length(wkt_vec, 2)
    expect_true(is.na(wkt_vec[1]))
    expect_true(g_equals(wkt_vec[2], g2))

    # first element of wkt_vec is NA
    rm(wkb_list)
    expect_warning(wkb_list <- g_wk2wk(wkt_vec))
    expect_length(wkb_list, 2)
    expect_true(is.na(wkb_list[[1]]))
    expect_true(g_equals(wkb_list[[2]] |> g_wk2wk(), g2))

    # POINT EMPTY special case
    wkt <- "POINT EMPTY"
    expect_warning(wkb <- g_wk2wk(wkt))
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


    skip_if(.gdal_version_num() < 3070000 )

    g <- "MULTIPOLYGON (((10 0,0 0,5 5,10 0)),((10 10,5 5,0 10,10 10)))"
    expect_summary <- "MULTIPOLYGON : 2 geometries: POLYGON : 4 points POLYGON : 4 points"
    expect_equal(g_summary(g), expect_summary)
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
    bb <- bbox_from_wkt(g_buffer(pt, dist = 10, as_wkb = FALSE))
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

test_that("make valid geometry works", {
    # test only with recent GDAL and GEOS
    # these tests could give different results if used across a range of older
    # GDAL/GEOS versions, and the STRUCTURE method requires GEOS >= 3.10
    skip_if(.gdal_version_num() < 3080000 ||
                geos_version()$major < 3 ||
                (geos_version()$major == 3 && geos_version()$minor < 11))

    # valid to valid
    wkt1 <- "POINT (0 0)"
    expect_no_error(wkb1 <- g_make_valid(wkt1))
    expect_true(g_equals(g_wk2wk(wkb1), wkt1))

    # invalid - error
    wkt2 <- "LINESTRING (0 0)"
    expect_warning(wkb2 <- g_make_valid(wkt2))
    expect_true(is.na(wkb2))

    # invalid to valid
    wkt3 <- "POLYGON ((0 0,10 10,0 10,10 0,0 0))"
    expect_no_error(wkb3 <- g_make_valid(wkt3))
    expected_wkt3 <- "MULTIPOLYGON (((10 0,0 0,5 5,10 0)),((10 10,5 5,0 10,10 10)))"
    expect_true(g_equals(g_wk2wk(wkb3), expected_wkt3))

    # STRUCTURE method
    wkt4 <- "POLYGON ((0 0,0 10,10 10,10 0,0 0),(5 5,15 10,15 0,5 5))"
    expect_no_error(wkb4 <- g_make_valid(wkt4, method = "STRUCTURE"))
    expected_wkt4 <- "POLYGON ((0 10,10 10,10.0 7.5,5 5,10.0 2.5,10 0,0 0,0 10))"
    expect_true(g_equals(g_wk2wk(wkb4), expected_wkt4))

    # vector of WKT input
    wkt_vec <- c(wkt1, wkt2, wkt3, wkt4)
    expect_warning(wkb_list <- g_make_valid(wkt_vec, method = "STRUCTURE"))
    expect_equal(length(wkb_list), length(wkt_vec))
    expect_true(is.na(wkb_list[[2]]))
    expect_true(g_equals(g_wk2wk(wkb_list[[4]]), expected_wkt4))

    # list of WKB input
    rm(wkb_list)
    expect_warning(wkb_list <- g_make_valid(g_wk2wk(wkt_vec), method = "STRUCTURE"))
    expect_equal(length(wkb_list), length(wkt_vec))
    expect_true(is.na(wkb_list[[2]]))
    expect_true(g_equals(g_wk2wk(wkb_list[[4]]), expected_wkt4))
})
