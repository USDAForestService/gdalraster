# Tests for src/gdal_exp.cpp - Exported stand-alone functions for gdalraster
# Some functions in gdal_exp.cpp are called by gdalraster_proc.R and
# tested there.
test_that("gdal_version returns vector", {
    expect_length(gdal_version(), 4)
})

test_that("gdal_formats returns a data frame", {
    x <- gdal_formats()
    expect_s3_class(x, "data.frame")
    expect_true(nrow(x) > 1)
})

test_that(".check_gdal_filename works", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    expect_error(.check_gdal_filename(c(elev_file, b5_file)))
    vsifn <- paste0("/vsi/", b5_file)
    expect_equal(vsifn, .check_gdal_filename(vsifn))
    fn <- "~/_r82jRwnT.test"
    fn_out <- .check_gdal_filename(fn)
    expect_equal(basename(fn_out), basename(fn))
})

test_that("get/set_config_option work", {
    co <- get_config_option("GDAL_CACHEMAX")
    set_config_option("GDAL_CACHEMAX", "64")
    expect_equal(get_config_option("GDAL_CACHEMAX"), "64")
    set_config_option("GDAL_CACHEMAX", co)
})

test_that("get_cache_used returns integer64", {
    expect_s3_class(get_cache_used(), "integer64")
    expect_s3_class(get_cache_used("GB"), "integer64")
    expect_s3_class(get_cache_used("KB"), "integer64")
    expect_s3_class(get_cache_used(""), "integer64")
    expect_s3_class(get_cache_used("bytes"), "integer64")
    expect_error(get_cache_used("MiB"))
})

test_that("get_cache_max returns integer64", {
    expect_s3_class(get_cache_max(), "integer64")
    expect_s3_class(get_cache_max("GB"), "integer64")
    expect_s3_class(get_cache_max("KB"), "integer64")
    expect_s3_class(get_cache_max(""), "integer64")
    expect_s3_class(get_cache_max("bytes"), "integer64")
    expect_error(get_cache_max("MiB"))
})

test_that("set_cache_max works", {
    cachemax <- get_cache_max("bytes")
    set_cache_max(1e8)
    expect_equal(get_cache_max("bytes"), as.integer64(1e8))
    # reset to original
    set_cache_max(cachemax)
    expect_equal(get_cache_max("bytes"), cachemax)
})

test_that("get_num_cpus returns integer", {
    expect_type(get_num_cpus(), "integer")
})

test_that("get_usable_physical_ram returns integer64", {
    expect_s3_class(get_usable_physical_ram(), "integer64")
})

test_that("has_spatialite returns logical", {
    expect_type(has_spatialite(), "logical")
})

test_that("http_enabled returns logical", {
    expect_type(http_enabled(), "logical")
})

test_that(".cpl_http_cleanup runs without error", {
    expect_no_error(.cpl_http_cleanup())
})

test_that("createCopy writes correct output", {
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
    options <- c("COMPRESS=LZW")
    createCopy(format="GTiff", dst_filename=tif_file, src_filename=lcp_file,
               options=options)
    ds <- new(GDALRaster, tif_file, read_only=FALSE)
    on.exit(deleteDataset(tif_file))
    md <- ds$getMetadata(band=0, domain="IMAGE_STRUCTURE")
    expect_equal(md, c("COMPRESSION=LZW", "INTERLEAVE=PIXEL"))
    for (band in 1:ds$getRasterCount())
         ds$setNoDataValue(band, -9999)
    stats <- ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
    expect_equal(round(stats), round(c(2438.0, 3046.0, 2675.9713, 133.0185)))
    dm <- ds$dim()
    chk <- ds$getChecksum(5, 0, 0, dm[1], dm[2])
    expect_equal(chk, 64107)
    ds$close()
})

test_that("internal apply_geotransform gives correct result", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file, read_only=TRUE)
    gt <- ds$getGeoTransform()
    ds$close()
    expect_equal(.apply_geotransform(gt, 1, 1), c(323506.1, 5105052.0))
})

test_that("apply_geotransform gives correct results", {
    raster_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    ds <- new(GDALRaster, raster_file)

    # compute some raster coordinates in column/row space
    set.seed(1)
    col_coords <- runif(10, min = 0, max = ds$getRasterXSize() - 0.00001)
    row_coords <- runif(10, min = 0, max = ds$getRasterYSize() - 0.00001)
    col_row <- cbind(col_coords, row_coords)
    dimnames(col_row) <- NULL

    x_expected <- c(324615.1, 325072.5, 325933.6, 327372.3, 324341.3, 327330.2,
                    327528.7, 326310.9, 326175.0, 323741.1)
    y_expected <- c(5104421, 5104515, 5102877, 5103849, 5102611, 5103484,
                    5102778, 5101898, 5103862, 5102586)
    xy_expected <- cbind(x_expected, y_expected)
    dimnames(xy_expected) <- NULL

    gt <- ds$getGeoTransform()
    expect_equal(apply_geotransform(col_row, gt), xy_expected, tolerance = 0.1)

    # or, using the class method
    expect_equal(ds$apply_geotransform(col_row), xy_expected, tolerance = 0.1)

    expect_equal(ds$apply_geotransform(col_row) |> ds$get_pixel_line(),
                 trunc(col_row))

    # one coordinate as vector input
    res <- ds$apply_geotransform(c(col_coords[1], row_coords[1]))
    dim(res) <- NULL
    expect_equal(res, xy_expected[1, ], tolerance = 0.1)

    # NA input
    res <- ds$apply_geotransform(c(NA, NA))
    expect_true(all(is.na(res)))

    res <- apply_geotransform(c(NA, NA), gt)
    expect_true(all(is.na(res)))

    ds$close()
})

test_that("get_pixel_line gives correct results", {
    pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
    pts <- read.csv(pt_file)
    pix_line <- c(39, 23, 1, 58, 74, 94, 68, 92, 141, 23, 57, 68, 58, 52, 90,
                  38, 31, 85, 20, 39)
    raster_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    ds <- new(GDALRaster, raster_file, read_only=TRUE)
    gt <- ds$getGeoTransform()
    res <- get_pixel_line(as.matrix(pts[, -1]), gt)
    expect_equal(as.vector(res), pix_line)

    # or, using the class method and data frame input
    res <- ds$get_pixel_line(pts[, -1])
    expect_equal(as.vector(res), pix_line)

    pts[11, ] <- c(11, 323318, 5105104)
    expect_warning(res2 <- ds$get_pixel_line(pts[, -1]))
    res <- rbind(res, c(NA, NA))
    expect_equal(res2, res)

    # one coordinate as vector input
    res <- get_pixel_line(c(pts[1, 2], pts[1, 3]), gt)  # col 2 x, col 3 y
    expect_equal(as.vector(res), c(pix_line[1], pix_line[11]))

    # NA input
    res <- get_pixel_line(c(NA, NA), gt)
    expect_true(all(is.na(res)))

    res <- ds$get_pixel_line(c(NA, NA))
    expect_true(all(is.na(res)))

    ds$close()
})

test_that("fillNodata writes correct output", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    mod_file <- paste0(tempdir(), "/", "storml_elev_fill.tif")
    file.copy(elev_file,  mod_file)
    fillNodata(mod_file, band=1)
    ds <- new(GDALRaster, mod_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 49103)

    # invalid band
    expect_error(fillNodata(mod_file, band=2))
    # incorrect mask file
    expect_error(fillNodata(mod_file, band=1, mask_file="_err_"))

    # clean up
    deleteDataset(mod_file)
})

test_that("sieveFilter runs without error", {
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    evt_mmu_file <- paste0(tempdir(), "/", "storml_evt_mmu2.tif")
    rasterFromRaster(srcfile=evt_file, dstfile=evt_mmu_file, init=32767)
    expr <- "ifelse(EVT == 7292, 0, EVT)"
    mask_file <- calc(expr, rasterfiles=evt_file, var.names="EVT")
    expect_true(sieveFilter(evt_file, 1, evt_mmu_file, 1, 2, 8, mask_file, 1))

    # invalid source band
    expect_error(sieveFilter(evt_file, 2, evt_mmu_file, 1, 2, 8, mask_file, 1))
    # incorrect destination file
    expect_error(sieveFilter(evt_file, 1, "_err_", 1, 2, 8, mask_file, 1))
    # invalid destination band
    expect_error(sieveFilter(evt_file, 1, evt_mmu_file, 2, 2, 8, mask_file, 1))
    # incorrect mask file
    expect_error(sieveFilter(evt_file, 1, evt_mmu_file, 1, 2, 8, "_err_", 1))
    # invalid mask band
    expect_error(sieveFilter(evt_file, 1, evt_mmu_file, 1, 2, 8, mask_file, 2))

    # clean up
    deleteDataset(evt_mmu_file)
    deleteDataset(mask_file)
})

test_that("createColorRamp works", {
    colors <- createColorRamp(start_index = 0,
                              start_color = c(211, 211, 211),
                              end_index = 100,
                              end_color = c(0, 100, 0))
    expect_equal(nrow(colors), 101)
    # non-zero start_index
    colors = createColorRamp(109, c(254, 231, 152), 127, c(254, 254, 189))
    expect_equal(nrow(colors), 19)
})

test_that("bandCopyWholeRaster writes correct output", {
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    # make a copy since getStatistics will try to write a .xml file
    b5_tmp <- paste0(tempdir(), "/", "b5_tmp.tif")
    file.copy(b5_file,  b5_tmp)
    dst_file <- paste0(tempdir(), "/", "sr_multi.tif")
    rasterFromRaster(b5_tmp, dst_file, nbands=7, init=0)
    opt <- c("COMPRESSED=YES", "SKIP_HOLES=YES")
    bandCopyWholeRaster(b5_tmp, 1, dst_file, 5, options=opt)
    ds <- new(GDALRaster, b5_tmp, read_only=TRUE)
    on.exit(deleteDataset(b5_tmp))
    src_stats <- ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
    ds$close()
    ds <- new(GDALRaster, dst_file, read_only=TRUE)
    on.exit(deleteDataset(dst_file), add=TRUE)
    dst_stats <- ds$getStatistics(band=5, approx_ok=FALSE, force=TRUE)
    ds$close()
    expect_equal(src_stats, dst_stats)

    push_error_handler("quiet")
    # invalid source band
    expect_false(bandCopyWholeRaster(b5_tmp, 2, dst_file, 5, options=opt))
    # incorrect destination file
    expect_false(bandCopyWholeRaster(b5_tmp, 1, "_err_", 5, options=opt))
    # invalid destination band
    expect_false(bandCopyWholeRaster(b5_tmp, 1, dst_file, 8, options=opt))
    pop_error_handler()
})

test_that("deleteDataset works", {
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    b5_tmp <- paste0(tempdir(), "/", "b5_tmp.tif")
    file.copy(b5_file,  b5_tmp)
    ds <- new(GDALRaster, b5_tmp, read_only=TRUE)
    ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
    files <- ds$getFileList()
    ds$close()
    expect_true(all(file.exists(files)))
    deleteDataset(b5_tmp)
    expect_false(any(file.exists(files)))
    ds = NULL

    # with format argument
    b5_tmp2 <- paste0(tempdir(), "/", "b5_tmp2.tif")
    file.copy(b5_file,  b5_tmp2)
    ds2 <- new(GDALRaster, b5_tmp2, read_only=TRUE)
    ds2$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
    files <- ds2$getFileList()
    ds2$close()
    deleteDataset(b5_tmp2, "GTiff")
    expect_false(any(file.exists(files)))
})

test_that("renameDataset works", {
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    b5_tmp <- paste0(tempdir(), "/", "b5_tmp.tif")
    file.copy(b5_file,  b5_tmp)
    ds <- new(GDALRaster, b5_tmp, read_only=TRUE)
    ds$buildOverviews("BILINEAR", levels = c(2, 4, 8), bands = c(1))
    ds$close()
    b5_tmp2 <- paste0(tempdir(), "/", "b5_tmp_renamed.tif")
    renameDataset(b5_tmp2, b5_tmp)
    ds <- new(GDALRaster, b5_tmp2, read_only=TRUE)
    expect_length(ds$getFileList(), 2)
    ds$close()

    # with format argument
    b5_tmp3 <- paste0(tempdir(), "/", "b5_tmp3.tif")
    renameDataset(b5_tmp3, b5_tmp2)
    ds <- new(GDALRaster, b5_tmp3, read_only=TRUE)
    expect_length(ds$getFileList(), 2)
    ds$close()

    deleteDataset(b5_tmp3)
})

test_that("copyDatasetFiles works", {
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    ds <- new(GDALRaster, lcp_file, read_only=TRUE)
    num_files <- length(ds$getFileList())
    ds$close()
    lcp_tmp <- paste0(tempdir(), "/", "storm_lake_copy.lcp")
    copyDatasetFiles(lcp_tmp, lcp_file)
    ds_copy <- new(GDALRaster, lcp_tmp, read_only=TRUE)
    expect_equal(length(ds_copy$getFileList()), num_files)
    ds_copy$close()
    deleteDataset(lcp_tmp)

    # with format argument
    lcp_tmp2 <- paste0(tempdir(), "/", "storm_lake_copy2.lcp")
    copyDatasetFiles(lcp_tmp2, lcp_file)
    ds_copy2 <- new(GDALRaster, lcp_tmp2, read_only=TRUE)
    expect_equal(length(ds_copy2$getFileList()), num_files)
    ds_copy2$close()
    deleteDataset(lcp_tmp2)
})

test_that("buildVRT works", {
    b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    b6_file <- system.file("extdata/sr_b6_20200829.tif", package="gdalraster")
    band_files <- c(b6_file, b5_file, b4_file)
    vrt_file <- paste0(tempdir(), "/", "storml_b6_b5_b4.vrt")
    buildVRT(vrt_file, band_files, cl_arg="-separate")
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    expect_equal(ds$getRasterCount(), 3)
    ds$close()
    deleteDataset(vrt_file)
})

test_that("footprint runs without error", {
    skip_if(gdal_version_num() < 3080000)

    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    args <- c("-t_srs", "EPSG:4326")
    out_file <- paste0(tempdir(), "/", "storml.geojson")
    expect_true(footprint(evt_file, out_file, args))
    deleteDataset(out_file)
})

test_that("ogr2ogr works", {
    # this may be removed in the future
    # cf. https://gdal.org/en/stable/programs/ogr2ogr.html#known-issues
    set_config_option("OGR2OGR_USE_ARROW_API", "NO")
    on.exit(set_config_option("OGR2OGR_USE_ARROW_API", ""))

    src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")

    # convert GeoPackage to Shapefile
    shp_file <- file.path(tempdir(), "ynp_fires.shp")
    ogr2ogr(src, shp_file, src_layers = "mtbs_perims")
    fld_idx <- .ogr_field_index(shp_file, "ynp_fires", "ig_year")
    expect_equal(fld_idx, 8)
    deleteDataset(shp_file)
    fld_idx <- -1

    # reproject to WGS84
    ynp_wgs84 <- file.path(tempdir(), "ynp_fires_wgs84.gpkg")
    args <- c("-t_srs", "EPSG:4326")
    ogr2ogr(src, ynp_wgs84, cl_arg = args)
    fld_idx <- .ogr_field_index(ynp_wgs84, "mtbs_perims", "ig_year")
    expect_equal(fld_idx, 8)
    deleteDataset(ynp_wgs84)
    fld_idx <- -1

    # with open_options
    vsi_curl_clear_cache()
    ogr2ogr(src, ynp_wgs84, cl_arg = args, open_options = "LIST_ALL_TABLES=NO")
    fld_idx <- .ogr_field_index(ynp_wgs84, "mtbs_perims", "ig_year")
    expect_equal(fld_idx, 8)
    deleteDataset(ynp_wgs84)
    fld_idx <- -1

    # clip to a bounding box (xmin, ymin, xmax, ymax)
    ynp_clip <- file.path(tempdir(), "ynp_fires_aoi_clip.gpkg")
    bb <- c(469685.97, 11442.45, 544069.63, 85508.15)
    args <- c("-spat", bb)
    ogr2ogr(src, ynp_clip, cl_arg = args)
    fld_idx <- .ogr_field_index(ynp_clip, "mtbs_perims", "ig_year")
    expect_equal(fld_idx, 8)
    deleteDataset(ynp_clip)
    fld_idx <- -1

    # filter features by a -where clause
    ynp_filtered <- file.path(tempdir(), "ynp_fires_2000_2022.gpkg")
    sql <- "ig_year >= 2000 ORDER BY ig_year"
    args <- c("-where", sql)
    ogr2ogr(src, ynp_filtered, src_layers = "mtbs_perims", cl_arg = args)
    fld_idx <- .ogr_field_index(ynp_filtered, "mtbs_perims", "ig_year")
    expect_equal(fld_idx, 8)
    deleteDataset(ynp_filtered)
})

test_that("ogrinfo works", {
    skip_if(gdal_version_num() < 3070000)

    src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")

    info <- ogrinfo(src)
    expect_vector(info, ptype = character(), size = 1)
    expect_false(info[1] == "")

    json <- ogrinfo(src, "mtbs_perims", cl_arg = c("-json", "-so", "-nomd"))
    expect_true(nchar(json) > 1000)

    src_mem <- paste0("/vsimem/", basename(src))
    vsi_copy_file(src, src_mem)

    args <- c("-sql", "ALTER TABLE mtbs_perims ADD burn_bnd_ha float")
    ogrinfo(src_mem, cl_arg = args, read_only = FALSE)
    idx <- .ogr_field_index(src_mem, "mtbs_perims", "burn_bnd_ha")
    expect_equal(idx, 9)

    vsi_unlink(src_mem)
})

test_that("autoCreateWarpedVRT works", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    ds <- new(GDALRaster, elev_file)

    expect_no_error(ds2 <- autoCreateWarpedVRT(ds, epsg_to_wkt(5070),
                                               "Bilinear"))
    expect_equal(ds2$getProjection(), epsg_to_wkt(5070))
    ds2$close()

    # with creation of alpha band
    expect_no_error(ds3 <- autoCreateWarpedVRT(ds, epsg_to_wkt(5070),
                                               "Cubic", alpha_band = TRUE))
    expect_equal(ds3$getRasterCount(), 2)
    ds3$close()

    # errors
    expect_error(ds4 <- autoCreateWarpedVRT(ds, epsg_to_wkt(5070),
                                            resample_alg = "invalid"))

    ds$close()
})

test_that("identifyDriver works", {
    src <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")

    expect_equal(identifyDriver(src), "GPKG")
    expect_equal(identifyDriver(src, raster = FALSE), "GPKG")
    expect_equal(identifyDriver(src, vector = FALSE), "GPKG")
    expect_equal(identifyDriver(src, allowed_drivers = c("GPKG", "GTiff")), "GPKG")
    expect_true(is.null(identifyDriver(src, allowed_drivers = c("GTiff", "GeoJSON"))))
    expect_equal(identifyDriver(src, file_list = "ynp_fires_1984_2022.gpkg"), "GPKG")

    # PostGISRaster vs. PostgreSQL
    skip_if(nrow(gdal_formats("PostgreSQL")) == 0 ||
            nrow(gdal_formats("PostGISRaster")) == 0)

    dsn <- "PG:dbname='testdb', host='127.0.0.1' port='5444' user='user' password='pwd'"
    expect_equal(identifyDriver(dsn), "PostGISRaster")
    expect_equal(identifyDriver(dsn, raster = FALSE), "PostgreSQL")
})

test_that("flip_vertical works", {
    v <- seq_len(9)
    v_flip <- .flip_vertical(v, 3, 3, 1)
    expect_equal(v_flip, c(7, 8, 9, 4, 5, 6, 1, 2, 3))

    expect_error(.flip_vertical(v, 5, 5, 1))
    expect_error(.flip_vertical(v, -3, -3, 1))
})

test_that("validateCreationOptions works", {
    expect_true(validateCreationOptions("GTiff", c("COMPRESS=LZW",
                                                   "TILED=YES")))
    expect_false(validateCreationOptions("GTiff", "COMPRESS=invalid"))
})

test_that("gdal_get_driver_md works", {
    expect_no_error(md <- gdal_get_driver_md("GTiff"))
    expect_true(is.list(md))
    expect_equal(md$DMD_LONGNAME, "GeoTIFF")

    expect_no_error(mdi <- gdal_get_driver_md("GTiff", "DCAP_RASTER"))
    expect_equal(mdi, "YES")

    expect_true(is.null(gdal_get_driver_md("GTiff", "invalid")))
})
