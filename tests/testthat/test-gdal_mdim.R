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
    # and:
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
})
