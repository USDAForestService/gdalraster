test_that("ogr_proc works", {
    dsn <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")

    lyr <- new(GDALVector, dsn, "mtbs_perims")
    lyr$setAttributeFilter("ig_year >= 2000")

    lyr2 <- new(GDALVector, dsn, "mtbs_perims")
    lyr2$setAttributeFilter("incid_name = 'NORTH FORK'")

    dsn_tmp <- tempfile(fileext = ".gpkg")

    opt <- c("INPUT_PREFIX=input_",
             "METHOD_PREFIX=method_",
             "PROMOTE_TO_MULTI=YES")

    # intersection
    test_lyr_name <- "intersect_test"

    expect_no_error(lyr_out <- ogr_proc(mode = "Intersection",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt))

    expect_true(is(lyr_out, "Rcpp_GDALVector"))
    expect_equal(lyr_out$getGeomType(), "MULTIPOLYGON")
    expect_true(srs_is_same(lyr$getSpatialRef(), lyr_out$getSpatialRef()))
    lyr_out$returnGeomAs <- "WKT"
    d <- lyr_out$fetch(-1)
    expect_equal(nrow(d), 5)
    expect_equal(ncol(d), 20)
    expect_equal(min(d$input_ig_date), as.Date("2010-09-14"))
    expect_equal(max(d$input_ig_date), as.Date("2016-08-26"))
    # burn_bnd_ac is an int64 field
    expect_equal(sum(d$input_burn_bnd_ac), bit64::as.integer64(116770))
    expect_equal(unique(d$method_incid_name), "NORTH FORK")
    expect_equal(unique(d$method_ig_year), 1988)
    expect_equal(sum(sapply(d$geom, g_area)), 223994493)

    lyr_out$close()
    rm(lyr_out)
    rm(d)

    # append
    expect_no_error(lyr_out <- ogr_proc(mode = "Intersection",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt))

    lyr_out$returnGeomAs <- "WKT"
    d <- lyr_out$fetch(-1)
    expect_equal(nrow(d), 10)
    expect_equal(ncol(d), 20)
    expect_equal(sum(sapply(d$geom, g_area)), 223994493 * 2)

    lyr_out$close()
    rm(lyr_out)
    rm(d)

    # overwrite
    expect_no_error(lyr_out <- ogr_proc(mode = "Intersection",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt,
                                        overwrite = TRUE))

    lyr_out$returnGeomAs <- "WKT"
    d <- lyr_out$fetch(-1)
    expect_equal(nrow(d), 5)
    expect_equal(ncol(d), 20)
    expect_equal(sum(sapply(d$geom, g_area)), 223994493)

    lyr_out$close()
    rm(lyr_out)
    rm(d)

    # union
    test_lyr_name <- "union_test"
    expect_no_error(lyr_out <- ogr_proc(mode = "Union",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt))

    expect_equal(lyr_out$getFeatureCount(), 46)
    lyr_out$close()
    rm(lyr_out)

    # symdifference
    test_lyr_name <- "symdiff_test"
    expect_no_error(lyr_out <- ogr_proc(mode = "SymDifference",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt))

    expect_equal(lyr_out$getFeatureCount(), 41)
    lyr_out$close()
    rm(lyr_out)

    # erase
    test_lyr_name <- "erase_test"
    expect_no_error(lyr_out <- ogr_proc(mode = "Erase",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt))

    expect_equal(lyr_out$getFeatureCount(), 40)
    lyr_out$close()
    rm(lyr_out)

    # clip
    test_lyr_name <- "clip_test"
    expect_no_error(lyr_out <- ogr_proc(mode = "Clip",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt))

    expect_equal(lyr_out$getFeatureCount(), 5)
    lyr_out$close()
    rm(lyr_out)

    # identity
    test_lyr_name <- "identity_test"
    expect_no_error(lyr_out <- ogr_proc(mode = "Identity",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt))

    expect_equal(lyr_out$getFeatureCount(), 45)
    lyr_out$close()
    rm(lyr_out)

    # update
    test_lyr_name <- "update_test"
    expect_no_error(lyr_out <- ogr_proc(mode = "Update",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = dsn_tmp,
                                        out_lyr_name = test_lyr_name,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = "PROMOTE_TO_MULTI=YES"))

    expect_equal(lyr_out$getFeatureCount(), 41)
    lyr_out$close()
    rm(lyr_out)

    # shapefile output
    shp_tmp <- tempfile(fileext = ".shp")
    expect_no_error(lyr_out <- ogr_proc(mode = "Intersection",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = shp_tmp,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt))

    # SHPT_POLYGON shapefiles, reported as layers of type wkbPolygon
    expect_equal(lyr_out$getGeomType(), "POLYGON")
    expect_true(srs_is_same(lyr$getSpatialRef(), lyr_out$getSpatialRef()))
    lyr_out$returnGeomAs <- "WKT"
    d <- lyr_out$fetch(-1)
    expect_equal(nrow(d), 5)
    expect_equal(ncol(d), 20)
    expect_equal(sum(sapply(d$geom, g_area)), 223994493)

    lyr_out$close()
    rm(lyr_out)
    rm(d)

    # overwrite shapefile
    expect_no_error(lyr_out <- ogr_proc(mode = "Intersection",
                                        input_lyr = lyr,
                                        method_lyr = lyr2,
                                        out_dsn = shp_tmp,
                                        out_geom_type = "MULTIPOLYGON",
                                        mode_opt = opt,
                                        overwrite = TRUE))

    lyr_out$returnGeomAs <- "WKT"
    d <- lyr_out$fetch(-1)
    expect_equal(nrow(d), 5)
    expect_equal(ncol(d), 20)
    expect_equal(sum(sapply(d$geom, g_area)), 223994493)

    lyr_out$close()
    rm(lyr_out)
    rm(d)

    lyr$close()
    lyr2$close()
    deleteDataset(dsn_tmp)
    deleteDataset(shp_tmp)
})
