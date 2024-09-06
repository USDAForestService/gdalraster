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

    # transaction
    lyr$open(read_only = FALSE)
    num_feat <- lyr$getFeatureCount()

    expect_false(lyr$commitTransaction())
    expect_false(lyr$rollbackTransaction())

    expect_true(lyr$startTransaction(force = FALSE))
    lyr$deleteFeature(10)
    expect_true(lyr$commitTransaction())
    expect_equal(lyr$getFeatureCount(), num_feat - 1)
    lyr$startTransaction(force = FALSE)
    lyr$deleteFeature(11)
    expect_true(lyr$rollbackTransaction())
    expect_equal(lyr$getFeatureCount(), num_feat - 1)

    lyr$close()
    unlink(dsn)
})

test_that("feature write methods work", {
    # initial test set for feature write
    # TODO: add tests for all OGR field types

    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims", read_only = FALSE)
    start_count <- lyr$getFeatureCount()
    lyr$returnGeomAs <- "WKB"

    # create and write a new feature
    # new feature is a modified copy of existing FID 1 with same geom
    feat <- lyr$getNextFeature()
    test1_orig_fid <- feat$FID
    feat$FID <- NULL
    feat$event_id <- "ZZ01"
    feat$incid_name <- "TEST 1"
    feat$map_id <- 999991
    feat$ig_date <- as.Date("9999-01-01")
    feat$ig_year <- 9999
    test1_fid <- lyr$createFeature(feat)
    expect_false(is.null(test1_fid))
    expect_equal(lyr$getFeatureCount(), start_count + 1)

    # edit an existing feature and set
    feat <- NULL
    feat <- lyr$getNextFeature()
    feat$event_id <- "ZZ02"
    feat$incid_name <- "TEST 2"
    feat$map_id <- 999992
    feat$ig_date <- as.Date("9999-01-02")
    feat$ig_year <- 9999
    test2_fid <- lyr$setFeature(feat)
    expect_false(is.null(test2_fid))
    expect_equal(lyr$getFeatureCount(), start_count + 1)

    if(.gdal_version_num() > 3060000) {
        # edit an existing feature and upsert with existing FID
        feat <- NULL
        feat <- lyr$getNextFeature()
        feat$event_id <- "ZZ03"
        feat$incid_name <- "TEST 3"
        feat$map_id <- 999993
        feat$ig_date <- as.Date("9999-01-03")
        feat$ig_year <- 9999
        test3_fid <- lyr$upsertFeature(feat)
        expect_false(is.null(test3_fid))
        expect_equal(lyr$getFeatureCount(), start_count + 1)

        # edit an existing feature and upsert with new non-existing FID
        feat <- NULL
        feat <- lyr$getNextFeature()
        test4_orig_fid <- feat$FID
        feat$FID <- bit64::as.integer64(9999999999999994)
        feat$event_id <- "ZZ04"
        feat$incid_name <- "TEST 4"
        feat$map_id <- 999994
        feat$ig_date <- as.Date("9999-01-04")
        feat$ig_year <- 9999
        test4_fid <- lyr$upsertFeature(feat)
        expect_false(is.null(test4_fid))
        expect_equal(lyr$getFeatureCount(), start_count + 2)
    }

    # read back
    lyr$open(read_only = TRUE)
    lyr$returnGeomAs <- "WKT"

    test1_feat <- lyr$getFeature(test1_fid)
    expect_false(is.null(test1_feat))
    expect_equal(test1_feat$event_id, "ZZ01")
    expect_equal(test1_feat$incid_name, "TEST 1")
    expect_equal(test1_feat$map_id, bit64::as.integer64(999991))
    expect_equal(test1_feat$ig_date, as.Date("9999-01-01"))
    test1_orig_feat <- lyr$getFeature(test1_orig_fid)
    geom_fld <- lyr$getGeometryColumn()
    expect_true(g_equals(test1_feat[[geom_fld]], test1_orig_feat[[geom_fld]]))
    test1_feat <- NULL

    test2_feat <- lyr$getFeature(test2_fid)
    expect_false(is.null(test2_feat))
    expect_equal(test2_feat$event_id, "ZZ02")
    expect_equal(test2_feat$incid_name, "TEST 2")
    expect_equal(test2_feat$map_id, bit64::as.integer64(999992))
    expect_equal(test2_feat$ig_date, as.Date("9999-01-02"))
    expect_equal(test2_feat$ig_year, 9999)
    test2_feat <- NULL

    if(.gdal_version_num() > 3060000) {
        test3_feat <- lyr$getFeature(test3_fid)
        expect_false(is.null(test3_feat))
        expect_equal(test3_feat$event_id, "ZZ03")
        expect_equal(test3_feat$incid_name, "TEST 3")
        expect_equal(test3_feat$map_id, bit64::as.integer64(999993))
        expect_equal(test3_feat$ig_date, as.Date("9999-01-03"))
        expect_equal(test3_feat$ig_year, 9999)
        test3_feat <- NULL

        test4_feat <- lyr$getFeature(test4_fid)
        expect_false(is.null(test4_feat))
        expect_equal(test4_feat$event_id, "ZZ04")
        expect_equal(test4_feat$incid_name, "TEST 4")
        expect_equal(test4_feat$map_id, bit64::as.integer64(999994))
        expect_equal(test4_feat$ig_date, as.Date("9999-01-04"))
        test4_orig_feat <- lyr$getFeature(test4_orig_fid)
        geom_fld <- lyr$getGeometryColumn()
        expect_true(g_equals(test4_feat[[geom_fld]], test4_orig_feat[[geom_fld]]))
        test4_feat <- NULL
    }

    lyr$close()
    unlink(dsn)
})
