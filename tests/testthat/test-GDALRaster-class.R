test_that("info() prints output to the console", {
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	ds <- new(GDALRaster, evt_file, TRUE)
	expect_output(ds$info())
	ds$close()
})

test_that("dataset parameters are correct", {
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	ds <- new(GDALRaster, evt_file, TRUE)
	expect_equal(ds$getDriverShortName(), "GTiff")
	expect_equal(ds$getDriverLongName(), "GeoTIFF")
	expect_equal(ds$getRasterXSize(), 143)
	expect_equal(ds$getRasterYSize(), 107)
	expect_equal(ds$getGeoTransform(),
					c(323476.1,30.0,0.0,5105082.0,0.0,-30.0))
	expect_equal(ds$bbox(), c(323476.1,5101872.0,327766.1,5105082.0))
	expect_equal(ds$res(), c(30,30))
	expect_equal(ds$dim(), c(143,107,1))
	ds$close()
})

test_that("band-level parameters are correct", {
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	ds <- new(GDALRaster, evt_file, TRUE)
	expect_equal(ds$getBlockSize(1), c(143,28))
	expect_equal(ds$getOverviewCount(1), 0)
	expect_equal(ds$getDataTypeName(1), "Int16")
	expect_equal(ds$getNoDataValue(1), 32767)
	ds$close()
})

test_that("metadata are correct", {
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	ds <- new(GDALRaster, evt_file, TRUE)
	expect_equal(ds$getMetadata(band=0, domain=""), "AREA_OR_POINT=Area")
	expect_equal(ds$getMetadata(band=1, domain=""), 
				"RepresentationType=ATHEMATIC")
	expect_equal(ds$getMetadata(band=0, domain="IMAGE_STRUCTURE"),
				"INTERLEAVE=BAND")
	expect_equal(ds$getMetadata(band=1, domain="IMAGE_STRUCTURE"), "")
	expect_equal(ds$getMetadataItem(band=0, mdi_name="AREA_OR_POINT",
				domain=""), "Area")
	ds$close()
})

