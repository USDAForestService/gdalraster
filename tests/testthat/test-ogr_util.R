test_that("OGR utility functions work on a vector data source", {
	dsn <- paste0(tempdir(), "/", "test.gpkg")
	expect_false(.ogr_ds_exists(dsn))
	layer <- "test_layer"
	expect_true(.create_ogr("GPKG", dsn, 0, 0, 0, "Unknown", layer))
	expect_true(.ogr_ds_exists(dsn, with_update=TRUE))
	expect_true(.ogr_layer_exists(dsn, layer))
	layer <- "test_layer_2"
	expect_false(.ogr_layer_exists(dsn, layer))
	lco <- "DESCRIPTION=gdalraster test"
	expect_true(.ogr_layer_create(dsn, layer, "EPSG:5070", lco))
	expect_true(.ogr_layer_exists(dsn, layer))
	expect_equal(.ogr_ds_layer_count(dsn), 2)
	expect_equal(.ogr_field_index(dsn, layer, "DN"), -1)
	expect_true(.ogr_field_create(dsn, layer, "DN"))
	expect_equal(.ogr_field_index(dsn, layer, "DN"), 0)
	expect_true(.ogr_ds_exists(dsn))
	expect_true(.ogr_layer_delete(dsn, layer))
	deleteDataset(dsn)
})
