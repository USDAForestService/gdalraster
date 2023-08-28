# Tests for src/gdalraster.cpp
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
	expect_equal(ds$getUnitType(1), "")
	expect_true(is.na(ds$getScale(1)))
	expect_true(is.na(ds$getOffset(1)))
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
	expect_equal(ds$getMetadataItem(band=1, mdi_name="COMPRESSION",
				domain="IMAGE_STRUCTURE"), "")
	ds$close()
})

test_that("open/close/re-open works", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	ds <- new(GDALRaster, elev_file, read_only=TRUE)
	dm <- ds$dim()
	r <- read_ds(ds)
	ds$close()
	mod_file <- paste0(tempdir(), "/", "storml_elev_mod.tif")
	on.exit(unlink(mod_file))
	rasterFromRaster(srcfile = elev_file,
					dstfile = mod_file,
					nbands = 1,
					dtName = "UInt32",
					init = DEFAULT_NODATA[["UInt32"]])
	ds <- new(GDALRaster, mod_file, read_only=TRUE)
	expect_true(ds$isOpen())
	expect_true(all(is.na(read_ds(ds))))
	ds$close()
	expect_false(ds$isOpen())
	expect_equal(ds$getFilename(), mod_file)
	ds$open(read_only=FALSE)
	expect_true(ds$isOpen())
	r[is.na(r)] <- DEFAULT_NODATA[["UInt32"]]
	ds$write(band=1, 0, 0, dm[1], dm[2], r)
	expect_false(all(is.na(read_ds(ds))))
	expect_true(any(is.na(read_ds(ds))))
	ds$close()
})

test_that("statistics are correct", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	mod_file <- paste0(tempdir(), "/", "storml_elev_mod.tif")
	file.copy(elev_file,  mod_file)
	on.exit(unlink(mod_file))
	ds <- new(GDALRaster, mod_file, read_only=TRUE)
	stats <- round(ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE))
	expect_equal(stats, c(2438, 3046, 2676, 133))
	ds$close()
})

test_that("floating point I/O works", {
	f <- paste0(tempdir(), "/", "testfloat.tif")
	on.exit(unlink(f))
	create(format="GTiff", dst_filename=f, xsize=10, ysize=10,
			nbands=1, dataType="Float32")
	ds <- new(GDALRaster, f, read_only=FALSE)
	set.seed(42)
	z <- runif(10*10)
	ds$write(band=1, xoff=0, yoff=0, xsize=10, ysize=10, z)
	ds$open(read_only=TRUE)
	expect_equal(round(read_ds(ds),5), round(z,5))
	ds$open(read_only=FALSE)
	ds$setNoDataValue(band=1, -99999)
	ds$write(band=1, xoff=0, yoff=0, xsize=10, ysize=10, rep(-99999, 100))
	ds$open(read_only=TRUE)
	expect_true(all(is.na(read_ds(ds))))
	ds$open(read_only=FALSE)
	ds$deleteNoDataValue(band=1)
	ds$open(read_only=TRUE)
	expect_equal(read_ds(ds), rep(-99999, 100))
	ds$close()
})

test_that("complex I/O works", {
	f <- paste0(tempdir(), "/", "testcomplex.tif")
	on.exit(unlink(f))
	create(format="GTiff", dst_filename=f, xsize=10, ysize=10,
			nbands=1, dataType="CFloat32")
	ds <- new(GDALRaster, f, read_only=FALSE)
	set.seed(42)
	z <- complex(real = stats::rnorm(100), imaginary = stats::rnorm(100))
	ds$write(band=1, xoff=0, yoff=0, xsize=10, ysize=10, z)
	ds$open(read_only=TRUE)
	expect_vector(read_ds(ds), ptype=complex(0), size=100)
	ds$close()
})

test_that("set unit type, scale and offset works", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	mod_file <- paste0(tempdir(), "/", "storml_elev_mod.tif")
	file.copy(elev_file,  mod_file)
	on.exit(unlink(mod_file))
	ds <- new(GDALRaster, mod_file, read_only=FALSE)
	ds$setUnitType(1, "m")
	ds$setScale(1, 1)
	ds$setOffset(1, 0)
	expect_equal(ds$getUnitType(1), "m")
	expect_equal(ds$getScale(1), 1)
	expect_equal(ds$getOffset(1), 0)
	ds$close()
})

test_that("build overviews runs without error", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	mod_file <- paste0(tempdir(), "/", "storml_elev.tif")
	file.copy(elev_file,  mod_file)
	on.exit(unlink(mod_file))
	ds <- new(GDALRaster, mod_file, read_only=FALSE)
	expect_no_error(ds$buildOverviews("BILINEAR", c(2,4,8), 0))
	expect_no_error(ds$buildOverviews("NONE", 0, 0))
	ds$close()
})
