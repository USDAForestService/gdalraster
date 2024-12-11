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
    expect_no_error(getCreationOptions("GTiff"))
    expect_output(x <- getCreationOptions("GTiff", filter="COMPRESS"))
    expect_type(x, "character")
    expect_message(getCreationOptions("AIG"))
})

test_that("dump_open_datasets works", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
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
    src <- system.file("extdata/storml_elev.tif", package="gdalraster")
    dsinfo <- inspectDataset(src)
    expect_equal(dsinfo$format, "GTiff")
    expect_true(dsinfo$supports_raster)
    expect_true(dsinfo$contains_raster)
    expect_true(dsinfo$supports_subdatasets)
    expect_false(dsinfo$contains_subdatasets)
    expect_false(dsinfo$supports_vector)
    expect_false(dsinfo$contains_vector)
    expect_vector(dsinfo$layer_names, ptype = character(), size = 0)
})
