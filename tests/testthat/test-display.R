test_that("plot_raster works", {
    # grayscale
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file, read_only=TRUE)
    expect_silent(plot_raster(ds, legend=TRUE))
    ds$close()

    # rgb
    b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    b6_file <- system.file("extdata/sr_b6_20200829.tif", package="gdalraster")
    band_files <- c(b6_file, b5_file, b4_file)
    ds <- new(GDALRaster, b5_file, read_only=TRUE)
    dm <- ds$dim()
    ds$close()
    r <- vector("integer")
    for (f in band_files) {
        ds <- new(GDALRaster, f, read_only=TRUE)
        r <- c(r, read_ds(ds))
        ds$close()
    }
    expect_silent(plot_raster(r, xsize=dm[1], ysize=dm[2], nbands=3,
                              minmax_def=c(7551,7679,7585,14842,24997,12451)))
    expect_silent(plot_raster(r, xsize=dm[1], ysize=dm[2], nbands=3,
                              minmax_pct_cut=c(2,98)))

    # color table
    evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
    evc_vat <- system.file("extdata/LF20_EVC_220.csv", package="gdalraster")
    vat <- read.csv(evc_vat)
    vat <- vat[,c(1,6:8)]
    ds <- new(GDALRaster, evc_file, read_only=TRUE)
    # dm <- ds$dim()
    r <- read_ds(ds)
    ds$close()
    expect_silent(plot_raster(r, col_tbl=vat, interpolate=FALSE))

    # as_list
    ds$open(TRUE)
    r <- read_ds(ds, as_list=TRUE)
    ds$close()
    expect_silent(plot_raster(r, col_tbl=vat, interpolate=FALSE))

    # built-in color table
    tcc_file <- system.file("extdata/storml_tcc.tif", package="gdalraster")
    ds <- new(GDALRaster, tcc_file, read_only=TRUE)
    expect_silent(plot_raster(ds, legend=TRUE))
    ds$close()

    # complex data type
    f <- system.file("extdata/complex.tif", package="gdalraster")
    ds <- new(GDALRaster, f)
    expect_error(plot_raster(ds))
    # with pixel function
    expect_silent(plot_raster(ds, pixel_fn = Arg, interpolate = FALSE))
    ds$close()
})

test_that("plot_geom works", {
    set.seed(1)
    x <- sample.int(100, 10)
    y <- sample.int(100, 10)
    g <- g_create("MULTIPOINT", cbind(x, y))
    expect_identical(plot_geom(g), g)
})
