# Tests for src/transform.cpp

test_that("transform/inv_project give correct results", {
    ## transform_xy
    pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
    pts <- read.csv(pt_file)
    xy_alb83 <- c(-1330885, -1331408, -1331994, -1330297, -1329991, -1329167,
                  -1329903, -1329432, -1327683, -1331265,  2684892,  2684660,
                   2685048,  2684967,  2683777,  2685212,  2685550,  2683821,
                   2685541,  2685514)

    xy_test <- transform_xy(pts = pts[,-1],
                            srs_from = epsg_to_wkt(26912),
                            srs_to = epsg_to_wkt(5070))
    expect_equal(as.vector(xy_test), xy_alb83, tolerance=0.1)

    # as matrix
    xy_test <- transform_xy(pts = as.matrix(pts[,-1]),
                            srs_from = epsg_to_wkt(26912),
                            srs_to = epsg_to_wkt(5070))
    expect_equal(as.vector(xy_test), xy_alb83, tolerance=0.1)

    # with NA
    pts[11, ] <- c(11, NA, NA)
    expect_warning(xy_test <- transform_xy(pts = pts[,-1],
                                           srs_from = epsg_to_wkt(26912),
                                           srs_to = epsg_to_wkt(5070)))
    expect_equal(as.vector(xy_test[1:10, ]), xy_alb83, tolerance=0.1)
    expect_true(is.na(xy_test[11, 1]) && is.na(xy_test[11, 2]))
    pts <- pts[-11, ]

    # as vector for one point
    xy_test <- transform_xy(pts = c(pts[1, 2], pts[1, 3]),
                            srs_from = epsg_to_wkt(26912),
                            srs_to = epsg_to_wkt(5070))
    expect_equal(as.vector(xy_test), c(xy_alb83[1], xy_alb83[11]),
                 tolerance=0.1)

    # as WKB/WKT geometries
    m <- as.matrix(pts[, -1])
    pts_wkb <- lapply(seq_len(nrow(m)), function(i) g_create("POINT", m[i, ]))
    pts_wkt <- g_wk2wk(pts_wkb)
    xy_test <- transform_xy(pts = pts_wkb,
                            srs_from = epsg_to_wkt(26912),
                            srs_to = epsg_to_wkt(5070))
    expect_equal(as.vector(xy_test), xy_alb83, tolerance=0.1)
    xy_test <- transform_xy(pts = pts_wkt,
                            srs_from = epsg_to_wkt(26912),
                            srs_to = epsg_to_wkt(5070))
    expect_equal(as.vector(xy_test), xy_alb83, tolerance=0.1)

    # errors
    expect_error(transform_xy(pts = pts[,-1],
                              srs_from = "invalid",
                              srs_to = epsg_to_wkt(5070)))
    expect_error(transform_xy(pts = pts[,-1],
                              srs_from = epsg_to_wkt(26912),
                              srs_to = "invalid"))
    # transform error
    pts[11, ] <- c(11, Inf, Inf)
    expect_warning(xy_test <- transform_xy(pts = pts[,-1],
                                           srs_from = epsg_to_wkt(26912),
                                           srs_to = epsg_to_wkt(5070)))
    expect_equal(as.vector(xy_test[1:10, ]), xy_alb83, tolerance=0.1)
    expect_true(is.na(xy_test[11, 1]) && is.na(xy_test[11, 2]))
    pts <- pts[-11, ]

    # invalid geometry type
    g <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
    expect_error(transform_xy(pts = g,
                              srs_from = epsg_to_wkt(26912),
                              srs_to = epsg_to_wkt(5070)))


    ## inv_project
    xy_wgs84 <- c(-113.26707, -113.27315, -113.28150, -113.25978, -113.25312,
                  -113.24600, -113.25613, -113.24613, -113.22794, -113.27334,
                  46.06118, 46.05827, 46.06076, 46.06280, 46.05276, 46.06682,
                  46.06862, 46.05405, 46.07214, 46.06607)

    inv_test <- inv_project(pts = pts[,-1],
                            srs = epsg_to_wkt(26912),
                            well_known_gcs = "WGS84")
    expect_equal(as.vector(inv_test), xy_wgs84, tolerance = 0.001)

    # as matrix
    inv_test <- inv_project(pts = as.matrix(pts[,-1]),
                            srs = epsg_to_wkt(26912),
                            well_known_gcs = "WGS84")
    expect_equal(as.vector(inv_test), xy_wgs84, tolerance = 0.001)

    # as vector for one point
    inv_test <- inv_project(pts = c(pts[1, 2], pts[1, 3]),
                            srs = epsg_to_wkt(26912),
                            well_known_gcs = "WGS84")
    expect_equal(as.vector(inv_test), c(xy_wgs84[1], xy_wgs84[11]),
                 tolerance=0.001)

    # as WKB/WKT geometries
    inv_test <- inv_project(pts = pts_wkb,
                            srs = epsg_to_wkt(26912),
                            well_known_gcs = "WGS84")
    expect_equal(as.vector(inv_test), xy_wgs84, tolerance = 0.001)
    inv_test <- inv_project(pts = pts_wkt,
                            srs = epsg_to_wkt(26912),
                            well_known_gcs = "WGS84")
    expect_equal(as.vector(inv_test), xy_wgs84, tolerance = 0.001)

    # warnings
    if (gdal_version_num() < gdal_compute_version(3, 11, 0)) {
        # behavior change at GDAL 3.11 https://github.com/OSGeo/gdal/pull/11819
        expect_warning(inv_project(pts = c(Inf, Inf),
                                   srs = epsg_to_wkt(26912),
                                   well_known_gcs = "WGS84"))
    }
    expect_warning(ret <- inv_project(matrix(c(Inf, -1330885, Inf, 2684892),
                                             ncol = 2),
                                      srs = epsg_to_wkt(26912),
                                      well_known_gcs = "WGS84"))
    expect_false(all(is.na(ret)))
    expect_true(any(is.na(ret)))

    # errors
    if (gdal_version_num() >= gdal_compute_version(3, 11, 0)) {
        # behavior change at GDAL 3.11 https://github.com/OSGeo/gdal/pull/11819
        expect_error(inv_project(pts = c(Inf, Inf),
                                 srs = epsg_to_wkt(26912),
                                 well_known_gcs = "WGS84"))
    }
    expect_error(inv_project(pts = as.vector(pts[,-1]),
                             srs = epsg_to_wkt(26912),
                             well_known_gcs = "WGS84"))
    expect_error(inv_project(pts = as.matrix(pts[,-1]),
                             srs = "invalid",
                             well_known_gcs = "WGS84"))
    expect_error(inv_project(pts = as.matrix(pts[,-1]),
                             srs = epsg_to_wkt(26912),
                             well_known_gcs = "invalid"))
    # transform error
    pts[11, ] <- c(11, Inf, Inf)
    expect_warning(inv_test <- inv_project(pts = pts[,-1],
                                           srs = epsg_to_wkt(26912),
                                           well_known_gcs = "WGS84"))
    expect_equal(as.vector(inv_test[1:10, ]), xy_wgs84, tolerance=0.001)
    expect_true(is.na(inv_test[11, 1]) && is.na(inv_test[11, 2]))
    pts <- pts[-11, ]

    # invalid geom type
    g <- "POLYGON ((0 0, 10 10, 10 0, 0 0))"
    expect_error(inv_project(pts = g,
                             srs = epsg_to_wkt(26912),
                             well_known_gcs = "WGS84"))


    ## with xyz/xyzt 3 or 4 column input
    # lon/lat to UTM zone 11N (EPSG:32611)
    m_xyz <- matrix(c(-117.5, 32.0, 0.0, -117.5, 32.0, 10.0),
                    ncol = 3, byrow = TRUE)

    res <- transform_xy(m_xyz, "WGS84", "EPSG:32611")
    expect_equal(nrow(res), 2)
    expect_equal(ncol(res), 3)
    expect_equal(res[2, 1], 452772.1, tolerance = 0.01)
    expect_equal(res[2, 2], 3540545, tolerance = 0.01)
    expect_equal(res[2, 3], 10)

    # inverse
    m_xyz <- matrix(c(452772.1, 3540545, 0.0, 452772.1, 3540545, 10.0),
                    ncol = 3, byrow = TRUE)
    res <- inv_project(m_xyz, "EPSG:32611", "WGS84")
    expect_equal(nrow(res), 2)
    expect_equal(ncol(res), 3)
    expect_equal(res[2, 1], -117.5, tolerance = 0.1)
    expect_equal(res[2, 2], 32.0, tolerance = 0.1)
    expect_equal(res[2, 3], 10)

    # with time values
    m_xyzt <- matrix(c(-117.5, 32.0, 0.0, 2000, -117.5, 32.0, 10.0, 2000),
                     ncol = 4, byrow = TRUE)

    res <- transform_xy(m_xyzt, "WGS84", "EPSG:32611")
    expect_equal(nrow(res), 2)
    expect_equal(ncol(res), 4)
    expect_equal(res[2, 1], 452772.1, tolerance = 0.01)
    expect_equal(res[2, 2], 3540545, tolerance = 0.01)
    expect_equal(res[2, 3], 10)
    expect_equal(res[2, 4], 2000)

    # inverse
    m_xyzt <- matrix(c(452772.1, 3540545, 0.0, 2000, 452772.1, 3540545, 10.0, 2000),
                     ncol = 4, byrow = TRUE)
    res <- inv_project(m_xyzt, "EPSG:32611", "WGS84")
    expect_equal(nrow(res), 2)
    expect_equal(ncol(res), 4)
    expect_equal(res[2, 1], -117.5, tolerance = 0.1)
    expect_equal(res[2, 2], 32.0, tolerance = 0.1)
    expect_equal(res[2, 3], 10)
    expect_equal(res[2, 4], 2000)
})

test_that("transform_bounds gives correct results", {
    skip_if(gdal_version_num() < 3040000)

    # south pole
    bb <- c(-1405880.717371, -1371213.762543, 5405880.717371, 5371213.762543)
    res <- transform_bounds(bb, "EPSG:32761", "EPSG:4326",
                            traditional_gis_order = FALSE)
    expected <- c(-90.0, -180.0, -48.65641, 180.0)
    expect_equal(res, expected, tolerance = 1e-4)
    # lon/lat axis ordering by default
    res <- transform_bounds(bb, "EPSG:32761", "EPSG:4326")
    expected <- c(-180.0, -90.0, 180.0, -48.65641)
    expect_equal(res, expected, tolerance = 1e-4)

    # antimeridian, normalized axis ordering
    bb <- c(160.6, -55.95, -171.2, -25.88)
    res <- transform_bounds(bb, "EPSG:4167", "EPSG:3851")
    expected <- c(1722483.900174921, 5228058.6143420935,
                  4624385.494808555, 8692574.544944234)
    expect_equal(res, expected, tolerance = 1e-4)
    # authority compliant axis ordering
    bb <- c(-55.95, 160.6, -25.88, -171.2)
    res <- transform_bounds(bb, "EPSG:4167", "EPSG:3851",
                            traditional_gis_order = FALSE)
    expected <- c(5228058.6143420935, 1722483.900174921,
                  8692574.544944234, 4624385.494808555)
    expect_equal(res, expected, tolerance = 1e-4)
})
