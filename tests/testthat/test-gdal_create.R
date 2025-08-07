# test the R interface to GDALCreate() and GDALCreateCopy() via the wrapper
# functions in src/gdal_exp.cpp, with tests also in test-gdal_exp.R

test_that("create and createCopy R public interface works", {

    ## createCopy
    f <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    f2 <- paste0(tempdir(), "/", "storml_elev2.tif")

    ds <- new(GDALRaster, f)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])

    expect_no_error(ds2 <- createCopy(format = "GTiff",
                                      dst_filename = f2,
                                      src_filename = ds,
                                      return_obj = TRUE))

    expect_true(is(ds2, "Rcpp_GDALRaster"))
    dm2 <- ds2$dim()
    chk2 <- ds2$getChecksum(1, 0, 0, dm2[1], dm2[2])

    expect_equal(dm, dm2)

    ds$close()
    ds2$close()
    expect_true(deleteDataset(f2))

    # errors
    expect_error(createCopy(format = NULL,
                            dst_filename = f2,
                            src_filename = f))

    expect_error(createCopy(format = "GTiff",
                            dst_filename = NULL,
                            src_filename = f))

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = NULL))

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = f,
                            strict = NA))

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = f,
                            options = NA))

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = f,
                            quiet = NULL))

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = ds),
                 regexp = "source dataset is not open",
                 fixed = TRUE)

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = NULL),
                 regexp = "'src_filename' is required",
                 fixed = TRUE)

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = f,
                            return_obj = NULL),
                 regexp = "'return_obj' must be TRUE or FALSE",
                 fixed = TRUE)

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = f,
                            return_obj = NA),
                 regexp = "'return_obj' must be TRUE or FALSE",
                 fixed = TRUE)

    expect_error(createCopy(format = "GTiff",
                            dst_filename = f2,
                            src_filename = f,
                            return_obj = "TRUE"),
                 regexp = "'return_obj' must be a logical value",
                 fixed = TRUE)


    ## create
    f3 <- tempfile(fileext = ".tif")
    expect_no_error(ds3 <- create(format = "GTiff", dst_filename = f3,
                                  xsize = 10, ysize = 10, nbands = 1,
                                  dataType = "Int32", return_obj = TRUE))

    ds3$close()
    deleteDataset(f3)

    # errors
    expect_error(create(format = NULL, dst_filename = f3,
                        xsize = 10, ysize = 10, nbands = 1,
                        dataType = "Int32"))

    expect_error(create(format = "GTiff", dst_filename = NULL,
                        xsize = 10, ysize = 10, nbands = 1,
                        dataType = "Int32"))

    expect_error(create(format = "GTiff", dst_filename = f3,
                        xsize = NULL, ysize = 10, nbands = 1,
                        dataType = "Int32"))

    expect_error(create(format = "GTiff", dst_filename = f3,
                        xsize = 10, ysize = NULL, nbands = 1,
                        dataType = "Int32"))

    expect_error(create(format = "GTiff", dst_filename = f3,
                        xsize = 10, ysize = 10, nbands = NULL,
                        dataType = "Int32"))

    expect_error(create(format = "GTiff", dst_filename = f3,
                        xsize = 10, ysize = 10, nbands = 1,
                        dataType = NULL))

    expect_error(create(format = "GTiff", dst_filename = f3,
                        xsize = 10, ysize = 10, nbands = 1,
                        dataType = "Int32", options = NA))

    expect_error(create(format = "GTiff", dst_filename = f3,
                        xsize = 10, ysize = 10, nbands = 1,
                        dataType = "Int32", return_obj = NULL),
                 regexp = "'return_obj' must be TRUE or FALSE",
                 fixed = TRUE)

    expect_error(create(format = "GTiff", dst_filename = f3,
                        xsize = 10, ysize = 10, nbands = 1,
                        dataType = "Int32", return_obj = NA),
                 regexp = "'return_obj' must be TRUE or FALSE",
                 fixed = TRUE)

    expect_error(create(format = "GTiff", dst_filename = f3,
                        xsize = 10, ysize = 10, nbands = 1,
                        dataType = "Int32", return_obj = "TRUE"),
                 regexp = "'return_obj' must be a logical value",
                 fixed = TRUE)
})
