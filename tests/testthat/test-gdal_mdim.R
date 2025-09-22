skip_if(gdal_version_num() < gdal_compute_version(3, 2, 0))

test_that("mdim_as_classic works", {
    f <- system.file("extdata/byte.nc", package="gdalraster")

    ds <- mdim_as_classic(f, "Band1", 1, 0)
    expect_false(is.null(ds))
    expect_equal(ds$dim(), c(20, 20, 1))
    expect_equal(ds$getGeoTransform(),
                 c(440720.0, 60.0, 0.0, 3750120.0, 0.0, 60.0))

    # matches checksum from GDAL autotest:
    #   autotest/gdrivers/data/vrt/arraysource_array.vrt on byte.nc
    #   https://github.com/OSGeo/gdal/blob/a8f0531cc90bc1fb554da9f6a94d059dbb995f81/autotest/gdrivers/vrtmultidim.py#L1417
    #       ds = gdal.Open("data/vrt/arraysource_array.vrt")
    #       assert ds.GetRasterBand(1).Checksum() == 4855
    # and Python:
    #   from osgeo import gdal
    #   f = 'byte.nc'
    #   ds = gdal.OpenEx(f, gdal.OF_MULTIDIM_RASTER)
    #   rg = ds.GetRootGroup()
    #   ar = rg.OpenMDArray("Band1")
    #   classic_ds = ar.AsClassicDataset(1, 0)
    #   classic_ds.GetRasterBand(1).Checksum()
    #   # 4855
    expect_equal(ds$getChecksum(1, 0, 0, 20, 20), 4855)

    ds$close()
    # filename is not set / driver-less dataset:
    expect_error(ds$open(read_only = TRUE))

    # view expressions
    f2 <- tempfile(fileext = ".nc")
    on.exit(unlink(f2), add = TRUE)
    file.copy(f, f2, overwrite = TRUE)

    ds <- mdim_as_classic(f2, "Band1", 1, 0, read_only = FALSE,
                          view_expr = "[0:10,...]")

    # https://github.com/OSGeo/gdal/blob/2b1c4d1fdb9377b62d821d70d022e7426940aec6/autotest/gdrivers/netcdf_multidim.py#L4036
    expect_equal(ds$getStatistics(1, FALSE, TRUE),
                 c(74.0, 255.0, 126.82, 26.729713803182),
                 tolerance = 0.01)

    ds$close()

    ds <- mdim_as_classic(f2, "Band1", 1, 0, read_only = FALSE,
                          view_expr = "[10:20,...]")

    # https://github.com/OSGeo/gdal/blob/2b1c4d1fdb9377b62d821d70d022e7426940aec6/autotest/gdrivers/netcdf_multidim.py#L4042
    expect_equal(ds$getStatistics(1, FALSE, TRUE),
                 c(99.0, 206.0, 126.71, 18.356086184152),
                 tolerance = 0.01)

    ds$close()
})

test_that("mdim_info works", {
    f <- system.file("extdata/byte.nc", package="gdalraster")
    expect_no_error(info <- mdim_info(f, array_options = "SHOW_ALL=YES"))
    expect_vector(info, ptype = character(), size = 1)
    expect_true(startsWith(info, "{"))
    expect_true(nchar(info) > 1000)

    expect_no_error(
        info <- mdim_info(f, "Band1", pretty = FALSE, detailed = TRUE,
                        limit = 5, allowed_drivers = "netCDF",
                        open_options = "HONOUR_VALID_RANGE=NO"))

    expect_vector(info, ptype = character(), size = 1)
    expect_true(startsWith(info, "{"))
    expect_true(nchar(info) > 1000)
})
