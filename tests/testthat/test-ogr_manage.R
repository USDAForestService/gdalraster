test_that("OGR management utilities work", {
    # these tests assume GDAL was built with libsqlite3 which is optional
    # this should be a safe assumption since PROJ requires libsqlite3

    # GPKG
    dsn <- tempfile(fileext = ".gpkg")
    expect_false(ogr_ds_exists(dsn))
    expect_true(ogr_ds_create("GPKG", dsn, "test_layer", geom_type = "POLYGON"))
    expect_true(ogr_ds_exists(dsn, with_update=TRUE))
    expect_true(ogr_layer_exists(dsn, "test_layer"))
    # overwrite defaults to FALSE
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer",
                               geom_type = "POLYGON"))
    expect_true(ogr_ds_create("GPKG", dsn, "test_layer", geom_type = "POLYGON",
                              overwrite = TRUE))
    expect_true(ogr_ds_exists(dsn, with_update=TRUE))
    expect_true(ogr_layer_exists(dsn, "test_layer"))
    expect_false(ogr_layer_exists(dsn, "test_layer_2"))
    expect_true(ogr_ds_test_cap(dsn)$CreateLayer)
    lco <- "DESCRIPTION=test layer2"
    expect_true(ogr_layer_create(dsn, "test_layer_2", NULL, "POLYGON",
                                 "EPSG:5070", lco))
    expect_true(ogr_layer_exists(dsn, "test_layer_2"))
    expect_equal(ogr_ds_layer_count(dsn), 2)
    expect_equal(ogr_ds_layer_names(dsn), c("test_layer", "test_layer_2"))
    expect_equal(ogr_field_index(dsn, "test_layer_2", "DN"), -1)
    expect_true(ogr_layer_test_cap(dsn, "test_layer_2")$CreateField)
    expect_true(ogr_field_create(dsn, "test_layer_2", "DN",
                                 fld_type = "OFTInteger"))
    expect_equal(ogr_field_index(dsn, "test_layer_2", "DN"), 0)
    expect_equal(ogr_layer_field_names(dsn, "test_layer_2"), c("DN", "geom"))
    expect_true(ogr_ds_test_cap(dsn)$DeleteLayer)
    expect_true(ogr_layer_delete(dsn, "test_layer"))
    expect_equal(ogr_ds_layer_count(dsn), 1)
    expect_false(ogr_layer_delete(dsn, "test_layer_nonexist"))
    expect_true(ogr_layer_test_cap(dsn, "test_layer_2")$AlterFieldDefn)
    expect_true(ogr_field_rename(dsn, "test_layer_2", "DN", "renamed"))
    expect_equal(ogr_layer_field_names(dsn, "test_layer_2"),
                 c("renamed", "geom"))
    expect_true(ogr_layer_test_cap(dsn, "test_layer_2")$DeleteField)
    expect_true(ogr_field_delete(dsn, "test_layer_2", "renamed"))
    expect_equal(ogr_layer_field_names(dsn, "test_layer_2"), "geom")
    expect_true(ogr_layer_delete(dsn, "test_layer_2"))
    expect_equal(ogr_ds_layer_count(dsn), 0)

    defn <- ogr_def_layer("Point", srs = epsg_to_wkt(4326))
    defn$field1 <- ogr_def_field("OFTInteger64",
                                 is_nullable = FALSE,
                                 is_unique = TRUE)
    defn$field2 <- ogr_def_field("OFTString",
                                 fld_width = 25,
                                 is_nullable = FALSE,
                                 default_value = "'a default string'")
    defn$field3 <- ogr_def_field("OFTReal")
    expect_true(ogr_layer_create(dsn, "test_layer3", layer_defn = defn))
    expect_equal(ogr_ds_layer_names(dsn), "test_layer3")
    expect_equal(ogr_layer_field_names(dsn, "test_layer3"),
                 c("field1", "field2", "field3", "geom"))

    fld4_defn <- ogr_def_field("OFTReal",
                               default_value = "0.0")
    expect_true(ogr_field_create(dsn, "test_layer3", "field4",
                                 fld_defn = fld4_defn))
    fld5_defn <- ogr_def_field("OFTString",
                               fld_width = 25,
                               is_nullable = FALSE,
                               default_value = "'a default string'")
    expect_true(ogr_field_create(dsn, "test_layer3", "field5",
                                 fld_defn = fld5_defn))
    fld6_defn <- ogr_def_field("OFTInteger", fld_subtype = "OFSTBoolean")
    expect_true(ogr_field_create(dsn, "test_layer3", "field6",
                                 fld_defn = fld6_defn))
    expect_equal(ogr_layer_field_names(dsn, "test_layer3"),
                 c("field1", "field2", "field3", "field4", "field5", "field6",
                   "geom"))

    # only one geom field supported by GPKG
    geomfld2_defn <- ogr_def_geom_field("Polygon", srs = epsg_to_wkt(4326))
    expect_false(ogr_geom_field_create(dsn, "test_layer3", "geom2",
                                       geom_fld_defn = geomfld2_defn))
    expect_equal(ogr_layer_field_names(dsn, "test_layer3"),
                 c("field1", "field2", "field3", "field4", "field5", "field6",
                   "geom"))

    deleteDataset(dsn)

    # SQLite
    dsn <- tempfile(fileext = "sqlite")
    defn <- ogr_def_layer("Point", srs = epsg_to_wkt(4326))
    defn$field1 <- ogr_def_field("OFTInteger64",
                                 is_nullable = FALSE,
                                 is_unique = TRUE)
    defn$field2 <- ogr_def_field("OFTString",
                                 fld_width = 25,
                                 default_value = "'a default string'")
    defn$field3 <- ogr_def_field("OFTReal")
    ds_opt <- NULL
    if (has_spatialite())
        ds_opt <- "SPATIALITE=YES"
    expect_true(ogr_ds_create("SQLite", dsn, "layer1",
                              layer_defn = defn,
                              dsco = ds_opt,
                              lco = "GEOMETRY_NAME=geom1"))
    expect_equal(ogr_layer_field_names(dsn, "layer1"),
                 c("field1", "field2", "field3", "geom1"))

    # multiple geom fields supported by SQLite
    geomfld2_defn <- ogr_def_geom_field("Polygon", srs = epsg_to_wkt(4326))
    expect_true(ogr_geom_field_create(dsn, "layer1", "geom2",
                                      geom_fld_defn = geomfld2_defn))
    expect_equal(ogr_layer_field_names(dsn, "layer1"),
                 c("field1", "field2", "field3", "geom1", "geom2"))

    deleteDataset(dsn)

    # create with two geom fields initially
    # with geom2 nullable and ignored
    dsn <- tempfile(fileext = "sqlite")
    defn$geom2 <- ogr_def_geom_field("Polygon", srs = epsg_to_wkt(4326),
                                     is_nullable = TRUE, is_ignored = TRUE)
    expect_true(ogr_ds_create("SQLite", dsn, "layer1",
                              layer_defn = defn))
    expect_equal(ogr_layer_field_names(dsn, "layer1"),
                 c("field1", "field2", "field3", "GEOMETRY", "geom2"))

    deleteDataset(dsn)
})

test_that("edit data using SQL works on shapefile", {
    # TODO(ctoney): add tests to confirm data once vector I/O is implemented

    src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    perims_shp <- paste0(tempdir(), "/", "mtbs_perims.shp")
    ogr2ogr(src, perims_shp, src_layers = "mtbs_perims")
    expect_equal(ogr_ds_layer_names(perims_shp), "mtbs_perims")
    num_fields <- length(ogr_layer_field_names(perims_shp, "mtbs_perims"))
    expect_true(num_fields > 2)
    expect_true(ogr_layer_test_cap(perims_shp, "mtbs_perims")$CreateField)
    sql <- "ALTER TABLE mtbs_perims ADD burn_bnd_ha float"
    expect_true(is.null(ogr_execute_sql(perims_shp, sql)))
    sql <- "UPDATE mtbs_perims SET burn_bnd_ha = (burn_bnd_ac / 2.471)"
    ogr_execute_sql(perims_shp, sql, dialect = "SQLite")
    expect_length(ogr_layer_field_names(perims_shp, "mtbs_perims"),
                  num_fields + 1)

    # spatial filter
    # this should filter to 40 features from 61
    bb <- c(469685.97, 11442.45, 544069.63, 85508.15)
    expect_true(is.null(ogr_execute_sql(perims_shp, sql,
                                        spatial_filter = bb)))
    bb_wkt <- bbox_to_wkt(bb)
    expect_true(is.null(ogr_execute_sql(perims_shp, sql,
                                        spatial_filter = bb_wkt)))
    # error for invalid bb
    bb_invalid <- c(469685.97, 11442.45)
    expect_error(is.null(ogr_execute_sql(perims_shp, sql,
                                         spatial_filter = bb_invalid)))

    deleteDataset(perims_shp)
})

test_that("GeoJSON layer and field names are correct", {
    dsn <- system.file("extdata/test.geojson", package="gdalraster")
    expect_true(ogr_ds_exists(dsn))
    dsn2 <- file.path(tempdir(TRUE), "test.geojson")
    file.copy(dsn, dsn2)
    expect_true(ogr_ds_exists(dsn2, with_update = TRUE))
    expect_equal(ogr_ds_format(dsn2), "GeoJSON")
    expect_equal(ogr_ds_layer_count(dsn2), 1)
    expect_equal(ogr_ds_layer_names(dsn2), "test")
    expect_equal(ogr_layer_field_names(dsn2, "test"),
                 c("int", "string", "double", "int2", ""))

    deleteDataset(dsn2)
})
