# Tests for src/gdalvector.cpp
test_that("class constructors work", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- paste0("/vsimem/", basename(f))
    vsi_copy_file(f, dsn)

    lyr <- new(GDALVector, dsn)
    expect_equal(lyr$getName(), "mtbs_perims")
    expect_type(lyr$getFeature(1), "list")
    lyr$close()

    lyr <- new(GDALVector, dsn, "mtbs_perims")
    expect_equal(lyr$bbox(), c(469685.73, -12917.76, 573531.72, 96577.34))
    lyr$close()

    lyr <- new(GDALVector, dsn, "mtbs_perims", read_only = FALSE)
    expect_true(lyr$testCapability()$RandomWrite)
    lyr$close()

    lyr <- new(GDALVector, dsn, "mtbs_perims", read_only = TRUE,
               "LIST_ALL_TABLES=NO")
    expect_false(lyr$testCapability()$RandomWrite)
    lyr$close()

    bb <- c(469685.97, 11442.45, 544069.63, 85508.15)

    # spatial filter with SQL layer
    sql <- "SELECT FID, * FROM mtbs_perims"
    lyr <- new(GDALVector, dsn, sql, read_only = TRUE, open_options = NULL,
               spatial_filter = bbox_to_wkt(bb))
    expect_equal(lyr$getFeatureCount(), 40)
    lyr$close()

    # add dialect
    lyr <- new(GDALVector, dsn, sql, read_only = TRUE, open_options = NULL,
               spatial_filter = bbox_to_wkt(bb), dialect = "")
    expect_equal(lyr$getFeatureCount(), 40)
    lyr$close()

    vsi_unlink(dsn)
})
