# Tests for src/gdalalg.cpp
# skip on CRAN while dev status of CLI bindings is "experimental"
skip_on_cran()
skip_if(gdal_version_num() < gdal_compute_version(3, 11, 3))

test_that("class constructors work", {
    # default constructor
    alg <- GDALAlg$new()
    expect_equal(alg$info()$name, "gdal")
    expect_output(print(alg), "gdal entry point")
    alg$release()

    # no args
    alg <- new(GDALAlg, "vector")
    alginfo <- alg$info()
    expect_true(alginfo$has_subalgorithms)
    expect_true(length(alginfo$subalgorithm_names) > 5)
    expect_output(print(alg), "vector")
    alg$release()

    # full path in a character string
    alg <- new(GDALAlg, "vector convert")
    alginfo <- alg$info()
    expect_false(alginfo$has_subalgorithms)
    expect_output(print(alg), "convert")
    alg$release()

    # cmd path as a character vector
    alg <- new(GDALAlg, c("vector", "convert"))
    alginfo <- alg$info()
    expect_false(alginfo$has_subalgorithms)
    expect_output(print(alg), "convert")
    alg$release()

    # args in a character vector
    f <- system.file("extdata/storml_elev.tif", package="gdalraster")
    args <- c("--format=text", "--no-md", f)
    expect_no_error(alg <- new(GDALAlg, "raster info", args))
    expect_true(alg$run())
    expect_true(length(nchar(alg$output())) == 1 && nchar(alg$output()) > 100)
    alg$release()

    # args in a list
    f <- system.file("extdata/ynp_fires_1984_2022.gpkg", package = "gdalraster")
    f_out = file.path(tempdir(), "ynp_fire_year.tif")

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

    expect_no_error(alg <- new(GDALAlg, "vector rasterize", args))
    expect_output(print(alg), "rasterize")
    expect_true(alg$parseCommandLineArgs())
    expect_true(alg$run())
    ds <- alg$output()
    expect_true(is(ds, "Rcpp_GDALRaster"))
    expect_true(alg$close())
    alg$release()

    expect_true(ds$isOpen())
    expect_true(length(ds$bbox()) == 4)
    expect_true(!any(is.na(ds$bbox())))
    ds$close()
    unlink(f_out)

    # errors
    expect_error(alg <- new(GDALAlg, ""), "empty")
    expect_error(alg <- new(GDALAlg, NA), "empty")
    expect_error(alg <- new(GDALAlg, c("raster", NA_character_)), "NA values")
    expect_error(alg <- new(GDALAlg, rep("gdal", 10)), "number of elements")
    expect_error(alg <- new(GDALAlg, "invalid"), "failed")
    expect_error(alg <- new(GDALAlg, "invalid info"), "failed")
    expect_error(alg <- new(GDALAlg, "raster invalid"), "failed")
})

test_that("algorithm and argument properties are returned correctly", {
    f <- system.file("extdata/storml_elev.tif", package="gdalraster")

    args <- list()
    args$input <- f
    f_clip <- file.path(tempdir(), "elev_clip.tif")
    on.exit(deleteDataset(f_clip))
    args$output <- f_clip
    args$overwrite <- TRUE
    args$bbox <- c(323776.1, 5102172.0,  327466.1, 5104782.0)

    expect_no_error(alg <- new(GDALAlg, "raster clip", args))

    # before arguments parsed or algorithm run
    alginfo <- alg$info()
    expect_true(is.list(alginfo))
    expect_equal(alginfo$name, "clip")
    expect_true(nchar(alginfo$description) > 10)
    expect_vector(alginfo$subalgorithm_names, character(), 0)
    expect_vector(alginfo$arg_names, character())

    arginfo <- alg$argInfo("bbox")
    expect_true(is.list(arginfo))
    expect_equal(arginfo$type, "REAL_LIST")
    expect_false(arginfo$is_explicitly_set)
    rm(arginfo)

    arginfo <- alg$argInfo("output")
    expect_true(is.list(arginfo))
    expect_equal(arginfo$type, "DATASET")
    expect_true(arginfo$is_required)
    expect_false(arginfo$is_explicitly_set)
    expect_equal(arginfo$dataset_type_flags, "RASTER")
    expect_equal(arginfo$dataset_output_flags, "OBJECT")

    expect_true(is.list(alg$getExplicitlySetArgs()))
    expect_equal(length(alg$getExplicitlySetArgs()), 0)

    rm(alginfo)
    rm(arginfo)

    # after args parsed (actual algorithm instantiated)
    expect_true(alg$parseCommandLineArgs())
    alginfo <- alg$info()
    expect_true(is.list(alginfo))
    expect_equal(alginfo$name, "clip")

    arginfo <- alg$argInfo("bbox")
    expect_true(is.list(arginfo))
    expect_equal(arginfo$type, "REAL_LIST")
    expect_true(arginfo$is_explicitly_set)
    rm(arginfo)

    arginfo <- alg$argInfo("output")
    expect_true(is.list(arginfo))
    expect_equal(arginfo$type, "DATASET")
    expect_true(arginfo$is_required)
    expect_true(arginfo$is_explicitly_set)
    expect_equal(arginfo$dataset_type_flags, "RASTER")
    expect_equal(arginfo$dataset_output_flags, "OBJECT")

    set_args <- alg$getExplicitlySetArgs()
    expect_true(is.list(set_args))
    expect_equal(length(set_args), 4)
    expect_true(startsWith(set_args$input, "<raster dataset object:") ||
                startsWith(set_args$input, "<list of dataset objects"))
    expect_true(startsWith(set_args$output, "<raster dataset object:"))

    rm(alginfo)
    rm(arginfo)

    # after actual algorithm has run
    expect_true(alg$run())
    alginfo <- alg$info()
    expect_true(is.list(alginfo))
    expect_equal(alginfo$name, "clip")

    arginfo <- alg$argInfo("bbox")
    expect_true(is.list(arginfo))
    expect_equal(arginfo$type, "REAL_LIST")
    expect_true(arginfo$is_explicitly_set)
    rm(arginfo)

    arginfo <- alg$argInfo("output")
    expect_true(is.list(arginfo))
    expect_equal(arginfo$type, "DATASET")
    expect_true(arginfo$is_required)
    expect_true(arginfo$is_explicitly_set)
    expect_equal(arginfo$dataset_type_flags, "RASTER")
    expect_equal(arginfo$dataset_output_flags, "OBJECT")

    rm(alginfo)
    rm(arginfo)

    expect_error(alg$argInfo(""), "required")
    expect_error(alg$argInfo("invalid"), "failed")

    expect_true(alg$close())
    alg$release()

    expect_error(alg$info(), "algorithm not instantiated")
    expect_error(alg$argInfo("output"), "algorithm not instantiated")
})

test_that("algorithm usage is returned", {
    expect_no_error(alg <- new(GDALAlg, "raster info"))

    expect_output(alg$usage(), "Usage:")

    expect_no_error(json <- alg$usageAsJSON())
    expect_true(is.character(json) && length(json) == 1)
    expect_true(nchar(json) > 50)

    alg$release()

    expect_error(alg$usageAsJSON(), "not instantiated")
})

test_that("GDALAlg S4 show() works", {
    expect_no_error(alg <- new(GDALAlg, "raster info"))

    expect_output(print(alg), "Description")

    alg$release()
})

test_that("algorithm run/output/outputs work", {
    f <- system.file("extdata/storml_elev.tif", package="gdalraster")

    ## basic test with a few different arg types
    f_clip <- file.path(tempdir(), "elev_clip.tif")

    args <- list()
    args$input <- f
    on.exit(deleteDataset(f_clip), add = TRUE)
    args$output <- f_clip
    args$overwrite <- TRUE
    args$bbox <- c(323776.1, 5102172.0,  327466.1, 5104782.0)

    expect_no_error(alg <- new(GDALAlg, "raster clip", args))
    expect_true(alg$run())

    # get the single output
    expect_no_error(ds <- alg$output())
    expect_true(is(ds, "Rcpp_GDALRaster"))
    ds$close()

    # get the output list (containing a single element in this case)
    expect_no_error(list_out <- alg$outputs())
    expect_true(is.list(list_out))
    expect_equal(length(list_out), 1)
    ds <- list_out$output
    expect_true(is(ds, "Rcpp_GDALRaster"))
    ds$close()

    expect_true(alg$close())
    alg$release()

    ## DATASET_LIST arg as vector of filenames and REAL_LIST arg
    b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    b6_file <- system.file("extdata/sr_b6_20200829.tif", package="gdalraster")
    band_files <- c(b6_file, b5_file, b4_file)

    f_vrt <- file.path(tempdir(), "stack_test.vrt")
    on.exit(deleteDataset(f_vrt), add = TRUE)

    args <- c("--dst-nodata=1,2,3", "--overwrite", band_files, f_vrt)
    alg <- new(GDALAlg, "raster stack", args)
    expect_true(alg$run())

    ds_stack <- alg$output()
    expect_true(is(ds_stack, "Rcpp_GDALRaster"))
    expect_equal(ds_stack$dim(),  c(149, 112, 3))
    expect_equal(ds_stack$getNoDataValue(band = 1), 1)
    expect_equal(ds_stack$getNoDataValue(band = 2), 2)
    expect_equal(ds_stack$getNoDataValue(band = 3), 3)

    ds_stack$close()
    expect_true(alg$close())
    alg$release()

    ## DATASET_LIST arg as objects and REAL_LIST as numeric vector
    ds_list <- list(new(GDALRaster, b6_file),
                    new(GDALRaster, b5_file),
                    new(GDALRaster, b4_file))

    f_vrt2 <- file.path(tempdir(), "stack_test2.vrt")
    on.exit(deleteDataset(f_vrt2), add = TRUE)

    args <- list("input" = ds_list,
                 "output" = f_vrt2,
                 "overwrite" = TRUE,
                 "dst_nodata" = c(1, 2, 3))

    alg <- new(GDALAlg, "raster stack", args)
    expect_true(alg$run())

    ds_stack <- alg$output()
    expect_true(is(ds_stack, "Rcpp_GDALRaster"))
    expect_equal(ds_stack$dim(),  c(149, 112, 3))
    expect_equal(ds_stack$getNoDataValue(band = 1), 1)
    expect_equal(ds_stack$getNoDataValue(band = 2), 2)
    expect_equal(ds_stack$getNoDataValue(band = 3), 3)

    ds_stack$close()
    for (i in seq_along(ds_list))
        ds_list[[i]]$close()
    alg$release()

    ## STRING_LIST arg (--src-nodata and --dst-nodata are STRING_LIST in
    ## "raster reproject")
    args <- c("--output", "", "--format", "MEM", "--size=2,1", "--band-count=2",
              "--datatype=Float32")
    alg <- gdal_run("raster create", args)
    ds <- alg$output()
    alg$release()
    expect_true(ds$setGeoTransform(c(0, 1, 0, 0, 0, -1)))
    ds$write(1, 0, 0, 2, 1, c(-999, 1))
    ds$write(2, 0, 0, 2, 1, c(NaN, 2))

    args <- list("input" = ds,
                 "output" = "",
                 "format" = "MEM",
                 "src-nodata" = c("-999", "nan"),
                 "dst-nodata" = c("0", "0"))
    alg <- gdal_run("raster reproject", args)
    out_ds <- alg$output()
    alg$release()
    expect_equal(out_ds$read(1, 0, 0, 2, 1, 2, 1), c(NA_real_, 1))
    expect_equal(out_ds$read(2, 0, 0, 2, 1, 2, 1), c(NA_real_, 2))
    out_ds$deleteNoDataValue(band = 1)
    out_ds$deleteNoDataValue(band = 2)
    expect_equal(out_ds$read(1, 0, 0, 2, 1, 2, 1), c(0, 1))
    expect_equal(out_ds$read(2, 0, 0, 2, 1, 2, 1), c(0, 2))

    out_ds$close()
    ds$close()
})

test_that("`setVectorArgsFromObject` and `outputLayerNameForOpen` work", {
    f <- system.file("extdata/ynp_features.zip", package = "gdalraster")
    ynp_dsn <- file.path("/vsizip", f, "ynp_features.gpkg")

    ## auto set "input-format", "input-layer"
    poi <- new(GDALVector, ynp_dsn, "points_of_interest")
    poi$setAttributeFilter("poiname = 'Abyss Pool'")

    f_out <- file.path(tempdir(), "filter_test.gpkg")
    on.exit(unlink(f_out), add = TRUE)

    args <- list()
    args$input <- poi
    args$output <- f_out
    args$overwrite <- TRUE

    expect_no_error(alg <- new(GDALAlg, "vector filter", args))
    expect_true(alg$setVectorArgsFromObject)  # the default
    expect_false(alg$argInfo("input-format")$is_explicitly_set)
    expect_false(alg$argInfo("input-layer")$is_explicitly_set)
    expect_true(alg$parseCommandLineArgs())
    expect_true(alg$argInfo("input-format")$is_explicitly_set)
    expect_true(alg$argInfo("input-layer")$is_explicitly_set)
    expect_true(alg$run())

    expect_true(ogr_ds_layer_count(f_out) == 1)
    lyr <- alg$output()
    expect_true(is(lyr, "Rcpp_GDALVector"))
    expect_equal(lyr$getName(), "points_of_interest")
    expect_equal(lyr$getFeatureCount(), 1)
    feat <- lyr$getNextFeature()
    expect_equal(feat$poiname, "Abyss Pool")

    alg$release()
    lyr$close()

    # with `setVectorArgsFromObject` disabled
    f2_out <- file.path(tempdir(), "filter_test_2.gpkg")
    on.exit(unlink(f2_out), add = TRUE)

    args <- list()
    args$input <- poi
    args$output <- f2_out
    args$overwrite <- TRUE

    alg <- new(GDALAlg, "vector filter", args)
    alg$setVectorArgsFromObject <- FALSE
    expect_false(alg$setVectorArgsFromObject)
    expect_false(alg$argInfo("input-format")$is_explicitly_set)
    expect_false(alg$argInfo("input-layer")$is_explicitly_set)
    expect_true(alg$parseCommandLineArgs())
    expect_false(alg$argInfo("input-format")$is_explicitly_set)
    expect_false(alg$argInfo("input-layer")$is_explicitly_set)
    expect_true(alg$run())

    # all layers in the dataset are processed and present in the output dataset
    expect_true(ogr_ds_layer_count(f2_out) > 1)
    # set the output layer name to open
    alg$outputLayerNameForOpen <- "points_of_interest"
    lyr <- alg$output()
    expect_true(is(lyr, "Rcpp_GDALVector"))
    expect_equal(lyr$getName(), "points_of_interest")

    # This gives feature count of 1, i.e., the attribute filter defined on
    # the layer via the GDALVector object is still honored when the layer is
    # referenced via the dataset ref acquired by the algorithm object.
    # "where" argument was not given explicitly:
    expect_false(alg$argInfo("where")$is_explicitly_set)
    expect_equal(lyr$getFeatureCount(), 1)
    feat <- lyr$getNextFeature()
    expect_equal(feat$poiname, "Abyss Pool")

    alg$release()
    lyr$close()
    poi$close()

    ## auto set "sql"
    sql <- "SELECT * FROM points_of_interest WHERE poiname = 'Abyss Pool'"
    poi_abyss <- new(GDALVector, ynp_dsn, sql)

    alg <- new(GDALAlg, "vector info", list("input" = poi_abyss))
    expect_false(alg$argInfo("sql")$is_explicitly_set)
    expect_true(alg$parseCommandLineArgs())
    expect_true(alg$argInfo("sql")$is_explicitly_set)
    # "vector info" should use "input-layer" instead
    # cf. https://github.com/OSGeo/gdal/issues/12903
    expect_false(alg$argInfo("layer")$is_explicitly_set)
    expect_true(alg$run())

    json <- alg$output()
    expect_true(startsWith(json, "{"))
    expect_true(nchar(json) > 2000)
    expect_true(grepl("name\":\"SELECT\"", json, ignore.case = TRUE))
    expect_true(grepl("featureCount\":1", json, ignore.case = TRUE))

    alg$release()
    poi_abyss$close()

    ## auto set "sql" with "dialect"
    shp_dsn <- system.file("extdata/poly_multipoly.shp", package = "gdalraster")
    sql <- "SELECT rowid AS fid, ST_Centroid(geometry) As geom
            FROM poly_multipoly"
    if (has_spatialite()) {
        lyr_in <- new(GDALVector, shp_dsn, sql, TRUE, NULL, "", "SQLite")

        f_sql_out <- file.path(tempdir(), "spatialite_test.gpkg")
        on.exit(unlink(f_sql_out), add = TRUE)

        args <- list()
        args$input <- lyr_in
        args$output <- f_sql_out
        args$overwrite <- TRUE

        expect_no_error(alg <- new(GDALAlg, "vector sql", args))
        expect_true(alg$setVectorArgsFromObject)  # the default
        expect_false(alg$argInfo("input-format")$is_explicitly_set)
        expect_false(alg$argInfo("sql")$is_explicitly_set)
        expect_false(alg$argInfo("dialect")$is_explicitly_set)
        expect_true(alg$parseCommandLineArgs())
        expect_true(alg$argInfo("input-format")$is_explicitly_set)
        expect_true(alg$argInfo("sql")$is_explicitly_set)
        expect_true(alg$argInfo("dialect")$is_explicitly_set)
        expect_true(alg$run())

        expect_true(ogr_ds_layer_count(f_sql_out) == 1)
        lyr_out <- alg$output()
        expect_true(is(lyr_out, "Rcpp_GDALVector"))
        expect_equal(toupper(lyr_out$getName()), "SELECT")
        expect_equal(lyr_out$getFeatureCount(), 4)
        feat <- lyr_out$getNextFeature()
        expect_equal(toupper(g_name(feat$geom)), "POINT")

        alg$release()
        lyr_out$close()
        lyr_in$close()
    }

    ## manually set SQL with input of GDALVector open on a regular layer
    lyr_in <- new(GDALVector, shp_dsn)

    f2_sql_out <- file.path(tempdir(), "sql_test.shp")
    on.exit(deleteDataset(f2_sql_out), add = TRUE)

    args <- list()
    args$input <- lyr_in
    args$sql <- "SELECT FID, '_ogr_geometry_' FROM poly_multipoly LIMIT 1"
    args$output <- f2_sql_out
    args$overwrite <- TRUE

    expect_no_error(alg <- new(GDALAlg, "vector sql", args))
    expect_true(alg$setVectorArgsFromObject)  # the default
    expect_false(alg$argInfo("sql")$is_explicitly_set)
    expect_false(alg$argInfo("dialect")$is_explicitly_set)
    expect_true(alg$parseCommandLineArgs())
    expect_true(alg$argInfo("sql")$is_explicitly_set)
    expect_false(alg$argInfo("dialect")$is_explicitly_set)
    expect_true(alg$run())

    lyr_out <- alg$output()
    expect_true(is(lyr_out, "Rcpp_GDALVector"))
    expect_equal(toupper(lyr_out$getName()), "SQL_TEST")
    expect_equal(lyr_out$getFeatureCount(), 1)
    feat <- lyr_out$getNextFeature()
    expect_equal(toupper(g_name(feat$geom)), "POLYGON")

    alg$release()
    lyr_out$close()
    lyr_in$close()

    # the following tests require fixes added in GDAL 3.12
    # e.g., gdal raster clip: fix bbox check
    # https://github.com/OSGeo/gdal/pull/12814
    skip_if(gdal_version_num() < gdal_compute_version(3, 12, 0))

    ## auto set "like-layer
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file)

    bnd_1 <- "POLYGON ((324467.3 5104814.2,
                    323909.4 5104365.4,
                    323794.2 5103455.8,
                    324970.7 5102885.8,
                    326420.0 5103595.3,
                    326389.6 5104747.5,
                    325298.1 5104929.4,
                    325298.1 5104929.4,
                    324467.3 5104814.2))"

    f_temp_poly <- file.path(tempdir(), "storml_test_poly.gpkg")
    on.exit(unlink(f_temp_poly), add = TRUE)
    lyr <- ogr_ds_create("GPKG", f_temp_poly, layer = "test_poly",
                        geom_type = "POLYGON", srs = ds$getProjection(),
                        fld_name = "poly_name", fld_type = "OFTString",
                        overwrite = TRUE, return_obj = TRUE)

    lyr$createFeature(list(poly_name = "bnd_1", geom = bnd_1))
    lyr$createFeature(list(poly_name = "bnd_2", geom = g_buffer(bnd_1, 90)))
    lyr$syncToDisk()

    f_clip_bnd_1_2 <- file.path(tempdir(), "elev_clip_bnd_1_2.tif")
    on.exit(deleteDataset(f_clip_bnd_1_2), add = TRUE)

    args <- list()
    args$input <- ds
    args$output <- f_clip_bnd_1_2
    args$overwrite <- TRUE
    args$like <- lyr

    expect_no_error(alg <- new(GDALAlg, "raster clip", args))
    expect_false(alg$argInfo("like-layer")$is_explicitly_set)
    expect_true(alg$parseCommandLineArgs())
    expect_true(alg$argInfo("like-layer")$is_explicitly_set)
    expect_true(alg$run())

    ds_out <- alg$output()
    expect_true(is(ds_out, "Rcpp_GDALRaster"))
    expect_equal(ds_out$bbox(), c(323686.1, 5102772.0, 326536.1, 5105022.0),
                 tolerance = 0.1)

    alg$release()
    ds_out$close()
    lyr$close()

    ## auto set "like-sql"
    sql <- "SELECT * FROM test_poly WHERE poly_name = 'bnd_1'"
    lyr <- new(GDALVector, f_temp_poly, sql)

    f_clip_bnd_1 <- file.path(tempdir(), "elev_clip_bnd_1.tif")
    on.exit(deleteDataset(f_clip_bnd_1), add = TRUE)

    args <- list()
    args$input <- ds
    args$output <- f_clip_bnd_1
    args$overwrite <- TRUE
    args$like <- lyr

    expect_no_error(alg <- new(GDALAlg, "raster clip", args))
    expect_false(alg$argInfo("like-layer")$is_explicitly_set)
    expect_false(alg$argInfo("like-sql")$is_explicitly_set)
    expect_true(alg$parseCommandLineArgs())
    expect_false(alg$argInfo("like-layer")$is_explicitly_set)
    expect_true(alg$argInfo("like-sql")$is_explicitly_set)
    expect_true(alg$run())

    ds_out <- alg$output()
    expect_true(is(ds_out, "Rcpp_GDALRaster"))
    expect_equal(ds_out$bbox(), c(323776.1, 5102862.0, 326446.1, 5104932.0),
                 tolerance = 0.1)

    alg$release()
    ds_out$close()
    lyr$close()
})

test_that("setArg works", {
    ## set args individually by API
    f_dem <- system.file("extdata/storml_elev.tif", package="gdalraster")
    ds <- new(GDALRaster, f_dem)

    f_aspect <- file.path(tempdir(), "aspect_test.tif")
    on.exit(deleteDataset(f_aspect), add = TRUE)

    alg <- new(GDALAlg, "raster aspect")
    expect_true(alg$setArg("input", ds))
    expect_true(alg$setArg("output", f_aspect))
    expect_true(alg$setArg("creation-option", "COMPRESS=DEFLATE"))
    expect_true(alg$setArg("overwrite", TRUE))
    expect_true(alg$setArg("zero-for-flat", TRUE))
    expect_true(alg$run())

    ds_aspect <- alg$output()
    expect_true(is(ds_aspect, "Rcpp_GDALRaster"))
    minmax <- ds_aspect$getMinMax(band = 1, approx_ok = FALSE)
    expect_equal(minmax[1], 0)
    expect_true(minmax[2] >= 359 && minmax[2] <= 360)
    compression <- ds_aspect$getMetadataItem(band = 0, "COMPRESSION",
                                             "IMAGE_STRUCTURE")
    expect_equal(compression, "DEFLATE")
    ds_aspect$close()
    expect_true(alg$close())
    alg$release()

    ## instantiate with args, then set additional args individually
    f_slpp <- file.path(tempdir(), "slpp_test.tif")
    on.exit(deleteDataset(f_slpp), add = TRUE)

    args <- list()
    args$input <- ds
    args$output <- f_slpp
    args$creation_option <- "COMPRESS=DEFLATE"
    args$overwrite <- TRUE
    alg <- new(GDALAlg, "raster slope", args)
    expect_true(alg$setArg("unit", "percent"))
    expect_true(alg$run())

    ds_slpp <- alg$output()
    expect_true(is(ds_slpp, "Rcpp_GDALRaster"))
    minmax <- ds_slpp$getMinMax(band = 1, approx_ok = FALSE)
    expect_equal(minmax[1], 0)
    expect_true(minmax[2] > 100 && minmax[2] < 200)
    compression <- ds_slpp$getMetadataItem(band = 0, "COMPRESSION",
                                           "IMAGE_STRUCTURE")
    expect_equal(compression, "DEFLATE")
    ds_slpp$close()
    expect_true(alg$close())
    alg$release()

    ## instantiate with args, then set additional args individually after
    ## parsing command-line arguments (which also instantiates the actual
    ## algorithm internally)
    f_clip <- file.path(tempdir(), "clip_test.tif")
    on.exit(deleteDataset(f_clip), add = TRUE)

    args <- list()
    args$input <- ds
    args$output <- f_clip
    args$overwrite <- TRUE
    alg <- new(GDALAlg, "raster clip", args)
    expect_true(alg$parseCommandLineArgs())
    expect_true(
        alg$setArg("bbox", c(323776.1, 5102172.0,  327466.1, 5104782.0)))
    expect_true(alg$setArg("creation-option", "COMPRESS=DEFLATE"))
    set_args <- alg$getExplicitlySetArgs()
    expect_equal(set_args$bbox, c(323776.1, 5102172.0,  327466.1, 5104782.0),
                 tolerance = 1e-5)
    expect_equal(set_args$creation_option, "COMPRESS=DEFLATE")
    expect_true(alg$run())

    ds_clip <- alg$output()
    expect_true(is(ds_clip, "Rcpp_GDALRaster"))
    expect_equal(ds_clip$dim(), c(123, 87, 1))
    compression <- ds_clip$getMetadataItem(band = 0, "COMPRESSION",
                                           "IMAGE_STRUCTURE")
    expect_equal(compression, "DEFLATE")
    ds_clip$close()
    expect_true(alg$close())
    alg$release()

    ds$close()

    ## DATASET_LIST arg as charcter vector and REAL_LIST as numeric vector
    b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    b6_file <- system.file("extdata/sr_b6_20200829.tif", package="gdalraster")
    band_files <- c(b6_file, b5_file, b4_file)

    f_vrt <- file.path(tempdir(), "stack_test.vrt")
    on.exit(deleteDataset(f_vrt), add = TRUE)

    alg <- new(GDALAlg, "raster stack")
    expect_true(alg$setArg("input", band_files))
    expect_true(alg$setArg("output", f_vrt))
    expect_true(alg$setArg("overwrite", TRUE))
    expect_true(alg$setArg("dst-nodata", c(1, 2, 3)))
    expect_true(alg$run())

    ds_stack <- alg$output()
    expect_true(is(ds_stack, "Rcpp_GDALRaster"))
    expect_equal(ds_stack$dim(),  c(149, 112, 3))
    expect_equal(ds_stack$getNoDataValue(band = 1), 1)
    expect_equal(ds_stack$getNoDataValue(band = 2), 2)
    expect_equal(ds_stack$getNoDataValue(band = 3), 3)

    ds_stack$close()
    alg$release()

    ## DATASET_LIST arg as list of objects
    ds_list <- list(new(GDALRaster, b6_file),
                    new(GDALRaster, b5_file),
                    new(GDALRaster, b4_file))

    f_vrt2 <- file.path(tempdir(), "stack_test2.vrt")
    on.exit(deleteDataset(f_vrt2), add = TRUE)

    alg <- new(GDALAlg, "raster stack")
    expect_true(alg$setArg("input", ds_list))
    expect_true(alg$setArg("output", f_vrt2))
    expect_true(alg$setArg("overwrite", TRUE))
    expect_true(alg$run())

    ds_stack <- alg$output()
    expect_true(is(ds_stack, "Rcpp_GDALRaster"))
    expect_equal(ds_stack$dim(),  c(149, 112, 3))
    expect_equal(ds_stack$getNoDataValue(band = 1), 0)
    expect_equal(ds_stack$getNoDataValue(band = 2), 0)
    expect_equal(ds_stack$getNoDataValue(band = 3), 0)

    ds_stack$close()
    for (i in seq_along(ds_list))
        ds_list[[i]]$close()
    alg$release()

    ## INTEGER_LIST (--size in "raster create")
    ## STRING_LIST (--src-nodata and --dst-nodata in "raster reproject")
    args <- c("--output", "", "--format", "MEM")
    alg <- gdal_alg("raster create", args)
    expect_true(alg$setArg("size", c(2, 1)))
    expect_true(alg$setArg("band-count", 2))
    expect_true(alg$setArg("datatype", "Float32"))
    expect_true(alg$run())

    ds <- alg$output()
    alg$release()
    expect_true(ds$setGeoTransform(c(0, 1, 0, 0, 0, -1)))
    ds$write(1, 0, 0, 2, 1, c(-999, 1))
    ds$write(2, 0, 0, 2, 1, c(NaN, 2))

    args <- list("input" = ds,
                 "output" = "",
                 "format" = "MEM")
    alg <- gdal_alg("raster reproject", args)
    expect_true(alg$setArg("src-nodata", c("-999", "nan")))
    expect_true(alg$setArg("dst-nodata", c("0", "0")))
    expect_true(alg$run())

    out_ds <- alg$output()
    alg$release()
    expect_equal(out_ds$read(1, 0, 0, 2, 1, 2, 1), c(NA_real_, 1))
    expect_equal(out_ds$read(2, 0, 0, 2, 1, 2, 1), c(NA_real_, 2))
    out_ds$deleteNoDataValue(band = 1)
    out_ds$deleteNoDataValue(band = 2)
    expect_equal(out_ds$read(1, 0, 0, 2, 1, 2, 1), c(0, 1))
    expect_equal(out_ds$read(2, 0, 0, 2, 1, 2, 1), c(0, 2))

    out_ds$close()
    ds$close()
})
