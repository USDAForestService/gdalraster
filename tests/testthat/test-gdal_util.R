test_that("translate runs without error", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    args <- c("-tr", "90", "90", "-r", "average")
    args <- c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
    img_file <- paste0(tempdir(), "/", "storml_elev_90m.img")
    expect_true(translate(elev_file, img_file, args))
    deleteDataset(img_file)
})

test_that("warp runs without error", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    args <- c("-tr", "90", "90", "-r", "cubic", "-tap")
    args <- c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
    alb83_file <- file.path(tempdir(), "storml_elev_alb83.img")
    expect_true(warp(elev_file, alb83_file, t_srs="EPSG:5070", cl_arg = args))
    ds <- new(GDALRaster, alb83_file)
    expect_true(srs_is_same(ds$getProjection(), srs_to_wkt("EPSG:5070")))
    expect_false(any(is.na(ds$getGeoTransform())))
    ds$close()
    unlink(alb83_file)

    # source as GDALRaster object
    src_ds <- new(GDALRaster, elev_file)
    expect_true(warp(src_ds, alb83_file, t_srs="EPSG:5070", cl_arg = args))
    ds <- new(GDALRaster, alb83_file)
    expect_true(srs_is_same(ds$getProjection(), srs_to_wkt("EPSG:5070")))
    expect_false(any(is.na(ds$getGeoTransform())))
    src_ds$close()
    ds$close()
    unlink(alb83_file)

    # invalid source files
    expect_error(warp(c(elev_file, "_err_"), alb83_file, t_srs="EPSG:5070",
                      cl_arg = args))

    # src_ds not open
    expect_error(warp(src_ds, alb83_file, t_srs="EPSG:5070", cl_arg = args))

    # invalid source list
    src_ds$open(read_only = TRUE)
    src_list <- list(src_ds, "_err_")
    expect_error(warp(src_list, alb83_file, t_srs="EPSG:5070", cl_arg = args))
    src_ds$close()

    # source andd destination as GDALRaster objects
    src_ds <- create("MEM", "", 20, 10, 1, "Byte", return_obj = TRUE)
    src_ds$setProjection(epsg_to_wkt(4326))
    src_ds$setGeoTransform(c(-180, 18, 0, 90, 0, -18))
    src_ds$fillRaster(1, 255, 0)

    dst_ds <- create("MEM", "", 20, 20, 1, "Byte", return_obj = TRUE)
    dst_ds$setProjection(epsg_to_wkt(3857))
    dst_ds$setGeoTransform(c(-2e6, 200000, 0, 2e6, 0, -200000))

    expect_true(warp(src_ds, dst_ds, t_srs = ""))
    expect_true(srs_is_same(dst_ds$getProjection(), srs_to_wkt("EPSG:3857")))
    r <- read_ds(dst_ds)
    attributes(r) <- NULL
    expect_equal(r, rep(255, 20 * 20))
    dst_ds$close()
    src_ds$close()

    # process without reproject
    resample_file <- file.path(tempdir(), "storml_elev_90m.img")
    expect_true(warp(elev_file, resample_file, t_srs="", cl_arg = args))
    ds1 <- new(GDALRaster, elev_file, read_only=TRUE)
    srs1 <- ds1$getProjectionRef()
    ds1$close()
    ds2 <- new(GDALRaster, resample_file, read_only=TRUE)
    srs2 <- ds2$getProjectionRef()
    ds2$close()
    expect_true(srs_is_same(srs1, srs2))

    # clean up
    deleteDataset(alb83_file)
    deleteDataset(resample_file)
})
