# skip on CRAN while dev status of CLI bindings is "experimental"
skip_on_cran()
skip_if(gdal_version_num() < gdal_compute_version(3, 11, 3))

test_that("gdal_commands works", {
    expect_output(cmds <- gdal_commands(), "info:")
    expect_true(is.data.frame(cmds))
    expect_true(nrow(cmds) > 10)
    expect_equal(colnames(cmds), c("command", "description", "URL"))

    expect_invisible(gdal_commands("raster info", cout = FALSE))
    expect_silent(cmds <- gdal_commands("raster info", cout = FALSE))
    expect_true(is.data.frame(cmds))
    expect_equal(nrow(cmds), 1)

    expect_no_error(cmds <- gdal_commands(recurse = FALSE, cout = FALSE))
    expect_equal(nrow(cmds[cmds$command_string == "raster info", ]), 0)

    expect_no_error(gdal_commands(NULL))
    expect_no_error(gdal_commands(c("raster", "info")))
    expect_error(gdal_commands(0))
    expect_error(gdal_commands(recurse = "invalid"))
    expect_error(gdal_commands(cout = "invalid"))
})

test_that("gdal_run works", {
    ## raster output
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    f_out = file.path(tempdir(), "ynp_fire_year.tif")
    on.exit(deleteDataset(f_out), add = TRUE)

    args <- list()
    args$input <- f
    args$sql <- "SELECT * FROM mtbs_perims ORDER BY mtbs_perims.ig_year"
    args$attribute_name <- "ig_year"
    args$output <- f_out
    args$overwrite <- TRUE
    args$creation_option <- c("TILED=YES", "COMPRESS=DEFLATE")
    args$resolution <- c(90, 90)
    args$output_data_type <- "Int16"
    args$init <- -32767
    args$nodata <- -32767

    expect_no_error(alg <- gdal_run("vector rasterize", args))

    ds <- alg$output()
    expect_true(is(ds, "Rcpp_GDALRaster"))
    expect_equal(ds$res(), c(90, 90))
    expect_equal(ds$getDataTypeName(band = 1), "Int16")
    expect_equal(ds$getNoDataValue(band = 1), -32767)

    ds$close()
    expect_true(alg$close())
    alg$release()

    ## vector output
    f_shp <- system.file("extdata/poly_multipoly.shp", package="gdalraster")
    f_gpkg <- file.path(tempdir(), "polygons_test.gpkg")
    on.exit(deleteDataset(f_gpkg), add = TRUE)

    args <- c("--input", f_shp, "--output", f_gpkg, "--overwrite")

    expect_no_error(alg <- gdal_run("vector convert", args))

    lyr <- alg$output()
    expect_true(is(lyr, "Rcpp_GDALVector"))
    expect_equal(lyr$getDriverShortName(), "GPKG")

    lyr$close()
    expect_true(alg$close())
    alg$release()

    # errors
    expect_error(gdal_run(NULL))
    expect_error(gdal_run(0))
    expect_error(gdal_run("raster info"))  # no args so alg$run() fails
    expect_error(gdal_run("raster info", 0))
    expect_error(gdal_run("raster info", "--invalid=0"))  # parse fails
    # invalid input for setVectorArgsFromObject:
    args <- c("--input", f_shp, "--output", f_gpkg, "--overwrite")
    expect_error(gdal_run("vector convert", args, "invalid"))

})

test_that("gdal_alg works", {
    expect_no_error(alg <- gdal_alg())
    expect_true(is(alg, "Rcpp_GDALAlg"))
    expect_equal(alg$info()$name, "gdal")
    alg$release()

    expect_no_error(alg <- gdal_alg("raster"))
    expect_true(is(alg, "Rcpp_GDALAlg"))
    alg$release()

    expect_no_error(alg <- gdal_alg("vector"))
    expect_true(is(alg, "Rcpp_GDALAlg"))
    alg$release()

    f <- system.file("extdata/storml_elev.tif", package="gdalraster")

    # character vector args
    args <- c("--format=text", f)
    expect_no_error(alg <- gdal_alg("raster info", args))
    expect_error(alg$output())
    expect_true(alg$run())
    expect_true(nchar(alg$output()) > 1000)
    alg$release()

    # parse = FALSE
    args <- c("--format=text", f)
    expect_no_error(alg <- gdal_alg("raster info", args, FALSE))
    expect_error(alg$output())
    expect_true(alg$parseCommandLineArgs())
    expect_true(alg$run())
    expect_true(nchar(alg$output()) > 1000)
    alg$release()

    # list args
    args <- list()
    args$input <- f
    args$output_format <- "text"
    expect_no_error(alg <- gdal_alg("raster info", args))
    expect_true(alg$run())
    expect_true(nchar(alg$output()) > 1000)
    alg$release()

    # input as object
    ds <- new(GDALRaster, f)
    args <- list()
    args$input <- ds
    args$output_format <- "text"
    expect_no_error(alg <- gdal_alg("raster info", args))
    expect_true(alg$run())
    expect_true(nchar(alg$output()) > 1000)
    expect_true(alg$close())
    alg$release()

    # errors
    expect_error(gdal_alg(0))
    expect_error(gdal_alg("raster info", FALSE))
    expect_error(gdal_alg("raster info", args, parse = "invalid"))
    args$invalid <- "invalid arg name"
    expect_error(gdal_alg("raster info", args))

    ds$close()
})

test_that("gdal_usage works", {
    cmd <- "raster reproject"
    expect_output(gdal_usage(cmd), "Usage:")
    expect_output(gdal_usage(cmd), "Positional arguments:")
    expect_output(gdal_usage(cmd), "Options:")
    expect_output(gdal_usage(cmd), "Advanced options:")
    expect_output(gdal_usage(cmd), "For more details:")
})

test_that("gdal_global_reg_names returns a character vector", {
    expect_vector(gdal_global_reg_names(), character())
})
