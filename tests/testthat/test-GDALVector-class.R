# Tests for src/gdalvector.cpp
test_that("class constructors work", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn)
    expect_equal(lyr$getName(), "mtbs_perims")
    expect_type(lyr$getFeature(1), "list")
    expect_equal(length(lyr$getLayerDefn()) + 1, length(lyr$featureTemplate))
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

    unlink(dsn)
})

test_that("setting ignored fields works", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims")
    expect_true(lyr$testCapability()$IgnoreFields)
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    lyr$setIgnoredFields("event_id")
    feat <- lyr$getNextFeature()
    expect_length(feat, 9)
    expect_true(is.null(feat$event_id))
    expect_false(is.null(feat$incid_name))
    lyr$setIgnoredFields("")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    lyr$setIgnoredFields(c("event_id", "map_id", "ig_year"))
    feat <- lyr$getNextFeature()
    expect_length(feat, 7)
    lyr$setIgnoredFields("")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)

    # geometry
    lyr$returnGeomAs <- "WKT"
    feat <- lyr$getNextFeature()
    expect_length(feat, 11)
    lyr$setIgnoredFields("OGR_GEOMETRY")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    expect_true(is.null(feat$geom))
    lyr$setIgnoredFields(c("event_id", "OGR_GEOMETRY"))
    feat <- lyr$getNextFeature()
    expect_length(feat, 9)
    lyr$setIgnoredFields("")
    feat <- lyr$getNextFeature()
    expect_length(feat, 11)

    lyr$close()
    unlink(dsn)
})

test_that("cursor positioning works correctly", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims")

    expect_equal(lyr$getFeatureCount(), 61)
    expect_equal(lyr$getNextFeature()$FID, bit64::as.integer64(1))
    expect_equal(lyr$getNextFeature()$FID, bit64::as.integer64(2))
    lyr$resetReading()
    expect_equal(lyr$getNextFeature()$FID, bit64::as.integer64(1))
    lyr$setNextByIndex(3)
    expect_equal(lyr$getNextFeature()$FID, bit64::as.integer64(4))
    lyr$setNextByIndex(3.5)
    expect_equal(lyr$getNextFeature()$FID, bit64::as.integer64(4))
    lyr$setNextByIndex(0)
    expect_equal(lyr$getNextFeature()$FID, bit64::as.integer64(1))
    expect_equal(lyr$getFeature(10)$FID, bit64::as.integer64(10))
    lyr$setNextByIndex(61)
    expect_true(is.null(lyr$getNextFeature()))

    expect_error(lyr$setNextByIndex(NA))
    expect_error(lyr$setNextByIndex(-1))
    expect_error(lyr$setNextByIndex(Inf))
    expect_error(lyr$setNextByIndex(9007199254740993))

    lyr$close()

    unlink(dsn)
})

test_that("delete feature works", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims", read_only = FALSE)
    num_feat <- lyr$getFeatureCount()
    expect_true(lyr$deleteFeature(1))

    lyr$open(read_only = TRUE)
    expect_equal(lyr$getFeatureCount(), num_feat - 1)
    expect_false(lyr$deleteFeature(2))
    expect_equal(lyr$getFeatureCount(), num_feat - 1)

    lyr$close()

    unlink(dsn)
})
