test_that("geos_version returns a list of length 4", {
    expect_length(geos_version(), 4)
})

test_that("geom functions work on wkb/wkt geometries", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file, read_only=TRUE)
    bb <- bbox_to_wkt(ds$bbox())
    ds$close()
    expect_true(g_is_valid(bb))

    invalid_bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
    5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
    325298.1 5104929.4))"
    expect_false(g_is_valid(invalid_bnd))

    bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
    5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
    325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
    expect_true(g_is_valid(bnd))

    x <- c(324467.3, 323909.4, 323794.2)
    y <- c(5104814.2, 5104365.4, 5103455.8)
    pt_xy <- cbind(x, y)
    expect_error(g_create("POLYGON", pt_xy))

    if (gdal_version_num() >= 3050000) {
        # OGR_GEOMETRY_ACCEPT_UNCLOSED_RING config option added at 3.5.0
        x <- c(324467.3, 323909.4, 323794.2, 324970.7)
        y <- c(5104814.2, 5104365.4, 5103455.8, 5102885.8)
        pt_xy <- cbind(x, y)
        expect_error(g_create("POLYGON", pt_xy))
    }

    x <- c(324467.3, 323909.4, 323794.2, 324970.7, 326420.0, 326389.6,
           325298.1, 325298.1, 324467.3)
    y <- c(5104814.2, 5104365.4, 5103455.8, 5102885.8, 5103595.3, 5104747.5,
           5104929.4, 5104929.4, 5104814.2)
    pt_xy <- cbind(x, y)

    expect_true(g_create("POLYGON", pt_xy) |> g_is_valid())
    expect_true((g_create("POLYGON", pt_xy) |> g_area()) > 0)

    pt_xy <- c(324171, 5103034.3)
    expect_error(g_create("POLYGON", pt_xy))
    pt <- g_create("POINT", pt_xy, as_wkb = FALSE)

    line_xy <- matrix(c(324171, 327711.7, 5103034.3, 5104475.9),
                      nrow=2, ncol=2)
    # expect_error(g_create("POINT", line_xy))
    # now returns multiple POINT geoms:
    expect_equal(length(g_create("POINT", line_xy)), 2)
    expect_equal(g_create("POINT", line_xy) |> g_name(), c("POINT", "POINT"))
    line <- g_create("LINESTRING", line_xy, as_wkb = FALSE)

    # set up vector of WKT, WKB raw vector, and list of WKB raw vectors
    wkt_vec_1 <- c(bb, bnd)
    wkt_vec_2 <- c(pt, pt)

    bb_wkb <- g_wk2wk(bb)
    expect_true(is.raw(bb_wkb))
    pt_wkb <- g_wk2wk(pt)
    expect_true(is.raw(pt_wkb))

    wkb_list_1 <- g_wk2wk(wkt_vec_1)
    expect_true(is.list(wkb_list_1) && is.raw(wkb_list_1[[1]]))
    wkb_list_2 <- g_wk2wk(wkt_vec_2)
    expect_true(is.list(wkb_list_2) && is.raw(wkb_list_2[[1]]))

    # binary predicates
    # WKT
    expect_true(g_intersects(bb, pt))
    expect_false(g_intersects(bnd, pt))
    expect_error(g_intersects("invalid WKT", pt))
    expect_error(g_intersects(bnd, "invalid WKT"))
    # vector of WKT
    expected_value <- c(TRUE, FALSE)
    expect_equal(g_intersects(wkt_vec_1, wkt_vec_2), expected_value)
    # WKB
    expect_true(g_intersects(bb_wkb, pt_wkb))
    # list of WKB
    expect_equal(g_intersects(wkb_list_1, wkb_list_2), expected_value)

    # WKT
    expect_false(g_equals(bb, bnd))
    expect_error(g_equals("invalid WKT", bnd))
    expect_error(g_equals(bb, "invalid WKT"))
    # vector of WKT
    expected_value <- c(FALSE, FALSE)
    expect_equal(g_equals(wkt_vec_1, wkt_vec_2), expected_value)
    # WKB
    expect_false(g_equals(bb_wkb, pt_wkb))
    # list of WKB
    expect_equal(g_equals(wkb_list_1, wkb_list_2), expected_value)

    # WKT
    expect_false(g_disjoint(bb, bnd))
    expect_error(g_disjoint("invalid WKT", bnd))
    expect_error(g_disjoint(bb, "invalid WKT"))
    # vector of WKT
    expected_value <- c(FALSE, TRUE)
    expect_equal(g_disjoint(wkt_vec_1, wkt_vec_2), expected_value)
    # WKB
    expect_false(g_disjoint(bb_wkb, pt_wkb))
    # list of WKB
    expect_equal(g_disjoint(wkb_list_1, wkb_list_2), expected_value)

    # WKT
    expect_false(g_touches(bb, bnd))
    expect_error(g_touches("invalid WKT", bnd))
    expect_error(g_touches(bb, "invalid WKT"))
    # vector of WKT
    expected_value <- c(FALSE, FALSE)
    expect_equal(g_touches(wkt_vec_1, wkt_vec_2), expected_value)
    # WKB
    expect_false(g_touches(bb_wkb, pt_wkb))
    # list of WKB
    expect_equal(g_touches(wkb_list_1, wkb_list_2), expected_value)

    # WKT
    expect_true(g_contains(bb, bnd))
    expect_error(g_contains("invalid WKT", bnd))
    expect_error(g_contains(bb, "invalid WKT"))
    # vector of WKT
    expected_value <- c(TRUE, FALSE)
    expect_equal(g_contains(wkt_vec_1, wkt_vec_2), expected_value)
    # WKB
    expect_true(g_contains(bb_wkb, pt_wkb))
    # list of WKB
    expect_equal(g_contains(wkb_list_1, wkb_list_2), expected_value)

    # WKT
    expect_false(g_within(bb, bnd))
    expect_error(g_within("invalid WKT", bnd))
    expect_error(g_within(bb, "invalid WKT"))
    # vector of WKT
    expected_value <- c(FALSE, FALSE)
    expect_equal(g_within(wkt_vec_1, wkt_vec_2), expected_value)
    # WKB
    expect_false(g_within(bb_wkb, pt_wkb))
    # list of WKB
    expect_equal(g_within(wkb_list_1, wkb_list_2), expected_value)

    # WKT
    expect_true(g_crosses(line, bnd))
    expect_error(g_crosses("invalid WKT", bnd))
    expect_error(g_crosses(line, "invalid WKT"))
    expect_false(g_crosses(line, bb))
    # vector of WKT
    expected_value <- c(FALSE, FALSE)
    expect_equal(g_crosses(wkt_vec_1, wkt_vec_2), expected_value)
    # WKB
    expect_false(g_crosses(bb_wkb, pt_wkb))
    # list of WKB
    expect_equal(g_crosses(wkb_list_1, wkb_list_2), expected_value)

    # WKT
    expect_false(g_overlaps(bb, bnd))
    expect_error(g_overlaps("invalid WKT", bnd))
    expect_error(g_overlaps(bb, "invalid WKT"))
    # vector of WKT
    expected_value <- c(FALSE, FALSE)
    expect_equal(g_overlaps(wkt_vec_1, wkt_vec_2), expected_value)
    # WKB
    expect_false(g_overlaps(bb_wkb, pt_wkb))
    # list of WKB
    expect_equal(g_overlaps(wkb_list_1, wkb_list_2), expected_value)

    # buffer
    expect_equal(round(bbox_from_wkt(g_buffer(bnd, 100, as_wkb = FALSE))),
                 round(c(323694.2, 5102785.8, 326520.0, 5105029.4)))
    expect_error(g_buffer("invalid WKT", 100))
    # vector of WKT
    res <- g_buffer(wkt_vec_1, 100)
    expect_true(is.list(res) && length(res) == 2 && is.raw(res[[1]]))
    # WKB
    expect_true(is.raw(g_buffer(pt_wkb, 10)))
    # list of WKB
    res <- g_buffer(wkb_list_1, 100)
    expect_true(is.list(res) && length(res) == 2 && is.raw(res[[1]]))

    # area
    expect_equal(round(g_area(bnd)), 4039645)
    expect_equal(g_area(g_intersection(bb, bnd, as_wkb = FALSE)), g_area(bnd))
    expect_equal(g_area(g_union(bb, bnd, as_wkb = FALSE)), g_area(bb))
    expect_equal(round(g_area(g_difference(bb, bnd, as_wkb = FALSE))),
                 9731255)
    expect_equal(round(g_area(g_sym_difference(bb, bnd, as_wkb = FALSE))),
                 9731255)
    expect_error(g_area("invalid WKT"))
    # vector of WKT
    res <- g_area(wkt_vec_1)
    expect_vector(res, numeric(), size = 2)
    expect_equal(round(res[2]), 4039645)
    # WKB
    expect_equal(g_area(pt_wkb), 0)
    # list of WKB
    res <- g_area(wkb_list_1)
    expect_vector(res, numeric(), size = 2)
    expect_equal(round(res[2]), 4039645)

    # distance
    expect_equal(g_distance(pt, bnd), 215.0365, tolerance = 1e-4)
    expect_error(g_distance("invalid WKT"))
    # vector of WKT
    res <- g_distance(wkt_vec_1, wkt_vec_2)
    expect_equal(res[2], 215.0365, tolerance = 1e-4)
    # WKB
    expect_equal(g_distance(pt_wkb, bb_wkb), 0)
    # list of WKB
    res <- g_distance(wkb_list_1, wkb_list_2)
    expect_equal(res[2], 215.0365, tolerance = 1e-4)

    # length
    expect_equal(g_length(line), 3822.927, tolerance = 1e-2)
    expect_error(g_length("invalid WKT"))
    # vector of WKT
    res <- g_length(wkt_vec_2)
    expect_equal(res[1], 0)
    # WKB
    expect_equal(g_length(pt_wkb), 0)
    # list of WKB
    res <- g_length(wkb_list_2)
    expect_equal(res[1], 0)

    # centroid
    res <- g_centroid(bnd)
    expect_equal(names(res), c("x", "y"))
    names(res) <- NULL
    expect_equal(res, c(325134.9, 5103985.4), tolerance = 0.1)
    expect_error(g_centroid("invalid WKT"))
    # vector of WKT
    res <- g_centroid(wkt_vec_1)
    expect_true(is.matrix(res) && ncol(res) == 2 && nrow(res) == 2)
    expect_equal(colnames(res), c("x", "y"))
    colnames(res) <- NULL
    expect_equal(res[2, ], c(325134.9, 5103985.4), tolerance = 0.1)
    # WKB
    expect_equal(g_centroid(bb_wkb), g_centroid(bb))
    # list of WKB
    res <- g_centroid(wkb_list_1)
    expect_true(is.matrix(res) && ncol(res) == 2 && nrow(res) == 2)
    expect_equal(colnames(res), c("x", "y"))
    colnames(res) <- NULL
    expect_equal(res[2, ], c(325134.9, 5103985.4), tolerance = 0.1)

    # binary ops with invalid WKT
    expect_error(g_intersection("invalid WKT", bnd, as_wkb = FALSE))
    expect_error(g_intersection(bb, "invalid WKT", as_wkb = FALSE))
    expect_error(g_union("invalid WKT", bnd, as_wkb = FALSE))
    expect_error(g_union(bb, "invalid WKT", as_wkb = FALSE))
    expect_error(g_difference("invalid WKT", bnd, as_wkb = FALSE))
    expect_error(g_difference(bb, "invalid WKT", as_wkb = FALSE))
    expect_error(g_sym_difference("invalid WKT", bnd, as_wkb = FALSE))
    expect_error(g_sym_difference(bb, "invalid WKT", as_wkb = FALSE))
})

test_that("g_factory functions work", {
    ## create
    # polygon
    pts1 <- c(0, 0, 10, 10, 10, 0, 0, 0)
    pts1 <-matrix(pts1, ncol = 2, byrow = TRUE)
    # poly1 <- "POLYGON((0 0, 10 10, 10 0, 0 0))"
    poly1 <- g_create("POLYGON", pts1)
    pts2 <- c(0, 0, 0, 10, 10, 0, 0, 0)
    pts2 <-matrix(pts2, ncol = 2, byrow = TRUE)
    # poly2 <- "POLYGON((0 0, 0 10, 10 0, 0 0))"
    poly2 <- g_create("POLYGON", pts2)
    res = g_intersection(poly1, poly2)
    expected <- "POLYGON ((0 0,5 5,10 0,0 0))"
    empty_expected <- g_difference(res, expected)
    expect_true(g_is_empty(empty_expected))

    # linestring
    pts <- c(9, 1, 12, 4)
    pts <-matrix(pts, ncol = 2, byrow = TRUE)
    line1 <- g_create("LINESTRING", pts)
    expect_true(g_intersects(line1, poly1))
    # xyz
    pts <- c(9, 1, 0, 12, 4, 10)
    pts <-matrix(pts, ncol = 3, byrow = TRUE)
    line2 <- g_create("LINESTRING", pts)
    coords <- g_coords(line2)
    expect_equal(coords$x, pts[, 1])
    expect_equal(coords$y, pts[, 2])
    expect_equal(coords$z, pts[, 3])
    # xyzm
    pts <- c(9, 1, 0, 2000, 12, 4, 10, 2000)
    pts <-matrix(pts, ncol = 4, byrow = TRUE)
    line3 <- g_create("LINESTRING", pts)
    coords <- g_coords(line3)
    expect_equal(coords$x, pts[, 1])
    expect_equal(coords$y, pts[, 2])
    expect_equal(coords$z, pts[, 3])
    expect_equal(coords$m, pts[, 4])

    # point
    pt1 <- g_create("POINT", c(9, 1))
    expect_true(g_within(pt1, poly1))
    pt2 <- g_create("POINT", c(1, 9))
    expect_false(g_within(pt2, poly1))
    # xyz
    pt3 <- g_create("POINT", c(1, 9, 0))
    coords <- g_coords(pt3)
    expect_equal(coords$x, 1)
    expect_equal(coords$y, 9)
    expect_equal(coords$z, 0)
    # xyzm
    pt4 <- g_create("POINT", c(1, 9, 0, 2000))
    coords <- g_coords(pt4)
    expect_equal(coords$x, 1)
    expect_equal(coords$y, 9)
    expect_equal(coords$z, 0)
    expect_equal(coords$m, 2000)

    # multipoint
    pts <- c(9, 1, 1, 9)
    pts <-matrix(pts, ncol = 2, byrow = TRUE)
    mult_pt1 <- g_create("MULTIPOINT", pts)
    expect_false(g_within(mult_pt1, poly1))
    # multipoint xyz
    x <- c(9, 1)
    y <- c(1, 9)
    z <- c(0, 10)
    pts <- cbind(x, y, z)
    mult_pt_xyz <- g_create("MULTIPOINT", pts)
    expect_equal(g_name(mult_pt_xyz), "MULTIPOINT")
    coords <- g_coords(mult_pt_xyz)
    expect_equal(coords$x, x)
    expect_equal(coords$y, y)
    expect_equal(coords$z, z)
    # multipoint xyzm
    m <- c(2000, 2000)
    pts <- cbind(x, y, z, m)
    mult_pt_xyzm <- g_create("MULTIPOINT", pts)
    expect_equal(g_name(mult_pt_xyzm), "MULTIPOINT")
    coords <- g_coords(mult_pt_xyzm)
    expect_equal(coords$x, x)
    expect_equal(coords$y, y)
    expect_equal(coords$z, z)
    expect_equal(coords$m, m)

    # geometry collection
    g_coll <- g_create("GEOMETRYCOLLECTION")
    expect_equal(g_wk2wk(g_coll), "GEOMETRYCOLLECTION EMPTY")

    ## add subgeometry to container
    # create empty multipoint and add geoms
    mult_pt2 <- g_create("MULTIPOINT")
    expect_no_error(mult_pt2 <- g_add_geom(g_create("POINT", c(9, 1)),
                                           mult_pt2))
    expect_no_error(mult_pt2 <- g_add_geom(g_create("POINT", c(1, 9)),
                                           mult_pt2))
    expect_true(g_equals(mult_pt1, mult_pt2))  # mult_pt1 from above

    pt <- g_create("POINT", c(1, 2))
    expect_no_error(g_coll <- g_add_geom(pt, g_coll))  # g_coll from above
    expect_no_error(g_coll <- g_add_geom(mult_pt2, g_coll))
    expect_true(g_is_valid(g_coll))

    # polygon to polygon (add inner ring)
    container <- "POLYGON((0 0,0 10,10 10,0 0),(0.25 0.5,1 1.1,0.5 1,0.25 0.5))"
    # sub_geom <- "POLYGON((5.25 5.5,6 6.1,5.5 6,5.25 5.5))"
    v <- c(5.25, 5.5, 6, 6.1, 5.5, 6, 5.25, 5.5)
    pts <- matrix(v, nrow = 4, ncol = 2, byrow = TRUE)
    sub_geom <- g_create("POLYGON", pts)
    new_wkb <- g_add_geom(sub_geom, container)
    wkt_expect <- "POLYGON((0 0,0 10,10 10,0 0),(0.25 0.5,1.0 1.1,0.5 1.0,0.25 0.5),(5.25 5.5,6.0 6.1,5.5 6.0,5.25 5.5))"
    empty_expected <- g_difference(new_wkb, wkt_expect)
    expect_true(g_is_empty(empty_expected))
    # expect_true(g_equals(new_wkb, wkt_expect))

    # multipoint with an empty point inside
    geom <- "MULTIPOINT(0 1)"
    expect_no_error(g_add_geom(g_create("POINT"), geom))

    # multilinestring with an empty linestring inside
    geom <- "MULTILINESTRING((0 1,2 3,4 5,0 1), (0 1,2 3,4 5,0 1))"
    expect_no_error(g_add_geom(g_create("LINESTRING"), geom))
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

test_that("bbox functions work", {
    skip_if_not(has_geos())

    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file, read_only=TRUE)
    bb <- ds$bbox()
    ds$close()

    expect_equal(bbox_from_wkt("invalid WKT"), rep(NA_real_, 4))
    bb_wkt <- bbox_to_wkt(bb)
    expect_true(startsWith(bb_wkt, "POLYGON"))
    expect_equal(bbox_from_wkt(bb_wkt), bb)
    expect_error(bbox_to_wkt(c(0, 1)))
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
    ds_srs <- ds$getProjectionRef()
    ds_bbox <- ds$bbox()
    ds$close()

    bbox_wgs84 <- c(-113.28289, 46.04764, -113.22629, 46.07760)

    expect_error(g_transform("invalid WKT", ds_srs, epsg_to_wkt(4326)))
    expect_error(g_transform(bbox_to_wkt(ds_bbox), "invalid WKT",
                             epsg_to_wkt(4326)))
    expect_error(g_transform(bbox_to_wkt(ds_bbox), ds_srs,
                             "invalid WKT"))

    bbox_test <- bbox_to_wkt(ds_bbox) |>
                   g_transform(srs_from = ds_srs,
                               srs_to = epsg_to_wkt(4326),
                               as_wkb = FALSE) |>
                   bbox_from_wkt()

    expect_equal(bbox_test, bbox_wgs84, tolerance = 1e-4)

    pt_xy <- matrix(c(324171, 5103034.3), nrow=1, ncol=2)
    pt <- g_create("POINT", pt_xy, as_wkb = FALSE)
    wkt_vec <- c(bbox_to_wkt(ds_bbox), pt)
    wkb_list <- g_wk2wk(wkt_vec)
    expect_true(is.list(wkb_list) && is.raw(wkb_list[[1]]))

    # input vector of WKT strings
    res <- g_transform(wkt_vec, ds_srs, epsg_to_wkt(4326), as_wkb = FALSE)
    expect_vector(res, character(), size = 2)
    expect_equal(bbox_from_wkt(res[1]), bbox_wgs84, tolerance = 1e-4)

    # input list of WKB raw vectors
    res <- g_transform(wkb_list, ds_srs, epsg_to_wkt(4326))
    expect_true(is.list(res) &&
                length(res) == 2 &&
                is.raw(res[[1]]) &&
                is.raw(res[[2]]))
    expect_equal(g_wk2wk(res[[1]]) |> bbox_from_wkt(),
                 bbox_wgs84,
                 tolerance = 1e-4)

    # bbox_transform
    skip_if(gdal_version_num() < 3040000)

    bb <- c(-1405880.71737131, -1371213.7625429356,
            5405880.71737131, 5371213.762542935)
    res <- bbox_transform(bb, "EPSG:32661", "EPSG:4326")
    expected <- c(-180.0, 48.656, 180.0, 90.0)
    expect_equal(res, expected, tolerance = 1e-4)
})

test_that("geometry properties are correct", {
    g1 <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
    expect_true(g_is_valid(g1))
    g2 <- "POLYGON ((0 0, 10 10, 10 0, 0 1))"
    expect_false(g_is_valid(g2))

    expect_false(g_is_empty(g1))
    g3 <- "POLYGON EMPTY"
    expect_true(g_is_empty(g3))

    bb <- c(323476.1, 5101872.0,  327766.1, 5105082.0)
    expect_equal(bbox_to_wkt(bb) |> g_name(), "POLYGON")
    res <- bbox_to_wkt(bb) |> g_envelope()
    expect_equal(names(res), c("xmin", "xmax", "ymin", "ymax"))
    names(res) <- NULL
    expect_equal(res, bb)

    # input as vector of WKT / list of WKB
    wkt_vec <- c(g1, g2, g3)
    wkb_list <- g_wk2wk(wkt_vec)

    expected_value <- c(TRUE, FALSE, TRUE)
    expect_equal(g_is_valid(wkt_vec), expected_value)
    expect_equal(g_is_valid(wkb_list), expected_value)

    expected_value <- c(FALSE, FALSE, TRUE)
    expect_equal(g_is_empty(wkt_vec), expected_value)
    expect_equal(g_is_empty(wkb_list), expected_value)

    # 3D/measured
    # 2D
    pt1 <- g_create("POINT", c(1, 9))
    expect_false(g_is_3D(pt1))
    expect_false(g_is_measured(pt1))
    # xyz
    pt2 <- g_create("POINT", c(1, 9, 0))
    expect_true(g_is_3D(pt2))
    expect_false(g_is_measured(pt2))
    # xyzm
    pt3 <- g_create("POINT", c(1, 9, 0, 2000))
    expect_true(g_is_3D(pt3))
    expect_true(g_is_measured(pt3))

    # g_envelope
    bb1_2 <- c(0, 0, 10, 10)
    bb3 <- c(0, 0, 0, 0)
    bb_mat <- rbind(bb1_2, bb1_2, bb3)
    colnames(bb_mat) <- c("xmin", "xmax", "ymin", "ymax")
    dimnames(bb_mat) <- NULL
    names(bb1_2) <- c("xmin", "xmax", "ymin", "ymax")
    names(bb3) <- c("xmin", "xmax", "ymin", "ymax")
    expect_equal(g_envelope(g1), bb1_2)
    expect_equal(g_envelope(g2), bb1_2)  # same as g1 but g2 is invalid geom
    expect_equal(g_envelope(g3), bb3)
    wkt_vec <- c(g1, g2, g3)
    res <- g_envelope(wkt_vec)
    dimnames(res) <- NULL
    expect_equal(res, bb_mat)
    wkb_list <- g_wk2wk(wkt_vec)
    res <- g_envelope(wkb_list)
    dimnames(res) <- NULL
    expect_equal(res, bb_mat)


    skip_if(gdal_version_num() < 3070000 )

    g <- "MULTIPOLYGON (((10 0,0 0,5 5,10 0)),((10 10,5 5,0 10,10 10)))"
    expected_value <-
        "MULTIPOLYGON : 2 geometries: POLYGON : 4 points POLYGON : 4 points"
    expect_equal(g_summary(g), expected_value)

    expect_vector(g_summary(wkt_vec), character(), size = 3)
    expect_vector(g_summary(wkb_list), character(), size = 3)
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

    g1 <- g_intersection(bb, bnd_overlaps, as_wkb = FALSE)
    g2 <- g_union(bb, bnd_overlaps, as_wkb = FALSE)
    g3 <- g_sym_difference(bb, bnd_overlaps, as_wkb = FALSE)
    expect_equal(g_area(g3), g_area(g_difference(g2, g1, as_wkb = FALSE)),
                 tolerance = 0.01)
    expect_true(g_difference(bnd, bb, as_wkb = FALSE) |> g_is_empty())
    expect_false(g_difference(bnd_overlaps, bb, as_wkb = FALSE) |> g_is_empty())

    # binary ops on list input
    g1 <- g_wk2wk("POLYGON ((0 0, 10 10, 10 0, 0 0))")
    g2 <- g_wk2wk("POLYGON ((0 0, 5 5, 5 0, 0 0))")
    g3 <- g_wk2wk("POLYGON ((100 100, 110 110, 110 100, 100 100))")
    g_list1 <- list(g1, g1)
    g_list2 <- list(g2, g3)

    res <- g_intersection(g_list1, g_list2)
    expect_true(g_equals(res[[1]], "POLYGON ((5 5,5 0,0 0,5 5))"))
    expect_true(g_is_empty(res[[2]]))

    res <- g_union(g_list1, g_list2)
    expect_true(g_equals(res[[1]], "POLYGON ((5 5,10 10,10 0,5 0,0 0,5 5))"))
    expect_false(g_is_empty(res[[2]]))

    res <- g_difference(g_list1, g_list2)
    expect_true(g_equals(res[[1]], "POLYGON ((10 10,10 0,5 0,5 5,10 10))"))
    expect_false(g_is_empty(res[[2]]))

    res <- g_sym_difference(g_list1, g_list2)
    expect_true(g_equals(res[[1]], "POLYGON ((10 10,10 0,5 0,5 5,10 10))"))
    expect_equal(g_name(res[[2]]), "MULTIPOLYGON")
})

test_that("g_buffer returns correct values", {
    pt <- "POINT (0 0)"
    expect_error(g_buffer(wkt = "invalid WKT", dist = 10))
    bb <- bbox_from_wkt(g_buffer(pt, dist = 10, as_wkb = FALSE))
    expect_equal(bb, c(-10, -10,  10,  10))
})

test_that("g_simplify returns correct values", {
    g1 <- "LINESTRING(0 0,1 0,10 0)"
    g_simp <- g_simplify(g1, tolerance = 5, as_wkb = FALSE)
    g_expect <- "LINESTRING (0 0,10 0)"
    expect_equal(g_simp, g_expect)
    # wkb input
    g_simp <- g_simplify(g_wk2wk(g1), tolerance = 5, as_wkb = FALSE)
    expect_equal(g_simp, g_expect)
    # preserve_topology = FALSE
    g_simp <- g_simplify(g1, tolerance = 5, preserve_topology = FALSE,
                         as_wkb = FALSE)
    expect_equal(g_simp, g_expect)
    # vector/list input
    g_expect <- rep(g_expect, 2)
    g2 <- "LINESTRING(0 0,1 1,10 0)"
    # character vector of wkt input
    g_simp <- g_simplify(c(g1, g2), tolerance = 5, as_wkb = FALSE)
    expect_equal(g_simp, g_expect)
    # list of wkb input
    g_simp <- g_simplify(g_wk2wk(c(g1, g2)), tolerance = 5, as_wkb = FALSE)
    expect_equal(g_simp, g_expect)
})

test_that("geometry measures are correct", {
    expect_equal(g_distance("POINT (0 0)", "POINT (5 12)"), 13)

    expect_equal(g_length("LINESTRING (0 0, 3 4)"), 5)

    bb <- c(323476.1, 5101872.0,  327766.1, 5105082.0)

    bb_area <- (bb[3] - bb[1]) * (bb[4] - bb[2])
    expect_equal(g_area(bbox_to_wkt(bb)), bb_area)

    res <- bbox_to_wkt(bb) |> g_centroid()
    names(res) <- NULL
    expect_equal(res, c(325621.1, 5103477.0))
})

test_that("geodesic measures are correct", {
    # tests based on gdal/autotest/ogr/ogr_geom.py
    # https://github.com/OSGeo/gdal/blob/28e94e2f52893d4206830011d75efe1783f1b7c1/autotest/ogr/ogr_geom.py
    skip_if(gdal_version_num() < 3090000)

    ## geodesic area
    # lon/lat order (traditional_gis_order = TRUE by default)
    g <- "POLYGON((2 49,3 49,3 48,2 49))"
    a <- g_geodesic_area(g, "EPSG:4326")
    expect_equal(a, 4068384291.8911743, tolerance = 1e4)

    g <- "POLYGON((2 89,3 89,3 88,2 89))"
    a <- g_geodesic_area(g, "EPSG:4326")
    expect_equal(a, 108860488.12023926, tolerance = 1e4)

    # lat/lon order
    g <- "POLYGON((49 2,49 3,48 3,49 2))"
    a <- g_geodesic_area(g, "EPSG:4326", traditional_gis_order = FALSE)
    expect_equal(a, 4068384291.8911743, tolerance = 1e4)

    # projected srs
    g <- "POLYGON((2 49,3 49,3 48,2 49))"
    g2 <- g_transform(g, "EPSG:4326", "EPSG:32631")
    a <- g_geodesic_area(g2, "EPSG:32631")
    expect_equal(a, 4068384291.8911743, tolerance = 1e4)
    # For comparison: cartesian area in UTM
    a <- g_area(g2)
    expect_equal(a, 4065070548.465351, tolerance = 1e4)
    # start with lat/lon order
    g <- "POLYGON((49 2,49 3,48 3,49 2))"
    g2 <- g_transform(g, "EPSG:4326", "EPSG:32631",
                      traditional_gis_order = FALSE)
    a <- g_geodesic_area(g2, "EPSG:32631")
    expect_equal(a, 4068384291.8911743, tolerance = 1e4)


    skip_if(gdal_version_num() < 3100000)

    ## geodesic length
    # lon/lat order (traditional_gis_order = TRUE by default)
    g <- "LINESTRING(2 49,3 49)"
    l <- g_geodesic_length(g, "EPSG:4326")
    expect_equal(l, 73171.26435678436, tolerance = 1e4)

    g <- "POLYGON((2 49,3 49,3 48,2 49))"
    l <- g_geodesic_length(g, "EPSG:4326")
    expect_equal(l, 317885.78639964823, tolerance = 1e4)

    # lat/lon order
    g <- "LINESTRING(49 3,48 3)"
    l <- g_geodesic_length(g, "EPSG:4326", traditional_gis_order = FALSE)
    expect_equal(l, 111200.0367623785, tolerance = 1e4)

    # projected srs
    g <- "POLYGON((2 49,3 49,3 48,2 49))"
    g2 <- g_transform(g, "EPSG:4326", "EPSG:32631")
    l <- g_geodesic_length(g2, "EPSG:32631")
    expect_equal(l, 317885.78639964823, tolerance = 1e4)
    # For comparison: cartesian length in UTM
    l <- g_length(g2)
    expect_equal(l, 317763.15996565996, tolerance = 1e4)
    # start with lat/lon order
    g <- "POLYGON((49 2,49 3,48 3,49 2))"
    g2 <- g_transform(g, "EPSG:4326", "EPSG:32631",
                      traditional_gis_order = FALSE)
    l <- g_geodesic_length(g2, "EPSG:32631")
    expect_equal(l, 317885.78639964823, tolerance = 1e4)
})

test_that("make_valid works", {
    # test only with recent GDAL and GEOS
    # these tests could give different results if used across a range of older
    # GDAL/GEOS versions, and the STRUCTURE method requires GEOS >= 3.10
    skip_if(gdal_version_num() < 3080000 ||
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
    expected_wkt3 <-
        "MULTIPOLYGON (((10 0,0 0,5 5,10 0)),((10 10,5 5,0 10,10 10)))"
    expect_true(g_equals(g_wk2wk(wkb3), expected_wkt3))

    # STRUCTURE method
    wkt4 <- "POLYGON ((0 0,0 10,10 10,10 0,0 0),(5 5,15 10,15 0,5 5))"
    expect_no_error(wkb4 <- g_make_valid(wkt4, method = "STRUCTURE"))
    expected_wkt4 <-
        "POLYGON ((0 10,10 10,10.0 7.5,5 5,10.0 2.5,10 0,0 0,0 10))"
    expect_true(g_equals(g_wk2wk(wkb4), expected_wkt4))

    # vector of WKT input
    wkt_vec <- c(wkt1, wkt2, wkt3, wkt4)
    expect_warning(wkb_list <- g_make_valid(wkt_vec, method = "STRUCTURE"))
    expect_equal(length(wkb_list), length(wkt_vec))
    expect_true(is.na(wkb_list[[2]]))
    expect_true(g_equals(g_wk2wk(wkb_list[[4]]), expected_wkt4))

    # list of WKB input
    rm(wkb_list)
    expect_warning(wkb_list <- g_make_valid(g_wk2wk(wkt_vec),
                                            method = "STRUCTURE"))
    expect_equal(length(wkb_list), length(wkt_vec))
    expect_true(is.na(wkb_list[[2]]))
    expect_true(g_equals(g_wk2wk(wkb_list[[4]]), expected_wkt4))
})

test_that("swap xy works", {
    g <- "GEOMETRYCOLLECTION(POINT(1 2),LINESTRING(1 2,2 3),POLYGON((0 0,0 1,1 1,0 0)))"
    g_swapped <- g_swap_xy(g, as_wkb = FALSE)
    g_expect <- "GEOMETRYCOLLECTION (POINT (2 1),LINESTRING (2 1,3 2),POLYGON ((0 0,1 0,1 1,0 0)))"
    expect_equal(g_swapped, g_expect)
    # wkb input
    g_swapped <- g_swap_xy(g_wk2wk(g), as_wkb = FALSE)
    expect_equal(g_swapped, g_expect)
    # vector/list input
    g1 <- "POINT(1 2)"
    g2 <- "POINT(2 3)"
    g_expect <- c("POINT (2 1)", "POINT (3 2)")
    # character vector of wkt input
    g_swapped <- g_swap_xy(c(g1, g2), as_wkb = FALSE)
    expect_equal(g_swapped, g_expect)
    # list of wkb input
    g_swapped <- g_swap_xy(g_wk2wk(c(g1, g2)), as_wkb = FALSE)
    expect_equal(g_swapped, g_expect)
})

test_that("g_coords returns a data frame of vertices", {
    m <- matrix(c(0, 3, 3, 4, 0, 0), ncol = 2, byrow = TRUE)
    pts_wkb <- lapply(seq_len(nrow(m)), function(i) g_create("POINT", m[i, ]))
    expect_true(is.list(pts_wkb) && length(pts_wkb) == 3)
    xy <- g_coords(pts_wkb)
    expect_equal(xy$x, c(0, 3, 0))
    expect_equal(xy$y, c(3, 4, 0))
    # one WKB raw vector not in a list
    expect_true(is.raw(pts_wkb[[1]]))
    xy <- g_coords(pts_wkb[[1]])
    expect_equal(xy$x, 0)
    expect_equal(xy$y, 3)
    # WKT input
    pts_wkt <- g_wk2wk(pts_wkb)
    expect_true(is.character(pts_wkt) && length(pts_wkt) == 3)
    xy <- g_coords(pts_wkt)
    expect_equal(xy$x, c(0, 3, 0))
    expect_equal(xy$y, c(3, 4, 0))

    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn)
    lyr <- new(GDALVector, dsn, "mtbs_perims")
    d <- lyr$fetch(1)
    vertices <- g_coords(d$geom)
    expect_true(is.data.frame(vertices))
    expect_equal(nrow(vertices), 36)
    lyr$close()
    unlink(dsn)
})
