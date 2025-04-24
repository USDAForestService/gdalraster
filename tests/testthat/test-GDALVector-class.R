# Tests for src/gdalvector.cpp
test_that("class constructors work", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    expect_no_error(lyr <- new(GDALVector, dsn))
    expect_equal(normalizePath(lyr$getDsn()), normalizePath(dsn))
    expect_equal(lyr$getName(), "mtbs_perims")
    expect_type(lyr$getFeature(1), "list")
    lyr$close()

    expect_no_error(lyr <- new(GDALVector, dsn, "mtbs_perims"))
    expect_equal(lyr$bbox(), c(469685.73, -12917.76, 573531.72, 96577.34))
    lyr$close()

    expect_no_error(lyr <- new(GDALVector, dsn, "mtbs_perims",
                               read_only = FALSE))
    expect_true(lyr$testCapability()$RandomWrite)
    lyr$close()

    # open option
    expect_no_error(lyr <- new(GDALVector, dsn, "mtbs_perims", read_only = TRUE,
                               "LIST_ALL_TABLES=NO"))
    expect_false(lyr$testCapability()$RandomWrite)
    lyr$close()

    bb <- c(469685.97, 11442.45, 544069.63, 85508.15)

    # spatial filter with SQL layer
    sql <- "SELECT FID, * FROM mtbs_perims"
    expect_no_error(lyr <- new(GDALVector, dsn, sql, read_only = TRUE,
                               open_options = NULL,
                               spatial_filter = bbox_to_wkt(bb)))
    expect_equal(lyr$getFeatureCount(), 40)
    lyr$close()

    # add dialect
    expect_no_error(lyr <- new(GDALVector, dsn, sql, read_only = TRUE,
                               open_options = NULL,
                               spatial_filter = bbox_to_wkt(bb),
                               dialect = ""))
    expect_equal(lyr$getFeatureCount(), 40)

    # spatial filter error
    expect_error(lyr <- new(GDALVector, dsn, sql, read_only = TRUE,
                            open_options = NULL,
                            spatial_filter = "invalid WKT",
                            dialect = ""))

    lyr$close()
    unlink(dsn)

    # default construstrctor with no arguments should not error
    expect_no_error(lyr <- new(GDALVector))

    # not recognized as being in a supported file format
    f <- system.file("extdata/doctype.xml", package="gdalraster")
    expect_error(lyr <- new(GDALVector, f))
})

test_that("class basic interface works", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims")

    expect_true(is(lyr, "Rcpp_GDALVector"))
    expect_output(show(lyr), "MULTIPOLYGON")

    expect_equal(lyr$getDriverShortName(), "GPKG")
    expect_equal(lyr$getDriverLongName(), "GeoPackage")
    expect_length(lyr$getFileList(), 1)
    expect_equal(lyr$getName(), "mtbs_perims")
    expect_equal(lyr$getGeomType(), "MULTIPOLYGON")
    expect_equal(lyr$getGeometryColumn(), "geom")
    expect_equal(lyr$getFIDColumn(), "fid")
    expect_true(lyr$getSpatialRef() |> srs_is_projected())
    expect_equal(lyr$bbox(), c(469685.73, -12917.76, 573531.72, 96577.34))

    expect_no_error(cap <- lyr$testCapability())
    expect_true(cap$RandomRead)
    expect_false(cap$SequentialWrite)
    expect_false(cap$RandomWrite)
    # re-open with write access
    lyr$open(read_only = FALSE)
    expect_true(lyr$testCapability()$SequentialWrite)
    expect_true(lyr$testCapability()$RandomWrite)

    expect_no_error(defn <- lyr$getLayerDefn())
    fld_names <- c("event_id", "incid_name", "incid_type", "map_id",
                   "burn_bnd_ac", "burn_bnd_lat", "burn_bnd_lon", "ig_date",
                   "ig_year", "geom")
    expect_equal(names(defn), fld_names)

    expect_equal(lyr$returnGeomAs, "WKB")
    expect_equal(lyr$getFeatureCount(), 61)
    expect_no_error(feat <- lyr$getNextFeature())
    expect_true(!is.null(feat))

    # set an attribute filter
    expect_no_error(lyr$setAttributeFilter("ig_year = 2020"))
    expect_equal(lyr$getFeatureCount(), 1)
    expect_no_error(feat <- lyr$getNextFeature())
    expect_false(is.null(feat))
    expect_no_error(feat <- lyr$getNextFeature())
    expect_true(is.null(feat))
    # reset reading to the start and return geometries as WKT
    expect_no_error(lyr$resetReading())
    lyr$returnGeomAs <- "WKT"
    expect_false(is.null(lyr$getNextFeature()))
    # clear the attribute filter
    expect_no_error(lyr$setAttributeFilter(""))
    expect_equal(lyr$getFeatureCount(), 61)

    # set a spatial filter
    #  get the bounding box of the largest 1988 fire and use as spatial filter
    #  first set a temporary attribute filter to do the lookup
    lyr$setAttributeFilter("ig_year = 1988 ORDER BY burn_bnd_ac DESC")
    feat <- lyr$getNextFeature()
    bbox <- bbox_from_wkt(feat$geom)
    expect_equal(bbox, c(469685.97, 11442.45, 544069.63, 85508.15))
    # set spatial filter on the full layer
    lyr$setAttributeFilter("")
    expect_no_error(lyr$setSpatialFilterRect(bbox))
    expect_equal(lyr$getFeatureCount(), 40)

    # fetch in chunks and return as data frame
    expect_no_error(d <- lyr$fetch(20))
    expect_true(is.data.frame(d))
    expect_equal(nrow(d), 20)
    # the next chunk
    expect_no_error(d <- lyr$fetch(20))
    expect_true(is.data.frame(d))
    expect_equal(nrow(d), 20)
    # no features remaining
    expect_no_error(d <- lyr$fetch(20))
    expect_equal(nrow(d), 0)

    # fetch all pending features with geometries as WKB
    lyr$returnGeomAs <- "WKB"
    expect_no_error(d <- lyr$fetch(-1))
    expect_equal(nrow(d), 40)

    expect_no_error(lyr$clearSpatialFilter())
    expect_equal(lyr$getFeatureCount(), 61)

    lyr$close()

    # SQL layer
    sql_lyr <- new(GDALVector, dsn, "SELECT * FROM mtbs_perims LIMIT 10")
    expect_true(is(sql_lyr, "Rcpp_GDALVector"))
    expect_output(show(sql_lyr), "LIMIT 10")
    expect_equal(sql_lyr$getFeatureCount(), 10)
    sql_lyr$close()

    unlink(dsn)
})

test_that("set ignored/selected fields works", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims")

    # set ignored, no geom
    expect_vector(lyr$getIgnoredFields(), ptype = character(), size = 0)
    lyr$returnGeomAs <- "NONE"
    expect_true(lyr$testCapability()$IgnoreFields)
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    lyr$setIgnoredFields("event_id")
    feat <- lyr$getNextFeature()
    expect_length(feat, 9)
    expect_true(is.null(feat$event_id))
    expect_false(is.null(feat$incid_name))
    expect_equal(lyr$getIgnoredFields(), "event_id")
    lyr$setIgnoredFields("")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    expect_vector(lyr$getIgnoredFields(), ptype = character(), size = 0)
    lyr$setIgnoredFields(c("event_id", "map_id", "ig_year"))
    feat <- lyr$getNextFeature()
    expect_length(feat, 7)
    expect_equal(lyr$getIgnoredFields(), c("event_id", "map_id", "ig_year"))
    lyr$setIgnoredFields("")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)

    # set selected, no geom
    # FID is always included, so expected number of fields is +1
    lyr$returnGeomAs <- "NONE"
    lyr$setSelectedFields("event_id")
    feat <- lyr$getNextFeature()
    expect_length(feat, 2)
    expect_true(is.character(feat$event_id))
    expect_true(length(lyr$getIgnoredFields()) > 1)
    lyr$setSelectedFields("")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    lyr$setSelectedFields(c("event_id", "map_id", "ig_year"))
    feat <- lyr$getNextFeature()
    expect_length(feat, 4)
    lyr$setSelectedFields("")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    expect_true(length(lyr$getIgnoredFields()) == 0)

    # geometry
    # ignoring "OGR_GEOMETRY" is redundant with returnGeomAs = "NONE"
    # make sure we can repeat "OGR_GEOMETRY" in the ignore list
    lyr$returnGeomAs <- "NONE"
    lyr$setIgnoredFields("OGR_GEOMETRY")
    expect_equal(lyr$getIgnoredFields(), "OGR_GEOMETRY")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    expect_true(is.null(feat$geom))
    lyr$returnGeomAs <- "WKT"
    lyr$setIgnoredFields("")
    feat <- lyr$getNextFeature()
    expect_length(feat, 11)
    lyr$setIgnoredFields("OGR_GEOMETRY")
    feat <- lyr$getNextFeature()
    expect_length(feat, 10)
    expect_true(is.null(feat$geom))
    lyr$setIgnoredFields(c("event_id", "OGR_GEOMETRY"))
    expect_equal(lyr$getIgnoredFields(), c("event_id", "OGR_GEOMETRY"))
    feat <- lyr$getNextFeature()
    expect_length(feat, 9)
    lyr$setIgnoredFields("")
    expect_true(length(lyr$getIgnoredFields()) == 0)
    feat <- lyr$getNextFeature()
    expect_length(feat, 11)
    # selected
    lyr$returnGeomAs <- "WKT"
    lyr$setSelectedFields(c("event_id", "OGR_GEOMETRY"))
    feat <- lyr$getNextFeature()
    expect_length(feat, 3)
    expect_false(is.null(feat$geom))
    expect_true(length(lyr$getIgnoredFields()) > 1)

    # test fetch past end with ignored fields does not crash
    # https://github.com/USDAForestService/gdalraster/issues/539
    lyr$returnGeomAs <- "WKB"
    lyr$setSelectedFields(c("incid_name", "ig_year", "OGR_GEOMETRY"))
    lyr$setAttributeFilter("ig_year >= 2018")
    expect_equal(lyr$getFeatureCount(), 3)
    expect_s3_class(lyr$getNextFeature(), "OGRFeature")
    expect_s3_class(lyr$getNextFeature(), "OGRFeature")
    expect_s3_class(lyr$getNextFeature(), "OGRFeature")
    expect_true(is.null(lyr$getNextFeature()))

    lyr$close()
    unlink(dsn)
})

test_that("read methods work correctly", {
    # TODO: complete these tests

    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims")

    field_names <- c("event_id", "incid_name", "incid_type", "map_id",
                     "burn_bnd_ac", "burn_bnd_lat", "burn_bnd_lon",
                     "ig_date", "ig_year", "geom")
    expect_equal(lyr$getFieldNames(), field_names)

    # attribute filter
    filter = "ig_year = 2020"
    expect_no_error(lyr$setAttributeFilter(filter))
    expect_equal(lyr$getFeatureCount(), 1)
    expect_equal(lyr$getAttributeFilter(), filter)
    # clear
    expect_no_error(lyr$setAttributeFilter(""))

    # spatial filter as WKT
    bbox <- c(469685.97, 11442.45, 544069.63, 85508.15)
    expect_no_error(lyr$setSpatialFilter(bbox_to_wkt(bbox)))
    expect_equal(lyr$getFeatureCount(), 40)
    expect_true(g_equals(lyr$getSpatialFilter(), bbox_to_wkt(bbox)))
    # clear
    expect_no_error(lyr$clearSpatialFilter())
    expect_equal(lyr$getFeatureCount(), 61)

    # cursor positioning
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
    rm(lyr)
    rm(dsn)

    # promoteToMulti
    dsn <- system.file("extdata/poly_multipoly.shp", package="gdalraster")
    lyr <- new(GDALVector, dsn)
    lyr$returnGeomAs <- "TYPE_NAME"
    lyr$promoteToMulti <- FALSE
    geom_fld <-lyr$defaultGeomColName
    d <- lyr$fetch(-1)
    expect_true("POLYGON" %in% d[, geom_fld])
    expect_true("MULTIPOLYGON" %in% d[, geom_fld])
    lyr$promoteToMulti <- TRUE
    d <- lyr$fetch(-1)
    expect_false("POLYGON" %in% d[, geom_fld])
    expected_geoms <- rep("MULTIPOLYGON", lyr$getFeatureCount())
    expect_equal(d[, geom_fld], expected_geoms)
    expect_equal(attr(d, "gis")$geom_col_type, "MULTIPOLYGON")
    rm(d)

    # returnGeomAs
    lyr$returnGeomAs <- "WKB"
    f <- lyr$getFeature(1)
    expect_true(is.raw(f[[geom_fld]]))
    f <- NULL
    lyr$returnGeomAs <- "WKB_ISO"
    f <- lyr$getFeature(1)
    expect_true(is.raw(f[[geom_fld]]))
    f <- NULL
    lyr$returnGeomAs <- "WKT"
    f <- lyr$getFeature(1)
    expect_true(is.character(f[[geom_fld]]))
    f <- NULL
    lyr$returnGeomAs <- "WKT_ISO"
    f <- lyr$getFeature(1)
    expect_true(is.character(f[[geom_fld]]))
    f <- NULL
    lyr$returnGeomAs <- "TYPE_NAME"
    f <- lyr$getFeature(1)
    expect_true(is.character(f[[geom_fld]]))
    f <- NULL
    lyr$returnGeomAs <- "SUMMARY"
    f <- lyr$getFeature(1)
    expect_true(is.character(f[[geom_fld]]))
    f <- NULL
    lyr$returnGeomAs <- "NONE"
    f <- lyr$getFeature(1)
    expect_true(is.null(f[[geom_fld]]))
    f <- NULL
    lyr$returnGeomAs <- "BBOX"
    f <- lyr$getFeature(1)
    expect_true(is.numeric(f[[geom_fld]]))
    expect_true(all(diff(f[[geom_fld]])[c(1, 3)] > 0))
    f <- NULL
    lyr$close()
    rm(lyr)

    # convertToLinear
    dsn <- system.file("extdata/multisurface.zip", package="gdalraster")
    dsn <- file.path("/vsizip", dsn, "multisurface.gpkg")
    lyr <- new(GDALVector, dsn, "multisurface_test")
    g1 <- lyr$fetch(-1)
    expect_equal(attr(g1, "gis")$geom_col_type, "MULTISURFACE")
    expect_equal(g_name(g1$geom), rep("MULTISURFACE", 5))
    lyr$convertToLinear <- TRUE
    g2 <- lyr$fetch(-1)
    expect_equal(attr(g2, "gis")$geom_col_type, "MULTIPOLYGON")
    expect_equal(g_name(g2$geom), rep("MULTIPOLYGON", 5))
    diff <- g_difference(g1$geom, g2$geom)
    expect_true(all(g_is_empty(diff)))
    lyr$close()
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

    expect_true(lyr$startTransaction())
    lyr$deleteFeature(10)
    expect_true(lyr$commitTransaction())
    expect_equal(lyr$getFeatureCount(), num_feat - 1)
    lyr$startTransaction()
    lyr$deleteFeature(11)
    expect_true(lyr$rollbackTransaction())
    expect_equal(lyr$getFeatureCount(), num_feat - 1)

    lyr$close()
    unlink(dsn)
})

test_that("feature write methods work", {
    ## tests on an existing data source with real data
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
    expect_true(lyr$createFeature(feat))
    expect_equal(lyr$getFeatureCount(), start_count + 1)

    # edit an existing feature and set
    feat <- NULL
    feat <- lyr$getNextFeature()
    feat$event_id <- "ZZ02"
    feat$incid_name <- "TEST 2"
    feat$map_id <- 999992
    feat$ig_date <- as.Date("9999-01-02")
    feat$ig_year <- 9999
    expect_true(lyr$setFeature(feat))
    expect_equal(lyr$getFeatureCount(), start_count + 1)

    if(gdal_version_num() > 3060000) {
        # edit an existing feature and upsert with existing FID
        feat <- NULL
        feat <- lyr$getNextFeature()
        feat$event_id <- "ZZ03"
        feat$incid_name <- "TEST 3"
        feat$map_id <- 999993
        feat$ig_date <- as.Date("9999-01-03")
        feat$ig_year <- 9999
        expect_true(lyr$upsertFeature(feat))
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
        expect_true(lyr$upsertFeature(feat))
        expect_equal(lyr$getFeatureCount(), start_count + 2)
    }

    # read back
    lyr$open(read_only = TRUE)
    lyr$returnGeomAs <- "WKT"

    lyr$setAttributeFilter("event_id = 'ZZ01'")
    test1_feat <- lyr$getNextFeature()
    expect_false(is.null(test1_feat))
    expect_equal(test1_feat$incid_name, "TEST 1")
    expect_equal(test1_feat$map_id, bit64::as.integer64(999991))
    expect_equal(test1_feat$ig_date, as.Date("9999-01-01"))
    test1_orig_feat <- lyr$getFeature(test1_orig_fid)
    geom_fld <- lyr$getGeometryColumn()
    expect_true(g_equals(test1_feat[[geom_fld]], test1_orig_feat[[geom_fld]]))
    test1_feat <- NULL

    lyr$setAttributeFilter("event_id = 'ZZ02'")
    test2_feat <- lyr$getNextFeature()
    expect_false(is.null(test2_feat))
    expect_equal(test2_feat$incid_name, "TEST 2")
    expect_equal(test2_feat$map_id, bit64::as.integer64(999992))
    expect_equal(test2_feat$ig_date, as.Date("9999-01-02"))
    expect_equal(test2_feat$ig_year, 9999)
    test2_feat <- NULL

    if(gdal_version_num() > 3060000) {
        lyr$setAttributeFilter("event_id = 'ZZ03'")
        test3_feat <- lyr$getNextFeature()
        expect_false(is.null(test3_feat))
        expect_equal(test3_feat$incid_name, "TEST 3")
        expect_equal(test3_feat$map_id, bit64::as.integer64(999993))
        expect_equal(test3_feat$ig_date, as.Date("9999-01-03"))
        expect_equal(test3_feat$ig_year, 9999)
        test3_feat <- NULL

        lyr$setAttributeFilter("event_id = 'ZZ04'")
        test4_feat <- lyr$getNextFeature()
        expect_false(is.null(test4_feat))
        expect_equal(test4_feat$incid_name, "TEST 4")
        expect_equal(test4_feat$map_id, bit64::as.integer64(999994))
        expect_equal(test4_feat$ig_date, as.Date("9999-01-04"))
        test4_orig_feat <- lyr$getFeature(test4_orig_fid)
        geom_fld <- lyr$getGeometryColumn()
        expect_true(g_equals(test4_feat[[geom_fld]],
                             test4_orig_feat[[geom_fld]]))
        test4_feat <- NULL
    }

    lyr$close()
    deleteDataset(dsn)
    rm(dsn)
    rm(lyr)


    ## tests for field types supported by GPKG
    ## (does not inlcude OGR list field types)
    dsn2 <- tempfile(fileext = ".gpkg")
    defn <- ogr_def_layer("Point", srs = epsg_to_wkt(4326))
    defn$int_fld <- ogr_def_field("OFTInteger")
    defn$bool_fld <- ogr_def_field("OFTInteger", fld_subtype = "OFSTBoolean")
    defn$int64_fld <- ogr_def_field("OFTInteger64")
    defn$real_fld <- ogr_def_field("OFTReal")
    defn$str_fld <- ogr_def_field("OFTString", fld_width = 100)
    defn$date_fld <- ogr_def_field("OFTDate")
    defn$datetime_fld <- ogr_def_field("OFTDateTime")
    defn$time_fld <- ogr_def_field("OFTTime")
    defn$binary_fld <- ogr_def_field("OFTBinary")

    expect_true(ogr_ds_create("GPKG", dsn2, "test_layer", layer_defn = defn))

    lyr <- new(GDALVector, dsn2, "test_layer", read_only = FALSE)
    expect_equal(lyr$getFeatureCount(), 0)

    geom_fld <- lyr$getGeometryColumn()

    feat1 <- list()
    feat1$int_fld <- 1
    feat1$bool_fld <- TRUE
    feat1$int64_fld <- bit64::as.integer64(11)
    feat1$real_fld <- 1.1
    feat1$str_fld <- "string 1"
    feat1$date_fld <- as.Date("2000-01-01")
    feat1$datetime_fld <- as.POSIXct("2000-01-01 13:01:01.123 GMT", tz = "UTC")
    feat1$time_fld <- "01:02:03"
    feat1$binary_fld <- as.raw(c(1, 1, 1))
    feat1[[geom_fld]] <- "POINT (1 1)"

    expect_true(lyr$createFeature(feat1))
    test1_fid <- lyr$getLastWriteFID()
    expect_false(is.null(test1_fid))

    expect_true(lyr$createFeature(feat1))
    test2_fid <- lyr$getLastWriteFID()
    expect_false(is.null(test2_fid))

    lyr$open(read_only = TRUE)
    expect_equal(lyr$getFeatureCount(), 2)

    lyr$open(read_only = FALSE)

    # edit feature 2
    feat2 <- list()
    feat2$FID <- test2_fid
    feat2$int_fld <- 2
    feat2$bool_fld <- FALSE
    feat2$int64_fld <- bit64::as.integer64(22)
    feat2$real_fld <- 2.2
    feat2$str_fld <- "string 2"
    feat2$date_fld <- as.Date("2000-01-02")
    feat2$datetime_fld <- as.POSIXct("2000-01-02 14:02.234 GMT", tz = "UTC")
    feat2$time_fld <- "02:03:04"
    feat2$binary_fld <- as.raw(c(2, 2, 2))
    feat2[[geom_fld]] <- "POINT (2 2)"

    expect_true(lyr$setFeature(feat2))
    expect_equal(lyr$getLastWriteFID(), test2_fid)

    # read back
    lyr$open(read_only = TRUE)
    expect_equal(lyr$getFeatureCount(), 2)

    lyr$returnGeomAs <- "WKT"

    feat1_check <- lyr$getFeature(test1_fid)
    feat1_check$FID <- NULL
    class(feat1_check) <- "list"
    attr(feat1_check, "gis") <- NULL
    expect_equal(feat1_check, feat1)

    feat2_check <- lyr$getFeature(test2_fid)
    class(feat2_check) <- "list"
    attr(feat2_check, "gis") <- NULL
    expect_equal(feat2_check, feat2)

    lyr$open(read_only = FALSE)

    # NULL geometry
    # as NA
    feat3 <- list()
    feat3$int_fld <- 3
    feat3$bool_fld <- TRUE
    feat3$int64_fld <- bit64::as.integer64(33)
    feat3$real_fld <- 3.3
    feat3$str_fld <- "string 3"
    feat3$date_fld <- as.Date("2000-01-03")
    feat3$datetime_fld <- as.POSIXct("2000-01-03 13:01:01.123 GMT", tz = "UTC")
    feat3$time_fld <- "03:04:05"
    feat3$binary_fld <- as.raw(c(3, 3, 3))
    feat3[[geom_fld]] <- NA

    expect_true(lyr$createFeature(feat3))
    test3_fid <- lyr$getLastWriteFID()
    expect_false(is.null(test3_fid))

    # as NULL
    feat4 <- list()
    feat4$int_fld <- 4
    feat4$bool_fld <- TRUE
    feat4$int64_fld <- bit64::as.integer64(44)
    feat4$real_fld <- 4.4
    feat4$str_fld <- "string 4"
    feat4$date_fld <- as.Date("2000-01-04")
    feat4$datetime_fld <- as.POSIXct("2000-01-04 13:01:01.123 GMT", tz = "UTC")
    feat4$time_fld <- "04:05:06"
    feat4$binary_fld <- as.raw(c(4, 4, 4))
    feat4[[geom_fld]] <- list(NULL)

    expect_true(lyr$createFeature(feat4))
    test4_fid <- lyr$getLastWriteFID()
    expect_false(is.null(test4_fid))

    # read back
    lyr$open(read_only = TRUE)
    expect_equal(lyr$getFeatureCount(), 4)

    lyr$returnGeomAs <- "WKB"

    feat3_check <- lyr$getFeature(test3_fid)
    expect_true(is.null(feat3_check[[geom_fld]]))

    feat4_check <- lyr$getFeature(test4_fid)
    expect_true(is.null(feat4_check[[geom_fld]]))

    lyr$open(read_only = FALSE)

    # NULL attribute fields
    # as NA
    feat5 <- list()
    feat5$int_fld <- NA
    feat5$bool_fld <- NA
    feat5$int64_fld <- NA
    feat5$real_fld <- NA
    feat5$str_fld <- NA
    feat5$date_fld <- NA
    feat5$datetime_fld <- NA
    feat5$time_fld <- NA
    feat5$binary_fld <- raw()
    feat5[[geom_fld]] <- "POINT (5 5)"

    expect_true(lyr$createFeature(feat5))
    test5_fid <- lyr$getLastWriteFID()
    expect_false(is.null(test5_fid))

    # as typed NA
    feat6 <- list()
    feat6$int_fld <- NA_integer_
    feat6$bool_fld <- NA
    feat6$int64_fld <- bit64::NA_integer64_
    feat6$real_fld <- NA_real_
    feat6$str_fld <- NA_character_
    feat6$date_fld <- as.Date(NA_real_)
    feat6$datetime_fld <- as.POSIXct(NA_real_)
    feat6$time_fld <- NA_character_
    feat6$binary_fld <- raw(0)
    feat6[[geom_fld]] <- "POINT (6 6)"

    expect_true(lyr$createFeature(feat6))
    test6_fid <- lyr$getLastWriteFID()
    expect_false(is.null(test6_fid))

    # as NULL
    feat7 <- list()
    feat7$int_fld <- list(NULL)
    feat7$bool_fld <- list(NULL)
    feat7$int64_fld <- list(NULL)
    feat7$real_fld <- list(NULL)
    feat7$str_fld <- list(NULL)
    feat7$date_fld <- list(NULL)
    feat7$datetime_fld <- list(NULL)
    feat7$time_fld <- list(NULL)
    feat7$binary_fld <- list(NULL)
    feat7[[geom_fld]] <- "POINT (7 7)"

    expect_true(lyr$createFeature(feat7))
    test7_fid <- lyr$getLastWriteFID()
    expect_false(is.null(test7_fid))

    # read back
    lyr$open(read_only = TRUE)
    lyr$returnGeomAs <- "WKT"

    feat5_check <- lyr$getFeature(test5_fid)
    expect_true(is.na(feat5_check$int_fld ))
    expect_true(is.na(feat5_check$bool_fld))
    expect_true(is.na(feat5_check$int64_fld))
    expect_true(is.na(feat5_check$real_fld))
    expect_true(is.na(feat5_check$str_fld))
    expect_true(is.na(feat5_check$date_fld))
    expect_true(is.na(feat5_check$datetime_fld))
    expect_true(is.null(feat5_check$binary_fld))
    expect_equal(feat5_check[[geom_fld]], "POINT (5 5)")

    feat6_check <- lyr$getFeature(test6_fid)
    expect_true(is.na(feat6_check$int_fld ))
    expect_true(is.na(feat6_check$bool_fld))
    expect_true(is.na(feat6_check$int64_fld))
    expect_true(is.na(feat6_check$real_fld))
    expect_true(is.na(feat6_check$str_fld))
    expect_true(is.na(feat6_check$date_fld))
    expect_true(is.na(feat6_check$datetime_fld))
    expect_true(is.null(feat6_check$binary_fld))
    expect_equal(feat6_check[[geom_fld]], "POINT (6 6)")

    feat7_check <- lyr$getFeature(test7_fid)
    expect_true(is.na(feat7_check$int_fld ))
    expect_true(is.na(feat7_check$bool_fld))
    expect_true(is.na(feat7_check$int64_fld))
    expect_true(is.na(feat7_check$real_fld))
    expect_true(is.na(feat7_check$str_fld))
    expect_true(is.na(feat7_check$date_fld))
    expect_true(is.na(feat7_check$datetime_fld))
    expect_true(is.null(feat7_check$binary_fld))
    expect_equal(feat7_check[[geom_fld]], "POINT (7 7)")

    ## test input errors
    lyr$open(read_only = FALSE)

    # NULL feature
    expect_error(lyr$createFeature(NULL))
    expect_error(lyr$setFeature(NULL))

    # feature not a list
    feat <- c(1, 2)
    expect_error(lyr$setFeature(feat))
    feat <- c("1", "2")
    expect_error(lyr$setFeature(feat))

    # no element names
    feat <- lyr$getFeature(1)
    names(feat) <- NULL
    expect_error(lyr$setFeature(feat))

    # a name does not match the layer schema
    feat <- lyr$getFeature(1)
    feat$nonexistent_fld <- 1
    expect_error(lyr$setFeature(feat))
5
    feat <- lyr$getFeature(1)

    # character for OFTInteger
    orig <- feat$int_fld
    feat$int_fld <- "1"
    expect_error(lyr$setFeature(feat))
    feat$int_fld <- orig

    # character for OFTInteger64
    orig <- feat$int64_fld
    feat$int64_fld <- "1"
    expect_error(lyr$setFeature(feat))
    feat$int64_fld <- orig

    # character for OFTReal
    orig <- feat$real_fld
    feat$real_fld <- "1"
    expect_error(lyr$setFeature(feat))
    feat$real_fld <- orig

    # numeric for OFTString
    orig <- feat$str_fld
    feat$str_fld <- 1
    expect_error(lyr$setFeature(feat))
    feat$str_fld <- orig

    # missing Date class for OFTDate
    orig <- feat$date_fld
    feat$date_fld <- as.numeric(as.Date("9999-01-04"))
    expect_error(lyr$setFeature(feat))
    feat$date_fld <- orig

    # character string for OFTDate
    orig <- feat$date_fld
    feat$date_fld <- "2000-01-01"
    expect_error(lyr$setFeature(feat))
    feat$date_fld <- orig

    # missing POSIXct class for OFTDateTime
    orig <- feat$datetime_fld
    feat$datetime_fld <- as.numeric(as.Date("9999-01-04"))
    expect_error(lyr$setFeature(feat))
    feat$datetime_fld <- orig

    # character string for OFTDateTime
    orig <- feat$datetime_fld
    feat$datetime_fld <- "2000-01-02 14:02.234 GMT"
    expect_error(lyr$setFeature(feat))
    feat$datetime_fld <- orig

    # other than raw vector for OFTBinary
    orig <- feat$binary_fld
    feat$binary_fld <- integer(10)
    expect_error(lyr$setFeature(feat))
    feat$binary_fld <- orig

    orig <- feat$binary_fld
    feat$binary_fld <- numeric(10)
    expect_error(lyr$setFeature(feat))
    feat$binary_fld <- orig

    orig <- feat$binary_fld
    feat$binary_fld <- character(10)
    expect_error(lyr$setFeature(feat))
    feat$binary_fld <- orig

    # other than raw vector for OFTBinary, in list
    orig <- feat$binary_fld
    feat$binary_fld <- list(integer(10))
    expect_error(lyr$setFeature(feat))
    feat$binary_fld <- orig

    # not character or raw vector for geom field
    orig <- feat[[geom_fld]]
    feat[[geom_fld]] <- integer(10)
    expect_false(lyr$setFeature(feat))
    feat[[geom_fld]] <- orig

    # not character or raw vector for geom field, in list
    orig <- feat[[geom_fld]]
    feat[[geom_fld]] <- list(numeric(10))
    expect_false(lyr$setFeature(feat))
    feat[[geom_fld]] <- orig

    # multi-row data frame
    feat <- lyr$fetch(-1)
    expect_true(nrow(feat) > 1)
    expect_error(lyr$setFeature(feat))

    lyr$close()
    deleteDataset(dsn2)
    rm(lyr)
    rm(dsn2)


    ## tests for OGR list field types with the CSV driver
    dsn3 <- file.path(tempdir(), "test_list.csv")

    defn <- ogr_def_layer("Point", srs = epsg_to_wkt(4326))
    defn$id <- ogr_def_field("OFTInteger")
    defn$real_fld <- ogr_def_field("OFTReal")
    defn$str_fld <- ogr_def_field("OFTString")
    defn$int_list_fld <- ogr_def_field("OFTIntegerList", "JSonIntegerList")
    defn$real_list_fld <- ogr_def_field("OFTRealList", "JSonRealList")
    defn$str_list_fld <- ogr_def_field("OFTStringList", "JSonStringList")

    lyr_opt <- c("GEOMETRY=AS_WKT", "CREATE_CSVT=YES")
    expect_true(ogr_ds_create("CSV", dsn3, "test_list", layer_defn = defn,
                              lco = lyr_opt, overwrite = TRUE))

    lyr <- new(GDALVector, dsn3, "test_list", read_only = FALSE)

    geom_fld <- lyr$getGeometryColumn()

    feat1 <- list()
    feat1$id <- 1
    feat1$real_fld <- 1.1
    feat1$str_fld <- "string 1"
    feat1$int_list_fld <- c(1, 1, 1)
    feat1$real_list_fld <- c(1.1, 1.1, 1.1)
    feat1$str_list_fld <- c("str 1", "str 1", "str 1")
    feat1[[geom_fld]] <- "POINT (1 1)"

    expect_true(lyr$createFeature(feat1))

    expect_true(lyr$syncToDisk())

    # close and re-open
    lyr$open(read_only = TRUE)
    lyr$returnGeomAs <- "WKT"

    f <- lyr$getNextFeature()
    expect_equal(f$id, feat1$id)
    expect_equal(f$real_fld, feat1$real_fld)
    expect_equal(f$str_fld, feat1$str_fld)
    expect_equal(f$int_list_fld, feat1$int_list_fld)
    expect_equal(f$real_list_fld, feat1$real_list_fld)
    expect_equal(f$str_list_fld, feat1$str_list_fld)
    # this fails with GDAL < 3.5 due to change in geom column naming?
    # expect_true(g_equals(f$WKT, feat1[[geom_fld]]))

    lyr$close()
    deleteDataset(dsn3)
    rm(lyr)
    rm(dsn3)


    ## test ESRI Shapefile for supported field types, Polygon geom, no SRS set
    dsn4 <- tempfile(fileext = ".shp")

    defn <- ogr_def_layer("Polygon")
    defn$id <- ogr_def_field("OFTInteger")
    defn$real_fld <- ogr_def_field("OFTReal")
    defn$str_fld <- ogr_def_field("OFTString")
    defn$date_fld <- ogr_def_field("OFTDate")

    expect_true(ogr_ds_create("ESRI Shapefile", dsn4, "", layer_defn = defn,
                              overwrite = TRUE))

    lyr <- new(GDALVector, dsn4, "", read_only = FALSE)
    expect_equal(lyr$getFeatureCount(), 0)

    feat1 <- list()
    feat1$id <- 100
    feat1$real_fld <- 0.123
    feat1$str_fld <- "test string"
    feat1$date_fld <- as.Date("2100-01-01")
    feat1$geom <- "POLYGON ((0 0,0 10,10 10,0 0),(0.25 0.5,1 1,0.5 1,0.25 0.5))"

    test1_fid <- NULL
    expect_true(lyr$createFeature(feat1))
    test1_fid <- lyr$getLastWriteFID()
    expect_false(is.null(test1_fid))

    # close and re-open
    lyr$open(read_only = TRUE)
    lyr$returnGeomAs <- "WKT"

    f <- lyr$getNextFeature()

    expect_equal(f$FID, test1_fid)
    expect_equal(f$id, feat1$id)
    expect_equal(f$real_fld, feat1$real_fld)
    expect_equal(f$str_fld, feat1$str_fld)
    expect_equal(f$date_fld, feat1$date_fld)
    expect_true(g_equals(f$geom, feat1$geom))

    lyr$close()
    deleteDataset(dsn4)
    rm(lyr)
    rm(dsn4)

    ## test GeoJSON write, Point with SRS
    dsn5 <- tempfile(fileext = ".geojson")

    defn <- ogr_def_layer("Point", srs = epsg_to_wkt(4322))
    defn$real_field <- ogr_def_field("OFTReal")
    defn$str_field <- ogr_def_field("OFTString")

    expect_no_error(lyr <- ogr_ds_create("GeoJSON", dsn5, "test_layer",
                                         layer_defn = defn,
                                         lco = "WRITE_BBOX=YES",
                                         overwrite = TRUE,
                                         return_obj = TRUE))

    feat1 <- list()
    feat1$real_field <- 0.123
    feat1$str_field <- "test string 1"
    feat1$geom <- "POINT (1 10)"
    expect_true(lyr$createFeature(feat1))


    feat2 <- list()
    feat2$real_field <- 0.234
    feat2$str_field <- "test string 2"
    feat2$geom <- "POINT (2 20)"
    expect_true(lyr$createFeature(feat2))

    # close and re-open
    lyr$open(read_only = TRUE)

    expect_equal(lyr$getFeatureCount(), 2)
    expect_equal(lyr$bbox(), c(1, 10, 2, 20))
    expect_true(srs_is_same(lyr$getSpatialRef(), epsg_to_wkt(4322)))

    lyr$close()
    unlink(dsn5)
    rm(lyr)
    rm(dsn5)

    ## test GeoJSON write, geom only, no SRS
    dsn6 <- tempfile(fileext = ".geojson")
    lyr <- ogr_ds_create("GeoJSON", dsn6, "box", geom_type = "POLYGON",
                         return_obj = TRUE)
    pts <-  matrix(c(0.25, 0.25, 0.75, 0.25, 0.75, 0.75, 0.25, 0.75,
                     0.25, 0.25), ncol = 2, byrow = TRUE)
    feat <- list()
    feat$geom <- g_create("POLYGON", pts)
    expect_true(lyr$createFeature(feat))
    lyr$open(read_only = TRUE)
    expect_equal(lyr$getFeatureCount(), 1)
    feat_chk <- lyr$getNextFeature()
    expect_true(g_equals(feat$geom, feat_chk$geom))

    lyr$close()
    unlink(dsn6)
    rm(lyr)
    rm(dsn6)
})

test_that("feature batch writing works", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    sql <- "SELECT incid_name, burn_bnd_ac, ig_date, geom
            FROM mtbs_perims WHERE ig_year > 2010"
    lyr <- new(GDALVector, dsn, sql)

    # define new layer by modifying the source definition
    defn <- lyr$getLayerDefn()
    # define new attribute field
    defn$burn_bnd_ha <- ogr_def_field("OFTInteger64")
    # redefine geom field
    defn$geom <- ogr_def_geom_field("POINT", srs = defn$geom$srs)

    dst_dsn <- tempfile(fileext = ".gpkg")
    new_lyr <- ogr_ds_create("GPKG", dst_dsn, "mtbs_centroids",
                             layer_defn = defn, overwrite = TRUE,
                             return_obj = TRUE)

    d <- lyr$fetch(-1)
    # create a new data frame of point features
    d_new <- data.frame(d[, c("FID", "incid_name", "burn_bnd_ac", "ig_date")])
    # add new calculated attribute field
    d_new$burn_bnd_ha <- d_new$burn_bnd_ac / 2.471
    # add a geom field with the centroids
    perim_centroids <- g_centroid(d$geom)
    d_new$geom <- g_create("POINT", perim_centroids)

    # batch write
    expect_no_error(ret <- new_lyr$batchCreateFeature(d_new))
    expect_vector(ret, logical(), size = 15)
    expect_true(all(ret))

    # read back
    new_lyr$open(read_only = TRUE)
    expect_equal(new_lyr$getName(), "mtbs_centroids")
    d_new_out <- new_lyr$fetch(-1)
    expect_equal(nrow(d_new_out), 15)
    expect_equal(d_new_out$incid_name, d$incid_name)
    expect_equal(d_new_out$ig_date, d$ig_date)
    expect_equal(sum(d_new_out$burn_bnd_ha - (d$burn_bnd_ac / 2.471)), 0,
                 tolerance = 0.01)

    pt_coords <- g_coords(d_new_out$geom)
    expect_equal(cbind(pt_coords$x, pt_coords$y), perim_centroids,
                 ignore_attr = TRUE)

    lyr$close()
    unlink(dsn)
    new_lyr$close()
    unlink(dst_dsn)
})

test_that("get/set metadata works", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims", read_only = FALSE)

    expect_no_error(lyr$getMetadata())
    expect_equal(lyr$getMetadataItem(mdi_name = "DESCRIPTION"),
                 "MTBS fire perims 1984-2022 clipped to YNP bbox")

    # write metadata
    md <- c("TEST_ITEM_1=test 1 string", "TEST_ITEM_2=test 2 string")
    expect_true(lyr$setMetadata(md))

    # close and re-open
    lyr$open(read_only = TRUE)
    expect_equal(lyr$getMetadataItem(mdi_name = "TEST_ITEM_1"), "test 1 string")
    expect_equal(lyr$getMetadataItem(mdi_name = "TEST_ITEM_2"), "test 2 string")

    lyr$close()
    deleteDataset(dsn)
})

test_that("field domain specifications are returned correctly", {
    skip_if(gdal_version_num() < 3030000)

    f <- system.file("extdata/domains.gpkg", package="gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn)

    # integer range domain
    fld_dom <- lyr$getFieldDomain("range_domain_int")
    expect_true(!is.null(fld_dom))
    expect_equal(fld_dom$domain_type, "range")
    expect_equal(fld_dom$field_type, "Integer")
    expect_equal(fld_dom$split_policy, "default value")
    expect_equal(fld_dom$merge_policy, "default value")
    expect_equal(fld_dom$min_value, 1)
    expect_true(fld_dom$min_value_included)
    expect_equal(fld_dom$max_value, 2)
    expect_false(fld_dom$max_value_included)
    rm(fld_dom)

    # integer64 range domain
    fld_dom <- lyr$getFieldDomain("range_domain_int64")
    expect_true(!is.null(fld_dom))
    expect_equal(fld_dom$domain_type, "range")
    expect_equal(fld_dom$field_type, "Integer64")
    expect_equal(fld_dom$split_policy, "default value")
    expect_equal(fld_dom$merge_policy, "default value")
    expect_equal(fld_dom$min_value, bit64::as.integer64(-1234567890123))
    expect_false(fld_dom$min_value_included)
    expect_equal(fld_dom$max_value, bit64::as.integer64(1234567890123))
    expect_true(fld_dom$max_value_included)
    rm(fld_dom)

    # real range domain
    fld_dom <- lyr$getFieldDomain("range_domain_real")
    expect_true(!is.null(fld_dom))
    expect_equal(fld_dom$domain_type, "range")
    expect_equal(fld_dom$field_type, "Real")
    expect_equal(fld_dom$split_policy, "default value")
    expect_equal(fld_dom$merge_policy, "default value")
    expect_equal(fld_dom$min_value, 1.5)
    expect_true(fld_dom$min_value_included)
    expect_equal(fld_dom$max_value, 2.5)
    expect_true(fld_dom$max_value_included)
    rm(fld_dom)

    # real range domain inf
    fld_dom <- lyr$getFieldDomain("range_domain_real_inf")
    expect_true(!is.null(fld_dom))
    expect_equal(fld_dom$domain_type, "range")
    expect_equal(fld_dom$field_type, "Real")
    expect_equal(fld_dom$split_policy, "default value")
    expect_equal(fld_dom$merge_policy, "default value")
    expect_true(is.null(fld_dom$min_value))
    expect_true(is.null(fld_dom$mxn_value))
    rm(fld_dom)

    # coded values domain
    fld_dom <- lyr$getFieldDomain("enum_domain")
    expect_true(!is.null(fld_dom))
    expect_equal(fld_dom$domain_type, "coded")
    expect_equal(fld_dom$field_type, "Integer")
    expect_equal(fld_dom$split_policy, "default value")
    expect_equal(fld_dom$merge_policy, "default value")
    expect_vector(fld_dom$coded_values, character(), size = 2)
    expect_equal(fld_dom$coded_values[["1"]], "one")
    expect_equal(fld_dom$coded_values[["2"]], "")
    rm(fld_dom)

    # glob domain
    fld_dom <- lyr$getFieldDomain("glob_domain")
    expect_true(!is.null(fld_dom))
    expect_equal(fld_dom$domain_type, "glob")
    expect_equal(fld_dom$field_type, "String")
    expect_equal(fld_dom$split_policy, "default value")
    expect_equal(fld_dom$merge_policy, "default value")
    expect_equal(fld_dom$glob, "*")
    rm(fld_dom)

    lyr$close()
    deleteDataset(dsn)
})

test_that("info() prints output to the console", {
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn)

    lyr <- new(GDALVector, dsn, "mtbs_perims")
    expect_output(lyr$info())
    lyr$close()

    lyr <- new(GDALVector, dsn, "SELECT * FROM mtbs_perims LIMIT 10")
    if (gdal_version_num() >= 3070000) {
        expect_output(lyr$info(), "Feature Count: 10")
    } else {
        # we only get the fallback minimal info
        expect_output(lyr$info(), "Layer")
    }
    lyr$close()

    # default layer first by index
    lyr <- new(GDALVector, dsn)
    if (gdal_version_num() >= 3070000) {
        expect_output(lyr$info(), "Feature Count: 61")
    } else {
        # we only get the fallback minimal info
        expect_output(lyr$info(), "Layer")
    }
    lyr$close()

    unlink(dsn)

    skip_if_not(gdal_version_num() >= 3070000)

    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn)

    lyr <- new(GDALVector, dsn, "mtbs_perims")
    lyr$setAttributeFilter("ig_year = 2020")
    expect_output(lyr$info(), "Feature Count: 1")
    lyr$setAttributeFilter("")
    expect_output(lyr$info(), "Feature Count: 61")

    lyr$resetReading()
    lyr$setAttributeFilter("ig_year = 1988 ORDER BY burn_bnd_ac DESC")
    feat <- lyr$getNextFeature()
    bbox <- g_wk2wk(feat$geom) |> bbox_from_wkt()
    lyr$setAttributeFilter("")
    lyr$setSpatialFilterRect(bbox)
    expect_output(lyr$info(), "Feature Count: 40")

    lyr$close()
    unlink(dsn)

    skip_if_not(has_spatialite())

    dsn <- system.file("extdata/poly_multipoly.shp", package="gdalraster")
    sql <- "SELECT 1 As ID, ST_Union(geometry) As geom FROM poly_multipoly GROUP BY ID"
    lyr <- new(GDALVector, dsn, sql, TRUE, NULL, "", "SQLite")
    expect_output(lyr$info(), "Feature Count: 1")
    lyr$close()
})

test_that("ArrowArrayStream is readable", {
    skip_if(gdal_version_num() < 3060000)
    skip_if_not_installed("nanoarrow")

    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims")

    expect_no_error(stream <- lyr$getArrowStream())
    expect_s3_class(stream, "nanoarrow_array_stream")

    schema <- stream$get_schema()
    expect_s3_class(schema, "nanoarrow_schema")
    expect_equal(length(schema$children), 11)

    batch <- stream$get_next()
    expect_s3_class(batch, "nanoarrow_array")
    expect_equal(batch$children$fid$length, 61)

    expect_no_error(stream$release())

    # with options
    lyr$arrowStreamOptions <- "INCLUDE_FID=NO"
    expect_no_error(stream2 <- lyr$getArrowStream())

    schema2 <- stream2$get_schema()
    expect_equal(length(schema2$children), 10)
    expect_true(is.null(schema2$children$fid))
    expect_no_error(stream2$release())

    lyr$close()

    deleteDataset(dsn)
})

test_that("nanoarrow_array_stream implicit release works", {
    skip_if(gdal_version_num() < 3060000)
    skip_if_not_installed("nanoarrow")

    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    dsn <- file.path(tempdir(), basename(f))
    file.copy(f, dsn, overwrite = TRUE)

    lyr <- new(GDALVector, dsn, "mtbs_perims")

    # dataset/layer closed without explicit release
    lyr$open(read_only = TRUE)
    expect_no_error(stream3 <- lyr$getArrowStream())
    expect_s3_class(stream3, "nanoarrow_array_stream")
    lyr$close()
    expect_error(stream3$get_next())

    # stream garbage collected without explicit release
    lyr$open(read_only = TRUE)
    expect_no_error(stream3 <- lyr$getArrowStream())
    expect_s3_class(stream3, "nanoarrow_array_stream")
    rm(stream3)
    gc()
    # released implicitly so a new stream should be available
    expect_no_error(stream4 <- lyr$getArrowStream())
    expect_s3_class(stream4, "nanoarrow_array_stream")

    lyr$close()

    deleteDataset(dsn)
})
