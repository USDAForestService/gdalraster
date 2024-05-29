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

    expect_true(srs_is_same(srs_to_wkt("EPSG:4326"),
                            srs_to_wkt("OGC:CRS84")))

    expect_true(srs_is_same(srs_to_wkt("EPSG:4326"),
                            srs_to_wkt("OGC:CRS84"),
                            ignore_axis_mapping=TRUE))

    expect_true(srs_is_same(srs_to_wkt("EPSG:4326"),
                            srs_to_wkt("OGC:CRS84"),
                            ignore_coord_epoch=TRUE))

    expect_false(srs_is_same(srs_to_wkt("EPSG:4326"),
                             srs_to_wkt("OGC:CRS84"),
                             ignore_axis_mapping=TRUE,
                             criterion="STRICT"))

    expect_equal(srs_to_wkt("NAD83"), epsg_to_wkt(4269))
    expect_equal(srs_to_wkt("NAD83", pretty=TRUE),
                 epsg_to_wkt(4269, pretty=TRUE))

    # errors
    expect_error(epsg_to_wkt(-1))
    expect_equal(srs_to_wkt(""), "")
    expect_error(srs_to_wkt("invalid"))
    expect_error(srs_is_geographic("invalid"))
    expect_error(srs_is_projected("invalid"))
    expect_error(srs_is_same("invalid", "invalid"))
    expect_error(srs_is_same(srs, "invalid"))
})

test_that("bbox functions work", {
    skip_if_not(has_geos())
    # geom/bbox functions are also tested in test-geos_wkt.R and test-geom.R

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
