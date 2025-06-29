test_that("OGR management utilities work", {
    # these tests assume GDAL was built with libsqlite3 which is optional
    # this should be a safe assumption since PROJ requires libsqlite3

    ## GPKG
    dsn <- tempfile(fileext = ".gpkg")
    expect_false(ogr_ds_exists(dsn))
    expect_true(ogr_ds_create("GPKG", dsn, "test_layer", geom_type = "POLYGON"))
    expect_true(ogr_ds_exists(dsn, with_update=TRUE))
    expect_true(ogr_layer_exists(dsn, "test_layer"))
    # default for 'overwrite' is FALSE
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

    # only one geom field is supported by GPKG
    geomfld2_defn <- ogr_def_geom_field("Polygon", srs = epsg_to_wkt(4326))
    expect_false(ogr_geom_field_create(dsn, "test_layer3", "geom2",
                                       geom_fld_defn = geomfld2_defn))
    expect_equal(ogr_layer_field_names(dsn, "test_layer3"),
                 c("field1", "field2", "field3", "field4", "field5", "field6",
                   "geom"))

    # rename layer
    if (gdal_version_num() >= gdal_compute_version(3, 5, 0)) {
        expect_true(ogr_layer_test_cap(dsn, "test_layer3")$Rename)
        expect_true(ogr_layer_rename(dsn, "test_layer3", "test_new_name"))
        expect_equal(ogr_layer_field_names(dsn, "test_new_name"),
                     c("field1", "field2", "field3", "field4", "field5",
                       "field6", "geom"))
    }

    # test input validation
    # ogr_ds_exists
    expect_error(ogr_ds_exists(NULL))
    expect_error(ogr_ds_exists(rep(dsn, 2)))
    expect_no_error(ogr_ds_exists(dsn, with_update = NA))
    expect_error(ogr_ds_exists(dsn, with_update = "TRUE"))
    # ogr_ds_format
    expect_error(ogr_ds_format(NULL))
    expect_error(ogr_ds_format(rep(dsn, 2)))
    # ogr_ds_test_cap
    expect_error(ogr_ds_test_cap(NULL))
    expect_error(ogr_ds_test_cap(rep(dsn, 2)))
    expect_no_error(ogr_ds_test_cap(dsn, with_update = NA))
    expect_error(ogr_ds_test_cap(dsn, with_update = "TRUE"))
    # ogr_ds_create
    expect_error(ogr_ds_create(NULL, dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               overwrite = TRUE))
    expect_error(ogr_ds_create(0, dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", NULL, "test_layer_0",
                               geom_type = "POLYGON",
                               overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", 0, "test_layer_0",
                               geom_type = "POLYGON",
                               overwrite = TRUE))
    expect_no_error(ogr_ds_create("GPKG", dsn, NULL,
                                  geom_type = "POLYGON",
                                  overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, 0,
                               geom_type = "POLYGON",
                               overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               layer_defn = NA,
                               overwrite = TRUE))
    expect_no_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                                  geom_type = NULL,
                                  overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = 0,
                               overwrite = TRUE))
    expect_no_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                                  geom_type = "POLYGON",
                                  srs = NULL,
                                  overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               srs = 0,
                               overwrite = TRUE))
    expect_no_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                                  geom_type = "POLYGON",
                                  fld_name = NULL,
                                  overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               fld_name = 0,
                               overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               fld_name = "test_fld_0",
                               fld_type = NULL,
                               overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               fld_name = "test_fld_0",
                               fld_type = 0,
                               overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               dsco = 0,
                               overwrite = TRUE))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               lco = 0,
                               overwrite = TRUE))
    # 'overwrite' defaults to FALSE so this should fail if 'dsn' exists:
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               overwrite = NULL))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               overwrite = 0))
    expect_no_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                                  geom_type = "POLYGON",
                                  overwrite = TRUE,
                                  return_obj = NULL))
    expect_error(ogr_ds_create("GPKG", dsn, "test_layer_0",
                               geom_type = "POLYGON",
                               overwrite = TRUE,
                               return_obj = 0))
    expect_error(ogr_ds_create("GPKG", dsn, "",
                               geom_type = "POLYGON",
                               overwrite = TRUE,
                               return_obj = TRUE))
    # ogr_ds_layer_count
    expect_error(ogr_ds_layer_count(NULL))
    expect_error(ogr_ds_layer_count(rep(dsn, 2)))
    # ogr_ds_layer_names
    expect_error(ogr_ds_layer_names(NULL))
    expect_error(ogr_ds_layer_names(rep(dsn, 2)))
    # field domain functions are also tested in test-GDALVector-class.R
    # ogr_ds_field_domain_names
    expect_error(ogr_ds_field_domain_names(NULL))
    expect_error(ogr_ds_field_domain_names(rep(dsn, 2)))
    # ogr_ds_add_field_domain
    expect_error(ogr_ds_add_field_domain(NULL, list(1)))
    expect_error(ogr_ds_add_field_domain(rep(dsn, 2), list(1)))
    expect_error(ogr_ds_add_field_domain(dsn, 0))
    # ogr_layer_exists
    expect_error(ogr_layer_exists(NULL, "test_layer_0"))
    expect_error(ogr_layer_exists(rep(dsn, 2), "test_layer_0"))
    expect_no_error(ogr_layer_exists(dsn, ""))
    expect_error(ogr_layer_exists(dsn, 0))
    # ogr_layer_test_cap
    expect_error(ogr_layer_test_cap(NULL, "test_layer_0"))
    expect_error(ogr_layer_test_cap(rep(dsn, 2), "test_layer_0"))
    expect_no_error(ogr_layer_test_cap(dsn, ""))
    expect_error(ogr_layer_test_cap(dsn, 0))
    expect_no_error(ogr_layer_test_cap(dsn, "", with_update = NA))
    expect_error(ogr_layer_test_cap(dsn, "", with_update = "TRUE"))
    # ogr_layer_create
    expect_error(ogr_layer_create(NULL, "test_layer_0"))
    expect_error(ogr_layer_create(rep(dsn, 2), "test_layer_0"))
    expect_error(ogr_layer_create(dsn, ""))
    expect_error(ogr_layer_create(dsn, 1))
    expect_warning(ogr_layer_create(dsn, "test_layer_000"))
    # already exists:
    expect_error(ogr_layer_create(dsn, "test_layer_000"))
    expect_error(ogr_layer_create(dsn, "test_layer_001", layer_defn = 0))
    expect_error(ogr_layer_create(dsn, "test_layer_001", geom_type = 0))
    expect_warning(ogr_layer_create(dsn, "test_layer_001", geom_type = NULL))
    expect_warning(ogr_layer_create(dsn, "test_layer_002", srs = NULL))
    expect_error(ogr_layer_create(dsn, "test_layer_002", srs = 0))
    expect_error(ogr_layer_create(dsn, "test_layer_002", lco = 0))
    expect_error(ogr_layer_create(dsn, "test_layer_002", return_obj = NULL))
    expect_error(ogr_layer_create(dsn, "test_layer_002", return_obj = 0))
    # ogr_layer_field_names
    expect_error(ogr_layer_field_names(NULL, "test_layer_0"))
    expect_error(ogr_layer_field_names(rep(dsn, 2), "test_layer_0"))
    expect_no_error(ogr_layer_field_names(dsn, NULL))
    expect_error(ogr_layer_field_names(dsn, 0))
    # ogr_layer_rename
    if (gdal_version_num() >= gdal_compute_version(3, 5, 0)) {
        expect_error(ogr_layer_field_names(NULL, "test_layer_0", "new"))
        expect_error(ogr_layer_field_names(rep(dsn, 2), "test_layer_0", "new"))
        expect_error(ogr_layer_field_names(dsn, "", "new"))
        expect_error(ogr_layer_field_names(dsn, 0, "new"))
        expect_error(ogr_layer_field_names(dsn, "test_layer_0", ""))
        expect_error(ogr_layer_field_names(dsn, "test_layer_0", 0))
    }
    # ogr_layer_delete
    expect_error(ogr_layer_delete(NULL, "test_layer_0"))
    expect_error(ogr_layer_delete(rep(dsn, 2), "test_layer_0"))
    expect_error(ogr_layer_delete(dsn, NULL))
    expect_error(ogr_layer_delete(dsn, 0))
    # ogr_field_index
    expect_error(ogr_layer_delete(NULL, "test_layer_0", "field0"))
    expect_error(ogr_layer_delete(rep(dsn, 2), "test_layer_0", "field0"))
    expect_error(ogr_layer_delete(dsn, 0, "field0"))
    expect_error(ogr_layer_delete(dsn, "test_layer_0", NA))
    expect_error(ogr_layer_delete(dsn, "test_layer_0", 0))
    # ogr_field_create
    fld_defn_0 <- ogr_def_field("OFTReal")
    expect_true(ogr_layer_exists(dsn, "test_layer_0"))
    expect_error(ogr_field_create(NULL, "test_layer_0", "field0", fld_defn_0))
    expect_error(ogr_field_create(0, "test_layer_0", "field0", fld_defn_0))
    expect_error(ogr_field_create(dsn, 0, "field0", fld_defn_0))
    expect_error(ogr_field_create(dsn, "invalid", "field0", fld_defn_0))
    expect_error(ogr_field_create(dsn, "test_layer_0", NULL, fld_defn_0))
    expect_error(ogr_field_create(dsn, "test_layer_0", 0, fld_defn_0))
    expect_error(ogr_field_create(dsn, "test_layer_0", "field0",
                                  fld_type = "OFTReal",
                                  default_value = c(0, 1)))
    expect_error(ogr_field_create(dsn, "test_layer_0", "field0",
                                  fld_type = "OFTReal",
                                  domain_name = 0))
    expect_true(ogr_field_create(dsn, "test_layer_0", "field0", fld_defn_0))
    # already exists:
    expect_false(ogr_field_create(dsn, "test_layer_0", "field0", fld_defn_0))
    # ogr_field_rename
    expect_error(ogr_field_rename(NULL, "test_layer_0", "field0", "new0"))
    expect_error(ogr_field_rename(0, "test_layer_0", "field0", "new0"))
    expect_error(ogr_field_rename(dsn, 0, "field0", "new0"))
    expect_error(ogr_field_rename(dsn, "invalid", "field0", "new0"))
    expect_error(ogr_field_rename(dsn, "test_layer_0", NA, "new0"))
    expect_error(ogr_field_rename(dsn, "test_layer_0", 0, "new0"))
    expect_false(ogr_field_rename(dsn, "test_layer_0", "invalid", "new0"))
    expect_error(ogr_field_rename(dsn, "test_layer_0", "field0", ""))
    expect_error(ogr_field_rename(dsn, "test_layer_0", "field0", 0))
    expect_no_error(ogr_field_rename(dsn, "test_layer_0", "field0", "new0"))
    # ogr_field_set_domain_name
    expect_error(ogr_field_set_domain_name(NULL, "test_layer_0", "new0",
                                           "domain0"))
    expect_error(ogr_field_set_domain_name(0, "test_layer_0", "new0",
                                           "domain0"))
    expect_error(ogr_field_set_domain_name(dsn, 0, "new0",
                                           "domain0"))
    expect_error(ogr_field_set_domain_name(dsn, "invalid", "new0",
                                           "domain0"))
    expect_error(ogr_field_set_domain_name(dsn, "test_layer_0", NULL,
                                           "domain0"))
    expect_error(ogr_field_set_domain_name(dsn, "test_layer_0", 0,
                                           "domain0"))
    expect_false(ogr_field_set_domain_name(dsn, "test_layer_0", "invalid",
                                           "domain0"))
    expect_error(ogr_field_set_domain_name(dsn, "test_layer_0", "new0",
                                           NULL))
    expect_error(ogr_field_set_domain_name(dsn, "test_layer_0", "new0",
                                           character(0)))
    # ogr_field_delete
    expect_error(ogr_field_delete(NULL, "test_layer_0", "new0"))
    expect_error(ogr_field_delete(0, "test_layer_0", "new0"))
    expect_error(ogr_field_delete(dsn, 0, "new0"))
    expect_error(ogr_field_delete(dsn, "invalid", "new0"))
    expect_error(ogr_field_delete(dsn, "test_layer_0", ""))
    expect_error(ogr_field_delete(dsn, "test_layer_0", 0))
    # ogr_execute_sql
    sql <- "SELECT * FROM test_layer_0"
    expect_error(ogr_execute_sql(NULL, sql))
    expect_error(ogr_execute_sql(0, sql))
    expect_error(ogr_execute_sql(dsn, NULL))
    expect_error(ogr_execute_sql(dsn, character(0)))
    expect_error(ogr_execute_sql(dsn, sql, spatial_filter = numeric(0)))
    expect_error(ogr_execute_sql(dsn, sql, dialect = character(0)))
    expect_no_error(ogr_execute_sql(dsn, sql))

    deleteDataset(dsn)

    ## SQLite
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

    # input errors for ogr_geom_field_create()
    geomfld3_defn <- ogr_def_geom_field("Polygon", srs = epsg_to_wkt(4326))
    expect_error(ogr_geom_field_create(NULL, "layer1", "geom3",
                                       geom_fld_defn = geomfld3_defn))
    expect_error(ogr_geom_field_create(0, "layer1", "geom3",
                                       geom_fld_defn = geomfld3_defn))
    expect_error(ogr_geom_field_create(dsn, 0, "geom3",
                                       geom_fld_defn = geomfld3_defn))
    expect_error(ogr_geom_field_create(dsn, "invalid", "geom3",
                                       geom_fld_defn = geomfld3_defn))
    expect_error(ogr_geom_field_create(dsn, "layer1", NULL,
                                       geom_fld_defn = geomfld3_defn))
    expect_error(ogr_geom_field_create(dsn, "layer1", 0,
                                       geom_fld_defn = geomfld3_defn))
    expect_error(ogr_geom_field_create(dsn, "layer1", "geom3",
                                       geom_type = 0))
    expect_error(ogr_geom_field_create(dsn, "layer1", "geom3",
                                       geom_type = "Polygon",
                                       srs = 0))
    expect_error(ogr_geom_field_create(dsn, "layer1", "geom3",
                                       geom_type = "Polygon",
                                       is_nullable = 0))

    deleteDataset(dsn)

    # create with two geom fields initially with geom2 nullable
    dsn <- tempfile(fileext = "sqlite")
    defn$geom2 <- ogr_def_geom_field("Polygon", srs = epsg_to_wkt(4326),
                                     is_nullable = TRUE)
    expect_true(ogr_ds_create("SQLite", dsn, "layer1",
                              layer_defn = defn))
    expect_equal(ogr_layer_field_names(dsn, "layer1"),
                 c("field1", "field2", "field3", "GEOMETRY", "geom2"))

    deleteDataset(dsn)

    ## single-layer format, layer argument may be NULL or ""
    dsn <- system.file("extdata/poly_multipoly.shp", package="gdalraster")
    expect_equal(ogr_layer_field_names(dsn),
                 c("Event_ID", "Map_ID", "Ig_Date",  ""))

})

test_that("create Memory format works", {
    defn <- ogr_def_layer("Point", srs = epsg_to_wkt(4269))
    defn$int_field <- ogr_def_field("OFTInteger")
    defn$str_field <- ogr_def_field("OFTString")
    defn$real_field <- ogr_def_field("OFTReal")
    defn$dt_field <- ogr_def_field("OFTDateTime")

    expect_no_error(lyr <- ogr_ds_create("Memory", "", "mem_layer",
                                         layer_defn = defn,
                                         lco = "WRITE_BBOX=YES",
                                         overwrite = TRUE,
                                         return_obj = TRUE))

    expect_equal(lyr$getDriverShortName(), "Memory")
    expect_equal(lyr$getFieldNames() |> length(), 5)

    lyr$close()
})

test_that("edit data using SQL works on shapefile", {
    src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    perims_shp <- paste0(tempdir(), "/", "mtbs_perims.shp")
    ogr2ogr(src, perims_shp, src_layers = "mtbs_perims")
    expect_equal(ogr_ds_layer_names(perims_shp), "mtbs_perims")
    num_fields <- length(ogr_layer_field_names(perims_shp, "mtbs_perims"))
    expect_true(num_fields > 2)
    expect_true(ogr_layer_test_cap(perims_shp, "mtbs_perims")$CreateField)
    # adjust field names for shapefile limitation on length
    sql <- "ALTER TABLE mtbs_perims ADD burn_bnd_h float"
    expect_true(is.null(ogr_execute_sql(perims_shp, sql)))
    sql <- "UPDATE mtbs_perims SET burn_bnd_h = (burn_bnd_a / 2.471)"
    ogr_execute_sql(perims_shp, sql, dialect = "SQLite")
    expect_length(ogr_layer_field_names(perims_shp, "mtbs_perims"),
                  num_fields + 1)

    # spatial filter
    # this should filter to 40 features from 61
    sql <- "SELECT * FROM mtbs_perims"
    bb <- c(469685.97, 11442.45, 544069.63, 85508.15)
    expect_no_error(lyr <- ogr_execute_sql(perims_shp, sql,
                                           spatial_filter = bb))
    expect_true(is(lyr, "Rcpp_GDALVector"))
    expect_equal(lyr$getFeatureCount(), 40)
    feat <- lyr$getNextFeature()
    expect_equal(feat$burn_bnd_h, (feat$burn_bnd_a / 2.471), tolerance = 0.1)
    lyr$close()

    bb_wkt <- bbox_to_wkt(bb)
    expect_no_error(lyr <- ogr_execute_sql(perims_shp, sql,
                                           spatial_filter = bb_wkt))
    expect_true(is(lyr, "Rcpp_GDALVector"))
    expect_equal(lyr$getFeatureCount(), 40)
    lyr$close()

    # error for invalid bb
    bb_invalid <- c(469685.97, 11442.45)
    expect_error(is.null(ogr_execute_sql(perims_shp, sql,
                                         spatial_filter = bb_invalid)))

    deleteDataset(perims_shp)
})

test_that("ogr_execute_sql returns results set for SELECT", {
    dsn <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    sql <- "SELECT FID, * FROM mtbs_perims WHERE ig_year >= 2000 ORDER BY mtbs_perims.ig_year"
    lyr <- ogr_execute_sql(dsn, sql)
    expect_equal(lyr$getFeatureCount(), 40)
    lyr$close()

    sql <- "SELECT * FROM mtbs_perims"
    lyr2 <- ogr_execute_sql(dsn, sql, spatial_filter = c(469685.97, 11442.45, 544069.63, 85508.15))
    expect_equal(lyr2$getFeatureCount(), 40)
    lyr2$close()

    lyr3 <- ogr_execute_sql(dsn, sql)
    expect_equal(lyr3$getFeatureCount(), 61)
    lyr3$close()
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
