test_that("OGR management utilities work", {
    # GPKG
    dsn <- paste0(tempdir(), "/", "test.gpkg")
    expect_false(ogr_ds_exists(dsn))
    expect_true(ogr_ds_create("GPKG", dsn, "test_layer", geom_type = "POLYGON"))
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
    defn$field3 <- ogr_def_field("OFTReal",
                                 default_value = "0.0")
    expect_true(ogr_layer_create(dsn, "test_layer3", layer_defn = defn))
    expect_equal(ogr_ds_layer_names(dsn), "test_layer3")
    expect_equal(ogr_layer_field_names(dsn, "test_layer3"),
                 c("field1", "field2", "field3", "geom"))

    deleteDataset(dsn)
})

test_that("edit data using SQL works on shapefile", {
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
    # TODO(ctoney): confirm data once vector I/O is implemented

    deleteDataset(perims_shp)
})

test_that("GeoJSON layer and field names are correct", {
    dsn <- system.file("extdata/test.geojson", package="gdalraster")
    expect_true(ogr_ds_exists(dsn, with_update = TRUE))
    expect_equal(ogr_ds_format(dsn), "GeoJSON")
    expect_equal(ogr_ds_layer_count(dsn), 1)
    expect_equal(ogr_ds_layer_names(dsn), "test")
    expect_equal(ogr_layer_field_names(dsn, "test"),
                 c("int", "string", "double", "int2", ""))
})
