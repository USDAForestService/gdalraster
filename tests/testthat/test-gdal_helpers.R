# apply_geotransform() and get_pixel_line() tests are in test-gdal_exp.R

test_that("gdal_compute_version works", {
    expect_equal(gdal_compute_version(3, 7, 0), 3070000L)
    expect_error(gdal_compute_version("3", 7, 0))
    expect_error(gdal_compute_version(3, "7", 0))
    expect_error(gdal_compute_version(3, 7, NULL))
})

test_that("addFilesInZip works", {
    # requires GDAL >= 3.7
    skip_if(as.integer(gdal_version()[2]) < 3070000)

    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
    evh_file <- system.file("extdata/storml_evh.tif", package="gdalraster")
    files_to_add <- c(evt_file, evc_file, evh_file)
    zip_file <- paste0(tempdir(), "/", "storml.zip")
    addFilesInZip(
            zip_file,
            files_to_add,
            full_paths=FALSE,
            sozip_enabled="YES",
            sozip_chunk_size=16384,
            sozip_min_file_size=1024,
            content_type="TEST",
            num_threads=1)
    d <- unzip(zip_file, list=TRUE)
    expect_equal(nrow(d), 3)
    unlink(zip_file)
})

test_that("getCreationOptions works", {
    opt <- getCreationOptions("GTiff", "COMPRESS")
    expect_true(is.list(opt))
    expect_equal(names(opt), "COMPRESS")
    expect_equal(opt$COMPRESS$type, "string-select")
    expect_vector(opt$COMPRESS$values, ptype = character())
    all_opt <- getCreationOptions("GTiff")
    expect_true(is.list(all_opt))
    expect_true(length(names(all_opt)) > 10)
    expect_true(is.list(all_opt$TILED))
    expect_error(getCreationOptions("invalid format name"))
    expect_error(getCreationOptions(NA))

})

test_that("dump_open_datasets works", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file)
    expect_output(dump_open_datasets())
    expect_true(dump_open_datasets() > 0)
    ds$close()
})

test_that("inspectDataset works", {
    # GPKG with subdatasets
    # https://download.osgeo.org/gdal/data/geopackage/small_world_and_byte.gpkg
    src <-  system.file("extdata/small_world_and_byte.gpkg", package="gdalraster")
    dsinfo <- inspectDataset(src)
    expect_equal(dsinfo$format, "GPKG")
    expect_true(dsinfo$supports_raster)
    expect_true(dsinfo$contains_raster)
    expect_true(dsinfo$supports_subdatasets)
    expect_true(dsinfo$contains_subdatasets)
    expect_no_error(ds <- new(GDALRaster, dsinfo$subdataset_names[1]))
    expect_no_error(ds$close())
    expect_no_error(ds <- new(GDALRaster, dsinfo$subdataset_names[2]))
    expect_no_error(ds$close())
    expect_true(dsinfo$supports_vector)
    expect_false(dsinfo$contains_vector)
    expect_vector(dsinfo$layer_names, ptype = character(), size = 0)

    # GPKG with vector
    src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    dsinfo <- inspectDataset(src)
    expect_false(dsinfo$contains_raster)
    expect_false(dsinfo$contains_subdatasets)
    expect_vector(dsinfo$subdataset_names, ptype = character(), size = 0)
    expect_true(dsinfo$contains_vector)
    expect_equal(dsinfo$layer_names, "mtbs_perims")

    # shapefile
    src <- system.file("extdata/poly_multipoly.shp", package="gdalraster")
    dsinfo <- inspectDataset(src)
    expect_equal(dsinfo$format, "ESRI Shapefile")
    expect_false(dsinfo$supports_raster)
    expect_false(dsinfo$contains_raster)
    expect_false(dsinfo$supports_subdatasets)
    expect_false(dsinfo$contains_subdatasets)
    expect_true(dsinfo$supports_vector)
    expect_true(dsinfo$contains_vector)
    expect_vector(dsinfo$layer_names, ptype = character(), size = 1)

    # GTiff
    src <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    dsinfo <- inspectDataset(src)
    expect_equal(dsinfo$format, "GTiff")
    expect_true(dsinfo$supports_raster)
    expect_true(dsinfo$contains_raster)
    expect_true(dsinfo$supports_subdatasets)
    expect_false(dsinfo$contains_subdatasets)
    expect_false(dsinfo$supports_vector)
    expect_false(dsinfo$contains_vector)
    expect_vector(dsinfo$layer_names, ptype = character(), size = 0)

    # PostGISRaster / PostgreSQL
    skip_if(nrow(gdal_formats("PostgreSQL")) == 0 ||
            nrow(gdal_formats("PostGISRaster")) == 0)

    dsn <- "PG:dbname='testdb', host='127.0.0.1' port='5444' user='user'
            password='pwd'"
    dsinfo <- inspectDataset(dsn)
    expect_equal(dsinfo$format, "PostgreSQL")
    expect_false(dsinfo$supports_raster)
    expect_false(dsinfo$contains_raster)
    expect_false(dsinfo$supports_subdatasets)
    expect_false(dsinfo$contains_subdatasets)
    expect_true(dsinfo$supports_vector)
    expect_false(dsinfo$contains_vector)
    expect_vector(dsinfo$layer_names, ptype = character(), size = 0)

    dsn <- "PG:dbname='testdb', host='127.0.0.1' port='5444' table='raster_tbl'
            column='raster_col' user='user' password='pwd'"
    dsinfo <- inspectDataset(dsn, vector = FALSE)
    expect_equal(dsinfo$format, "PostGISRaster")
    expect_true(dsinfo$supports_raster)
    expect_false(dsinfo$contains_raster)
    expect_true(dsinfo$supports_subdatasets)
    expect_false(dsinfo$contains_subdatasets)
    expect_false(dsinfo$supports_vector)
    expect_false(dsinfo$contains_vector)
    expect_vector(dsinfo$layer_names, ptype = character(), size = 0)
})

test_that("make_chunk_index works", {
    # the internal C++ function being called is tested in
    # tests/testthat/test-GDALRaster-class.R

    chunks <- make_chunk_index(raster_xsize = 156335, raster_ysize = 101538,
                               block_xsize = 256, block_ysize = 256,
                               gt = c(-2362395, 30, 0, 3267405, 0, -30),
                               max_pixels = 256 * 256 * 16)

    expect_equal(nrow(chunks), 15483)
})
