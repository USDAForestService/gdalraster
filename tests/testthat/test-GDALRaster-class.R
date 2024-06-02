# Tests for src/gdalraster.cpp
test_that("info() prints output to the console", {
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    ds <- new(GDALRaster, evt_file, TRUE)
    expect_output(ds$info())
    # with args
    ds$infoOptions <- c("-nomd", "-norat", "-noct")
    expect_output(ds$info())
    # with invalid arg
    ds$infoOptions <- "--invalid_arg"
    expect_error(ds$info())
    # restore default
    ds$infoOptions <- ""
    expect_output(ds$info())
    ds$close()
})

test_that("infoAsJSON() returns string output", {
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    tmp_file <- paste0(tempdir(), "/", "storml_evt_tmp.tif")
    file.copy(evt_file,  tmp_file)
    ds <- new(GDALRaster, tmp_file, TRUE)
    expect_vector(ds$infoAsJSON(), character(), size = 1)
    expect_true(nchar(ds$infoAsJSON()) > 100)
    # test passing extra `-json`
    ds$infoOptions <- "-json"
    expect_vector(ds$infoAsJSON(), character(), size = 1)
    expect_true(nchar(ds$infoAsJSON()) > 100)

    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()
})

test_that("dataset parameters are correct", {
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    ds <- new(GDALRaster, lcp_file)
    expect_length(ds$getFileList(), 2)
    ds$close()
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    ds <- new(GDALRaster, evt_file)
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

    # using dataset open options
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    oo <- "NUM_THREADS=2"
    ds_oo <- new(GDALRaster, elev_file, TRUE, oo)
    expect_equal(ds_oo$getDriverShortName(), "GTiff")
    expect_equal(ds_oo$getDriverLongName(), "GeoTIFF")
    expect_equal(ds_oo$getRasterXSize(), 143)
    expect_equal(ds_oo$getRasterYSize(), 107)
    expect_equal(ds_oo$getGeoTransform(),
                 c(323476.1,30.0,0.0,5105082.0,0.0,-30.0))
    expect_equal(ds_oo$bbox(), c(323476.1,5101872.0,327766.1,5105082.0))
    expect_equal(ds_oo$res(), c(30,30))
    expect_equal(ds_oo$dim(), c(143,107,1))
    ds_oo$close()

    # without using shared mode
    ds_ns <- new(GDALRaster, elev_file, TRUE, NULL, FALSE)
    expect_equal(ds_ns$getDriverShortName(), "GTiff")
    ds_ns$close()
})

test_that("band-level parameters are correct", {
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    ds <- new(GDALRaster, evt_file, TRUE)
    expect_equal(ds$getBlockSize(1), c(143,28))
    expect_equal(ds$getActualBlockSize(1, 0, 0), c(143,28))
    expect_equal(ds$getOverviewCount(1), 0)
    expect_equal(ds$getDataTypeName(1), "Int16")
    expect_equal(ds$getNoDataValue(1), 32767)
    expect_equal(ds$getUnitType(1), "")
    expect_true(is.na(ds$getScale(1)))
    expect_true(is.na(ds$getOffset(1)))
    expect_equal(ds$getDescription(1), "")
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
                                    domain=""),
                 "Area")
    expect_equal(ds$getMetadataItem(band=0, "INTERLEAVE",
                                    domain="IMAGE_STRUCTURE"),
                 "BAND")
    expect_equal(ds$getMetadataItem(band=1, mdi_name="COMPRESSION",
                                    domain="IMAGE_STRUCTURE"),
                 "")
    expect_equal(length(ds$getMetadataDomainList(band=0)), 3)
    expect_equal(length(ds$getMetadataDomainList(band=1)), 1)
    ds$close()
})

test_that("open/close/re-open works", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file, read_only=TRUE)
    dm <- ds$dim()
    r <- read_ds(ds)
    ds$close()
    mod_file <- paste0(tempdir(), "/", "storml_elev_mod.tif")
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
    expect_equal(ds$getFilename(), .check_gdal_filename(mod_file))
    ds$open(read_only=FALSE)
    expect_true(ds$isOpen())
    ds$setDescription(band=1, "test")
    expect_equal(ds$getDescription(band=1), "test")
    r[is.na(r)] <- DEFAULT_NODATA[["UInt32"]]
    ds$write(band=1, 0, 0, dm[1], dm[2], r)
    expect_silent(ds$flushCache())
    expect_false(all(is.na(read_ds(ds))))
    expect_true(any(is.na(read_ds(ds))))
    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()

    ds <- new(GDALRaster)
    ds$setFilename(elev_file)
    ds$open(TRUE)
    expect_equal(ds$dim(), dm)
    ds$close()
})

test_that("statistics are correct", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    mod_file <- paste0(tempdir(), "/", "storml_elev_mod.tif")
    file.copy(elev_file,  mod_file)
    ds <- new(GDALRaster, mod_file, read_only=FALSE)
    expect_equal(ds$getMinMax(band=1, approx_ok=FALSE), c(2438, 3046))
    stats <- round(ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE))
    expect_equal(stats, c(2438, 3046, 2676, 133))
    if (as.integer(gdal_version()[2]) >= 3020000) {
        ds$flushCache()
        ds$clearStatistics()
        ds$flushCache()
        stats <- round(ds$getStatistics(band=1, approx_ok=TRUE, force=FALSE))
        expect_true(all(is.na(stats)))
    }

    # quiet
    stats <- NULL
    ds$quiet <- TRUE
    expect_silent(stats <- ds$getStatistics(band=1, approx_ok=FALSE,
                                            force=TRUE))
    expect_equal(round(stats), c(2438, 3046, 2676, 133))

    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()
})

test_that("get histogram works", {
    tcc_file <- system.file("extdata/storml_tcc.tif", package="gdalraster")
    f <- paste0(tempdir(), "/", "storml_tcc_test.tif")
    file.copy(tcc_file,  f)
    ds <- new(GDALRaster, f, read_only=TRUE)
    num_bins <- length(ds$getHistogram(1, -0.5, 100.5, 101, FALSE, FALSE))
    expect_equal(num_bins, 101)
    num_pixels <- sum(ds$getHistogram(1, -0.5, 100.5, 101, FALSE, FALSE))
    expect_equal(num_pixels, 15301)
    # quiet
    num_pixels <- 0
    ds$quiet <- TRUE
    expect_silent(num_pixels <- sum(ds$getHistogram(1, -0.5, 100.5, 101,
                                                    FALSE, FALSE)))
    expect_equal(num_pixels, 15301)

    ds$close()
    deleteDataset(f)

    # default histogram
    f2 <- paste0(tempdir(), "/", "storml_tcc_test2.tif")
    file.copy(tcc_file,  f2)
    ds <- new(GDALRaster, f2, read_only=TRUE)
    expect_warning(ds$getDefaultHistogram(1, FALSE))
    expect_length(ds$getDefaultHistogram(1, TRUE), 4)
    ds$close()
    deleteDataset(f2)
})

test_that("floating point I/O works", {
    f <- paste0(tempdir(), "/", "testfloat.tif")
    create(format="GTiff", dst_filename=f, xsize=10, ysize=10,
           nbands=1, dataType="Float32")
    ds <- new(GDALRaster, f, read_only=FALSE)
    set.seed(42)
    z <- runif(10*10)
    ds$write(band=1, xoff=0, yoff=0, xsize=10, ysize=10, z)
    ds$open(read_only=TRUE)
    expect_warning(r <- read_ds(ds))
    attributes(r) <- NULL
    expect_equal(round(r,5), round(z,5))
    ds$open(read_only=FALSE)
    ds$setNoDataValue(band=1, -99999)
    ds$write(band=1, xoff=0, yoff=0, xsize=10, ysize=10, rep(-99999, 100))
    ds$open(read_only=TRUE)
    expect_warning(r <- read_ds(ds))
    expect_true(all(is.na(r)))
    ds$open(read_only=FALSE)
    ds$deleteNoDataValue(band=1)
    ds$open(read_only=TRUE)
    expect_warning(r <- read_ds(ds))
    attributes(r) <- NULL
    expect_equal(r, rep(-99999, 100))
    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()
})

test_that("complex I/O works", {
    f <- paste0(tempdir(), "/", "testcomplex.tif")
    create(format="GTiff", dst_filename=f, xsize=10, ysize=10,
           nbands=1, dataType="CFloat32")
    ds <- new(GDALRaster, f, read_only=FALSE)
    set.seed(42)
    z <- complex(real = stats::rnorm(100), imaginary = stats::rnorm(100))
    ds$write(band=1, xoff=0, yoff=0, xsize=10, ysize=10, z)
    ds$open(read_only=TRUE)
    expect_warning(r <- read_ds(ds))
    attributes(r) <- NULL
    expect_vector(r, ptype=complex(0), size=100)
    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()
})

test_that("Byte I/O works", {
    f <- paste0(tempdir(), "/", "testbyte.tif")
    create(format="GTiff", dst_filename=f, xsize=10, ysize=10,
           nbands=1, dataType="Byte")
    ds <- new(GDALRaster, f, read_only=FALSE)
    set.seed(42)
    z <- as.raw(sample(100, 100))
    ds$write(band=1, xoff=0, yoff=0, xsize=10, ysize=10, z)

    ds$open(read_only=TRUE)
    expect_false(ds$readByteAsRaw)
    ## read as raw with the temporary setting
    expect_warning(r_raw <- read_ds(ds, as_raw = TRUE))
    ## read with to-integer conversion (as default, not affected by 'as_raw')
    expect_warning(r_int <- read_ds(ds))
    ## set and read as raw via field
    ds$readByteAsRaw <- TRUE
    expect_warning(r_raw1 <- read_ds(ds, as_raw = TRUE))

    deleteDataset(f)

    expect_type(r_int, "integer")
    expect_type(r_raw, "raw")
    expect_type(r_raw1, "raw")
    attributes(r_raw) <- NULL
    expect_equal(r_raw, z)

    ds$close()
})

test_that("Byte I/O: warn when data type not compatible", {
    ## expect a warning when data type is not Byte
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file)
    expect_warning(read_ds(ds, 1L, 0L, 0L, 2L, 3L, 2L, 3L, as_raw = TRUE))
    ds$close()
})

test_that("set unit type, scale and offset works", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    mod_file <- paste0(tempdir(), "/", "storml_elev_mod.tif")
    file.copy(elev_file,  mod_file)
    ds <- new(GDALRaster, mod_file, read_only=FALSE)
    ds$setUnitType(1, "m")
    ds$setScale(1, 1)
    ds$setOffset(1, 0)
    expect_equal(ds$getUnitType(1), "m")
    expect_equal(ds$getScale(1), 1)
    expect_equal(ds$getOffset(1), 0)
    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()
})

test_that("build overviews runs without error", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    mod_file <- paste0(tempdir(), "/", "storml_elev.tif")
    file.copy(elev_file,  mod_file)
    ds <- new(GDALRaster, mod_file, read_only=FALSE)
    expect_no_error(ds$buildOverviews("BILINEAR", c(2,4,8), 0))
    expect_no_error(ds$buildOverviews("NONE", 0, 0))
    # quiet
    ds$quiet <- TRUE
    expect_silent(ds$buildOverviews("BILINEAR", c(2,4,8), 0))
    expect_silent(ds$buildOverviews("NONE", 0, 0))

    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()
})

test_that("get/set color table works", {
    f <- system.file("extdata/storml_evc.tif", package="gdalraster")
    f2 <- paste0(tempdir(), "/", "storml_evc_ct.tif")
    calc("A", f, dstfile=f2, dtName="UInt16", nodata_value=32767,
         setRasterNodataValue=TRUE)
    ds <- new(GDALRaster, f2, read_only=FALSE)
    evc_csv <- system.file("extdata/LF20_EVC_220.csv", package="gdalraster")
    vat <- read.csv(evc_csv)
    ct <- vat[,c(1,3:5)]
    expect_warning(ds$setColorTable(1, ct, "RGB"))
    evc_ct <- ds$getColorTable(1)
    expect_equal(nrow(evc_ct), 400)
    expect_equal(ds$getPaletteInterp(1), "RGB")
    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()
})

test_that("get/set band color interpretation works", {
    f <- paste0(tempdir(), "/", "test_col_interp.tif")
    create(format="GTiff", dst_filename=f, xsize=10, ysize=10,
            nbands=1, dataType="Byte")
    ds <- new(GDALRaster, f, read_only=FALSE)
    expect_equal(ds$getRasterColorInterp(1), "Gray")
    ds$setRasterColorInterp(1, "Palette")
    expect_equal(ds$getRasterColorInterp(1), "Palette")
    ds$setRasterColorInterp(1, "Red")
    expect_equal(ds$getRasterColorInterp(1), "Red")
    ds$setRasterColorInterp(1, "Green")
    expect_equal(ds$getRasterColorInterp(1), "Green")
    ds$setRasterColorInterp(1, "Blue")
    expect_equal(ds$getRasterColorInterp(1), "Blue")
    ds$setRasterColorInterp(1, "Alpha")
    expect_equal(ds$getRasterColorInterp(1), "Alpha")
    ds$setRasterColorInterp(1, "Hue")
    expect_equal(ds$getRasterColorInterp(1), "Hue")
    ds$setRasterColorInterp(1, "Saturation")
    expect_equal(ds$getRasterColorInterp(1), "Saturation")
    ds$setRasterColorInterp(1, "Lightness")
    expect_equal(ds$getRasterColorInterp(1), "Lightness")
    ds$setRasterColorInterp(1, "Cyan")
    expect_equal(ds$getRasterColorInterp(1), "Cyan")
    ds$setRasterColorInterp(1, "Magenta")
    expect_equal(ds$getRasterColorInterp(1), "Magenta")
    ds$setRasterColorInterp(1, "Yellow")
    expect_equal(ds$getRasterColorInterp(1), "Yellow")
    ds$setRasterColorInterp(1, "Black")
    expect_equal(ds$getRasterColorInterp(1), "Black")
    ds$setRasterColorInterp(1, "YCbCr_Y")
    expect_equal(ds$getRasterColorInterp(1), "YCbCr_Y")
    ds$setRasterColorInterp(1, "YCbCr_Cb")
    expect_equal(ds$getRasterColorInterp(1), "YCbCr_Cb")
    ds$setRasterColorInterp(1, "YCbCr_Cr")
    expect_equal(ds$getRasterColorInterp(1), "YCbCr_Cr")
    ds$setRasterColorInterp(1, "Gray")
    expect_equal(ds$getRasterColorInterp(1), "Gray")
    files <- ds$getFileList()
    on.exit(unlink(files))
    ds$close()
})

test_that("Int64 data type is detected", {
    skip_if(as.integer(gdal_version()[2]) < 3050000)

    f <- system.file("extdata/int64.tif", package="gdalraster")
    expect_warning(ds <- new(GDALRaster, f, TRUE))
    expect_equal(ds$getDataTypeName(1), "Int64")
    ds$close()
})

test_that("get/set default RAT works", {
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    f <- tempfile(fileext=".tif")
    file.copy(evt_file,  f)
    ds <- new(GDALRaster, f, read_only=FALSE)
    expect_true(is.null(ds$getDefaultRAT(band=1)))
    evt_csv <- system.file("extdata/LF20_EVT_220.csv", package="gdalraster")
    evt_tbl <- read.csv(evt_csv)
    evt_tbl <- evt_tbl[,1:7]
    rat <- buildRAT(ds, table_type="thematic", na_value=-9999, join_df=evt_tbl)
    ds$setDefaultRAT(band=1, rat)
    ds$flushCache()
    rat2 <- ds$getDefaultRAT(band=1)
    expect_equal(nrow(rat2), 24)
    expect_equal(ncol(rat2), 8)
    expect_equal(attr(rat2, "GDALRATTableType"), "thematic")
    expect_equal(attr(rat2$VALUE, "GFU"), "MinMax")
    expect_equal(attr(rat2$COUNT, "GFU"), "PixelCount")
    expect_equal(attr(rat2$EVT_NAME, "GFU"), "Name")
    expect_equal(attr(rat2$EVT_LF, "GFU"), "Generic")
    expect_equal(attr(rat2$B, "GFU"), "Blue")

    # quiet
    ds$close()
    ds$open(TRUE)
    ds$quiet <- TRUE
    expect_silent(rat3 <- ds$getDefaultRAT(band=1))
    expect_equal(nrow(rat2), 24)

    ds$close()
    deleteDataset(f)
})

