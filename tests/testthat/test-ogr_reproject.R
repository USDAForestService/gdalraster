test_that("ogr_reproject works", {
    f <- system.file("extdata/ynp_features.zip", package = "gdalraster")
    ynp_dsn <- file.path("/vsizip", f, "ynp_features.gpkg")

    out_gpkg <- tempfile(fileext = ".gpkg")
    on.exit(unlink(out_gpkg))
    ## create a new output dsn, append to existing, overwrite
    # create new
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, "ynp_bnd",
                                         out_gpkg, "EPSG:32100"))
    expect_equal(lyr$getFeatureCount(), 1)
    lyr$close()
    # append or overwrite required for existing layer
    expect_error(lyr <- ogr_reproject(ynp_dsn, "ynp_bnd",
                                      out_gpkg, "EPSG:32100"))
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, "ynp_bnd",
                                         out_gpkg, "EPSG:32100",
                                         append = TRUE))
    expect_equal(lyr$getFeatureCount(), 2)
    lyr$close()
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, "ynp_bnd",
                                         out_gpkg, "EPSG:32100",
                                         overwrite = TRUE))
    expect_equal(lyr$getFeatureCount(), 1)
    lyr$close()

    ## spat_bbox
    bb <- c(-111.18, 44.78, -111.03, 45.07)
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, "roads",
                                         out_gpkg, "EPSG:32100",
                                         spat_bbox = bb))
    expect_equal(lyr$getFeatureCount(), 11)
    lyr$close()

    ## add_cl_arg
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, "points_of_interest",
                                         out_gpkg, "EPSG:32100",
                                         add_cl_arg = c("-limit", "10")))
    expect_equal(lyr$getFeatureCount(), 10)
    lyr$close()

    ## output to shapefile
    out_shp <- tempfile(fileext = ".shp")
    on.exit(deleteDataset(out_shp))
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, "ynp_bnd",
                                         out_shp, "EPSG:32100"))
    expect_equal(lyr$getFeatureCount(), 1)
    lyr$close()
    expect_error(lyr <- ogr_reproject(ynp_dsn, "ynp_bnd",
                                      out_shp, "EPSG:32100"))
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, "ynp_bnd",
                                         out_shp, "EPSG:32100",
                                         append = TRUE))
    expect_equal(lyr$getFeatureCount(), 2)
    lyr$close()
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, "ynp_bnd",
                                         out_shp, "EPSG:32100",
                                         overwrite = TRUE))
    expect_equal(lyr$getFeatureCount(), 1)
    lyr$close()

    ## SQL layer with output to GeoJSON WGS84
    out_json <- tempfile(fileext = ".geojson")
    on.exit(unlink(out_json))
    sql <- "SELECT poiname, geom FROM points_of_interest
            WHERE poitype = 'Ranger Station'"
    # nln required for SQL layer
    expect_error(lyr <- ogr_reproject(ynp_dsn, sql,
                                      out_json, "WGS84"))
    expect_no_error(lyr <- ogr_reproject(ynp_dsn, sql,
                                         out_json, "WGS84",
                                         nln = "ynp_ranger_stations"))
    expect_equal(lyr$getFeatureCount(), 14)
    feat <- lyr$getNextFeature()
    expect_equal(g_coords(feat$geom)[1, "x"], -110.7005, tolerance = 1e-3)
    expect_equal(g_coords(feat$geom)[1, "y"], 44.97703, tolerance = 1e-3)
    lyr$close()
    # with dialect
    if (has_spatialite()) {
        sql <- "SELECT poiname, ST_X(geometry) as x, ST_Y(geometry) as y
                FROM ynp_ranger_stations"
        # reading back from the GeoJSON file, no change in coordinate system
        expect_no_error(lyr <- ogr_reproject(out_json, sql,
                                             out_gpkg, "WGS84",
                                             nln = "ynp_ranger_stations",
                                             dialect = "SQLite"))
        expect_equal(lyr$getFeatureCount(), 14)
        feat <- lyr$getNextFeature()
        expect_equal(feat$x, -110.7005, tolerance = 1e-3)
        expect_equal(feat$y, 44.97703, tolerance = 1e-3)
        lyr$close()
    }

    ## shapefile source
    shp_dsn <- system.file("extdata/poly_multipoly.shp", package="gdalraster")
    expect_no_error(lyr <- ogr_reproject(shp_dsn, "", out_gpkg, "EPSG:32100",
                                         nlt = "PROMOTE_TO_MULTI"))
    expect_equal(lyr$getName(), "poly_multipoly")
    expect_equal(lyr$getFeatureCount(), 4)
    expect_equal(lyr$getGeomType(), "MULTIPOLYGON")
    expect_true(srs_is_same(lyr$getSpatialRef(), "EPSG:32100"))
    lyr$close()


    ## errors
    out_gpkg2 <- tempfile(fileext = ".gpkg")
    on.exit(unlink(out_gpkg2))
    expect_error(ogr_reproject(c(ynp_dsn, shp_dsn), "ynp_bnd",
                               out_gpkg2, "EPSG:32100"))
    expect_error(ogr_reproject(ynp_dsn, c("ynp_bnd", "roads"),
                               out_gpkg2, "EPSG:32100"))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               NULL, "EPSG:32100"))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg2, NULL))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               out_fmt = 1))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               overwrite = NULL))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               append = NULL))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               nln = 1))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               dsco = 1))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               lco = 1))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               dialect = 1))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               spat_bbox = 1))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               src_open_options = 1))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               progress = 1))
    expect_error(ogr_reproject(ynp_dsn, "ynp_bnd",
                               out_gpkg, "EPSG:32100",
                               add_cl_arg = 1))

})
