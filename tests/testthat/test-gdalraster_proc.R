test_that(".getGDALformat works", {
    expect_equal(.getGDALformat("test.tif"), "GTiff")
    expect_equal(.getGDALformat("test.TIF"), "GTiff")
    expect_equal(.getGDALformat("test.img"), "HFA")
    expect_equal(.getGDALformat("test.vrt"), "VRT")
    expect_equal(.getGDALformat("test.VRT"), "VRT")
    expect_null(.getGDALformat("test.unknown"))
})

test_that("calc writes correct results", {
    # bioclimatic index
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    expr <- "round( ((ELEV_M * 3.281 - 5449) / 100) +
                ((pixelLat - 42.16) * 4) +
                ((-116.39 - pixelLon) * 1.25) )"
    hi_file <- calc(expr = expr,
                    rasterfiles = elev_file,
                    var.names = "ELEV_M",
                    dtName = "Int16",
                    setRasterNodataValue = TRUE)
    on.exit(deleteDataset(hi_file), add=TRUE)
    ds <- new(GDALRaster, hi_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 2978)

    # NDVI
    b4_file <- system.file("extdata/sr_b4_20200829.tif", package="gdalraster")
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    expr <- "((B5-B4)/(B5+B4)) * 1000" # rescale for checksum
    ndvi_file <- calc(expr = expr,
                      rasterfiles = c(b4_file, b5_file),
                      var.names = c("B4", "B5"),
                      dtName = "Float32",
                      nodata_value = -32767,
                      setRasterNodataValue = TRUE)
    on.exit(deleteDataset(ndvi_file), add=TRUE)
    ds <- new(GDALRaster, ndvi_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 63632)

    # recode
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
    on.exit(deleteDataset(tif_file), add=TRUE)
    createCopy("GTiff", tif_file, lcp_file)
    expr <- "ifelse( SLP >= 40 & FBFM %in% c(101,102), 99, FBFM)"
    calc(expr = expr,
         rasterfiles = c(lcp_file, lcp_file),
         bands = c(2, 4),
         var.names = c("SLP", "FBFM"),
         dstfile = tif_file,
         out_band = 4,
         write_mode = "update")
    ds <- new(GDALRaster, tif_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 28017)

    # multiband output
    # https://github.com/USDAForestService/gdalraster/issues/319
    expr <- "
to_terrainrgb <- function(dtm) {
    startingvalue <- 10000
    precision <- 0.1
    rfactor <- 256*256 * precision
    gfactor <- 256 * precision

    r <- floor((startingvalue +dtm)*(1/precision) / 256 / 256)
    g <- floor((startingvalue +dtm - r*rfactor)*(1/precision) / 256)
    b <- floor((startingvalue +dtm - r*rfactor - g*gfactor)*(1/precision))

    return(c(r,g,b))
}
to_terrainrgb(ELEV)"

    out_file <- "/vsimem/multiband-calc.tif"
    result <- calc(expr = expr,
                   rasterfiles = elev_file,
                   var.names = "ELEV",
                   dstfile = out_file,
                   dtName = "Byte",
                   out_band = 1:3,
                   nodata_value = 0)
    expect_equal(result, out_file)

    # revert out_file using from_terrainrgb()
    expr <- "
from_terrainrgb <- function(R,G,B) {
  elevation <- -10000 + ((R * 256 * 256 + G * 256 + B) * 0.1)
  return(elevation)
}
from_terrainrgb(R,G,B)"

    revert_file <- "/vsimem/multiband-calc-revert.tif"
    result <- calc(expr = expr,
                   rasterfiles = c(out_file, out_file, out_file),
                   bands = c(1, 2 ,3),
                   var.names = c("R", "G", "B"),
                   dstfile = revert_file,
                   dtName = "Int16",
                   nodata_value = -10000,
                   setRasterNodataValue = TRUE)
    expect_equal(result, revert_file)
    # compare revert_file with the original elev_file
    ds <- new(GDALRaster, elev_file)
    orig_elev_values <- read_ds(ds)
    attributes(orig_elev_values) <- NULL
    ds$close()
    ds2 <- new(GDALRaster, revert_file)
    revert_elev_values <- read_ds(ds2)
    attributes(revert_elev_values) <- NULL
    ds2$close()
    expect_equal(revert_elev_values, orig_elev_values)
    expect_equal(mean(revert_elev_values, na.rm = TRUE),
                 mean(orig_elev_values, na.rm = TRUE))
    deleteDataset(out_file)
    deleteDataset(revert_file)

    # test errors from input validation
    expr <- "((B5-B4)/(B5+B4))"
    expect_error(calc(expr = expr,
                      rasterfiles = c(b4_file, paste0(b5_file, ".error")),
                      var.names = c("B4", "B5"),
                      dtName = "Float32",
                      nodata_value = -32767,
                      setRasterNodataValue = TRUE))

    expect_error(calc(expr = expr,
                      rasterfiles = c(b4_file, b5_file),
                      bands = 1,
                      var.names = c("B4", "B5"),
                      dtName = "Float32",
                      nodata_value = -32767,
                      setRasterNodataValue = TRUE))

    expect_error(calc(expr = expr,
                      rasterfiles = c(b4_file, b5_file),
                      var.names = c("B4", "B5", "err"),
                      dtName = "Float32",
                      nodata_value = -32767,
                      setRasterNodataValue = TRUE))

    expect_error(calc(expr = expr,
                      rasterfiles = c(b4_file, b5_file),
                      dtName = "Float32",
                      nodata_value = -32767,
                      setRasterNodataValue = TRUE))

    expect_error(calc(expr = expr,
                      rasterfiles = c(b4_file, b5_file),
                      var.names = c("B4", "B5"),
                      dstfile = "unknown_file_type.err",
                      dtName = "Float32",
                      nodata_value = -32767,
                      setRasterNodataValue = TRUE))

    expect_error(calc(expr = expr,
                      rasterfiles = c(b4_file, b5_file),
                      var.names = c("B4", "B5"),
                      dtName = "Int64",
                      setRasterNodataValue = TRUE))

    expect_error(calc(expr = expr,
                      rasterfiles = c(b4_file, elev_file),
                      var.names = c("B4", "B5"),
                      dtName = "Float32",
                      nodata_value = -32767,
                      setRasterNodataValue = TRUE))
})

test_that("combine writes correct output", {
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    rasterfiles <- c(lcp_file, lcp_file)
    bands <- c(4, 5)
    var.names <- c("fbfm", "tree_cov")
    cmb_file <- paste0(tempdir(), "/", "fbfm_cov_cmbid.tif")
    on.exit(deleteDataset(cmb_file))
    df <- combine(rasterfiles, var.names, bands, cmb_file)
    expect_equal(nrow(df), 29)
    df <- NULL
    ds <- new(GDALRaster, cmb_file, TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 43024)

    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    df <- combine(evt_file)
    expect_equal(nrow(df), 24)
})

test_that("rasterFromRaster works", {
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    slpp_file <- paste0(tempdir(), "/", "storml_slpp.tif")
    on.exit(deleteDataset(slpp_file), add=TRUE)
    options = c("COMPRESS=LZW")
    rasterFromRaster(srcfile = lcp_file,
                     dstfile = slpp_file,
                     nbands = 1,
                     dtName = "Int16",
                     options = options,
                     init = -32767)
    ds <- new(GDALRaster, slpp_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 47771)

    slpp_file_img <- paste0(tempdir(), "/", "storml_slpp.img")
    on.exit(deleteDataset(slpp_file_img), add=TRUE)
    options = c("COMPRESSED=YES")
    rasterFromRaster(srcfile = lcp_file,
                     dstfile = slpp_file_img,
                     nbands = 1,
                     dtName = "Int16",
                     options = options,
                     init = -32767)
    ds <- new(GDALRaster, slpp_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 47771)

    slpp_file_envi <- paste0(tempdir(), "/", "storml_slpp")
    on.exit(deleteDataset(slpp_file_envi), add=TRUE)
    # error on unknown file format:
    expect_error(rasterFromRaster(srcfile = lcp_file,
                                  dstfile = slpp_file_envi,
                                  nbands = 1,
                                  dtName = "Int16",
                                  init = -32767))
    rasterFromRaster(srcfile = lcp_file,
                     dstfile = slpp_file_envi,
                     fmt = "ENVI",
                     nbands = 1,
                     dtName = "Int16",
                     init = -32767)
    ds <- new(GDALRaster, slpp_file_envi, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 47771)
})

test_that("rasterToVRT works", {
    ## resample
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    vrt_file <- rasterToVRT(evt_file,
                            resolution=c(90,90),
                            resampling="mode")
    on.exit(deleteDataset(vrt_file), add=TRUE)
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 23684)

    ## clip raster to polygon extent
    bnd = "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
    5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
    325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"

    # src_align = TRUE
    vrt_file <- rasterToVRT(evt_file,
                            subwindow = bbox_from_wkt(bnd),
                            src_align=TRUE)
    on.exit(deleteDataset(vrt_file), add=TRUE)
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 18148)

    # src_align = FALSE
    vrt_file <- rasterToVRT(evt_file,
                            subwindow = bbox_from_wkt(bnd),
                            src_align=FALSE)
    on.exit(deleteDataset(vrt_file), add=TRUE)
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 17631)

    # subwindow outside raster extent
    expect_error(rasterToVRT(evt_file,
                             subwindow = bbox_from_wkt(g_buffer(bnd, 10000)),
                             src_align=TRUE))

    ## subset and pixel align two rasters
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    ds_lcp <- new(GDALRaster, lcp_file, read_only=TRUE)
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    vrt_file <- rasterToVRT(b5_file,
                            resolution = ds_lcp$res(),
                            subwindow = ds_lcp$bbox(),
                            src_align = FALSE)
    on.exit(deleteDataset(vrt_file), add=TRUE)
    ds_lcp$close()
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 49589)

    ## kernel filter
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    krnl <- c(0.11111, 0.11111, 0.11111,
              0.11111, 0.11111, 0.11111,
              0.11111, 0.11111, 0.11111)
    vrt_file <- rasterToVRT(elev_file, krnl=krnl)
    on.exit(deleteDataset(vrt_file), add=TRUE)
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 46590)

    expect_error(rasterToVRT(elev_file, resolution=c(90,90), krnl=krnl))
})

test_that("dem_proc runs without error", {
    elev_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")
    hs_file <- paste0(tempdir(), "/", "storml_hillshade.tif")
    on.exit(deleteDataset(hs_file), add=TRUE)
    expect_true(dem_proc("hillshade", elev_file, hs_file))

    sq <- seq(2400, 3100, by = 100)
    col <- col2rgb(hcl.colors(length(sq)))
    pal <- tempfile(fileext = ".txt")
    on.exit(unlink(pal), add=TRUE)
    writeLines(paste(sq, col[1, ], col[2, ], col[3, ]), pal)
    cr_file <- tempfile(fileext = ".tif")
    on.exit(deleteDataset(cr_file), add=TRUE)
    expect_true(dem_proc("color-relief", elev_file, cr_file, color_file = pal))
})

test_that("polygonize runs without error", {
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    dsn <- paste0(tempdir(), "/", "storml_evt.shp")
    layer <- "storml_evt"
    fld <- "evt_value"
    expect_true(polygonize(evt_file, dsn, layer, fld))

    # overwrite
    expect_true(polygonize(evt_file, dsn, layer, fld, connectedness=8,
                           overwrite=TRUE))

    # with mask_file
    expr <- "ifelse(EVT == -9999, 0, 1)"
    mask_file <- calc(expr, rasterfiles=evt_file, var.names="EVT")
    expect_true(polygonize(evt_file, dsn, layer, fld, mask_file=mask_file,
                           overwrite=TRUE))

    # nomask
    expect_true(polygonize(evt_file, dsn, layer, fld, nomask=TRUE,
                           overwrite=TRUE))

    # append: layer already exists so expect warning if lco given
    expect_warning(polygonize(evt_file, dsn, layer, fld, connectedness=8,
                              lco="SPATIAL_INDEX=YES"))

    deleteDataset(dsn)
    deleteDataset(mask_file)

    # GPKG
    set_config_option("SQLITE_USE_OGR_VFS", "YES")
    set_config_option("OGR_SQLITE_JOURNAL", "MEMORY")
    dsn <- paste0(tempdir(), "/", "storml_evt.gpkg")
    layer <- "lf_evt"
    opt <- "VERSION=1.3"
    expect_true(polygonize(evt_file, dsn, layer, fld, dsco=opt))

    # existing out_dsn, but create a new layer
    layer <- "lf_evt_8connect"
    opt <- c("GEOMETRY_NULLABLE=NO","DESCRIPTION=LF EVT 8-connected polygons")
    expect_true(polygonize(evt_file,dsn,layer,fld,connectedness=8,lco=opt))

    set_config_option("SQLITE_USE_OGR_VFS", "")
    set_config_option("OGR_SQLITE_JOURNAL", "")
    deleteDataset(dsn)

    # unrecognized output format
    dsn <- paste0(tempdir(), "/", "storml_evt.txt")
    expect_error(polygonize(evt_file, dsn, layer, fld))
})

test_that("rasterize runs without error", {
    # layer from sql query
    dsn <- system.file("extdata/ynp_fires_1984_2022.gpkg", package="gdalraster")
    sql <- "SELECT * FROM mtbs_perims ORDER BY mtbs_perims.ig_year"
    out_file <- paste0(tempdir(), "/", "ynp_fires_1984_2022.tif")

    res <- rasterize(src_dsn = dsn,
                     dstfile = out_file,
                     sql = sql,
                     burn_attr = "ig_year",
                     tr = c(90,90),
                     tap = TRUE,
                     dtName = "Int16",
                     dstnodata = -9999,
                     init = -9999,
                     co = c("TILED=YES","COMPRESS=LZW"))
    expect_true(res)
    ds <- new(GDALRaster, out_file, TRUE)
    bbox <- ds$bbox()
    dm <- ds$dim()
    ds$close()
    deleteDataset(out_file)

    # layer with where clause
    out_file <- paste0(tempdir(), "/", "ynp_fires_1988.tif")
    layer <- "mtbs_perims"
    where <- "ig_year = 1988"

    res <- rasterize(src_dsn = dsn,
                     dstfile = out_file,
                     layer = layer,
                     where = where,
                     burn_value = 1,
                     te = bbox,
                     ts = dm[1:2],
                     dtName = "Byte",
                     init = 0,
                     fmt = "GTiff",
                     co = c("TILED=YES","COMPRESS=LZW"),
                     add_options = "-q")
    expect_true(res)
    deleteDataset(out_file)

    # update existing raster in-place
    # create the vector layer
    dsn <- tempfile(fileext = ".geojson")
    lyr <- ogr_ds_create("GeoJSON", dsn, "OGRGeoJSON", geom_type = "POLYGON",
                         return_obj = TRUE)
    pts <-  matrix(c(0.25, 0.25, 0.75, 0.25, 0.75, 0.75, 0.25, 0.75, 0.25, 0.25),
                   ncol = 2, byrow = TRUE)
    feat <- list()
    feat$geometry <- g_create("POLYGON", pts)
    lyr$createFeature(feat)
    lyr$close()
    # create the destination raster
    ds <- create("GTiff", "/vsimem/test.tif", xsize = 100, ysize = 100,
                 nbands = 1, dataType = "Byte", options = "COMPRESS=DEFLATE",
                 return_obj = TRUE)
    ds$setGeoTransform(c(0.0, 0.01, 0.0, 1.0, 0.0, -0.01))
    ds$setProjection(epsg_to_wkt(4326))
    ds$fillRaster(1, 0, 0)
    # update the destination pixels in-place
    res <- rasterize(dsn, ds, burn_value = 1)
    expect_true(res)
    r <- read_ds(ds)
    expect_equal(sum(r), 2500)

    unlink(dsn)
    ds$close()
    vsi_unlink("/vsimem/test.tif")
})

test_that("pixel_extract wrapper returns correct data", {
    # the C++ class method GDALRaster::pixel_extract() is tested in
    # test-GDALRaster-class.R

    pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
    pts <- read.csv(pt_file)
    raster_file <- system.file("extdata/storml_elev_orig.tif", package="gdalraster")

    # single pixel (1x1) extract on matrix input
    extr <- pixel_extract(raster_file, pts[-1])
    colnames(extr) <- NULL
    dim(extr) <- NULL
    expected_values <- c(2648, 2865, 2717, 2560, 2916,
                         2633, 2548, 2801, 2475, 2822)
    expect_equal(extr, expected_values)
    rm(extr)
    # data frame input
    extr <- pixel_extract(raster_file, pts)
    expect_equal(extr[, 2], expected_values)
    expect_equal(names(extr), c("id", "storml_elev_orig"))
    rm(extr)
    # as_data_frame from matrix input, so no input point IDs
    extr <- pixel_extract(raster_file, pts[-1], as_data_frame = TRUE)
    expect_true(is.data.frame(extr))
    expect_equal(names(extr), c("ptid", "storml_elev_orig"))
    expect_equal(extr[, 2], expected_values)
    expect_equal(extr[, 1], seq_len(nrow(extr)))
    rm(extr)

    # raster as a GDALRaster object
    ds <- new(GDALRaster, raster_file)
    extr_ds <- pixel_extract(ds, pts)
    expect_equal(extr_ds[, 2], expected_values)
    expect_equal(names(extr_ds), c("id", "storml_elev_orig"))
    # with missing values in the input
    pts_na <- rbind(pts, c(11, NA_real_, NA_real_))
    extr_na <- pixel_extract(ds, pts_na)
    expect_na <- c(expected_values, NA_real_)
    expect_equal(extr_na[, 2], expect_na)
    # with NaN in the input
    pts_nan <- rbind(pts, c(11, NaN, NaN))
    extr_nan <- pixel_extract(ds, pts_na)
    expect_nan <- c(expected_values, NA_real_)
    expect_equal(extr_nan[, 2], expect_nan)

    # MEM dataset with band names
    ds_mem <- create("MEM", "", 10, 10, 3, "Int16", return_obj = TRUE)
    expect_true(is(ds_mem, "Rcpp_GDALRaster"))
    ds_mem$setDescription(1, "elev")
    ds_mem$setDescription(2, "asp")
    ds_mem$setDescription(3, "slpp")
    ds_mem$fillRaster(1, 1000, 0)
    ds_mem$fillRaster(2, 100, 0)
    ds_mem$fillRaster(3, 10, 0)
    geo_xy <- matrix(c(5, -8, 0, -1), nrow = 2, ncol = 2, byrow = TRUE)
    # warning for no geotransform
    expect_warning(extr_mem <- pixel_extract(ds_mem, geo_xy,
                                             as_data_frame = TRUE))
    expect_true(is.data.frame(extr_mem))
    expect_equal(names(extr_mem), c("ptid", "b1_elev", "b2_asp", "b3_slpp"))
    expect_equal(extr_mem[, 2], c(1000, 1000))
    expect_equal(extr_mem[, 3], c(100, 100))
    expect_equal(extr_mem[, 4], c(10, 10))
    rm(extr_mem)
    # subset of bands, re-ordered
    expect_warning(extr_mem <- pixel_extract(ds_mem, geo_xy,
                                             bands = c(3, 2),
                                             as_data_frame = TRUE))
    expect_true(is.data.frame(extr_mem))
    expect_equal(names(extr_mem), c("ptid", "b3_slpp", "b2_asp"))
    expect_equal(extr_mem[, 2], c(10, 10))
    expect_equal(extr_mem[, 3], c(100, 100))
    rm(extr_mem)

    # GTiff without band names
    ds_mem$setDescription(1, "")
    ds_mem$setDescription(2, "")
    ds_mem$setDescription(3, "")
    tif_file <- tempfile(fileext = ".tif")
    ds_tif <- createCopy("GTiff", tif_file, ds_mem,
                         return_obj = TRUE)
    expect_true(is(ds_tif, "Rcpp_GDALRaster"))
    expect_warning(ds_tif$setGeoTransform(ds_mem$getGeoTransform()))
    extr_tif <- pixel_extract(ds_tif, geo_xy, as_data_frame = TRUE)
    expect_true(is.data.frame(extr_tif))
    ds_name <- tools::file_path_sans_ext(basename(ds_tif$getDescription(0)))
    expected_names <- c("ptid", paste0(rep(ds_name, 3), c("_b1", "_b2", "_b3")))
    expect_equal(names(extr_tif), expected_names)
    expect_equal(extr_tif[, 2], c(1000, 1000))
    expect_equal(extr_tif[, 3], c(100, 100))
    expect_equal(extr_tif[, 4], c(10, 10))
    rm(extr_tif)
    # subset of bands, re-ordered
    extr_tif <- pixel_extract(ds_tif, geo_xy,
                              bands = c(3, 2),
                              as_data_frame = TRUE)
    expect_true(is.data.frame(extr_tif))
    expected_names <- c("ptid", paste0(rep(ds_name, 2), c("_b3", "_b2")))
    expect_equal(names(extr_tif), expected_names)
    expect_equal(extr_tif[, 2], c(10, 10))
    expect_equal(extr_tif[, 3], c(100, 100))

    ds_mem$close()
    ds_tif$close()
    deleteDataset(tif_file)

    # interpolated values
    extr_bilinear <- pixel_extract(ds, pts, interp = "bilinear")
    expected_values <- c(2649.217, 2881.799, 2716.290, 2558.797, 2920.404,
                         2629.495, 2548.250, 2810.543, 2478.609, 2819.776)
    expect_equal(extr_bilinear[, 2], expected_values, tolerance = 1e-4)

    # kernel values
    extr_3x3 <- pixel_extract(ds, pts[-1], krnl_dim = 3)
    expect_equal(colnames(extr_3x3), c("b1_p1", "b1_p2", "b1_p3",
                                       "b1_p4", "b1_p5", "b1_p6",
                                       "b1_p7", "b1_p8", "b1_p9"))
    colnames(extr_3x3) <- NULL
    dimnames(extr_3x3) <- NULL
    expected_values <- c(
        2660,     2654, 2651,     2652, 2648, 2646,     2653, 2647, 2645,
        2924,     2895, 2901,     2893, 2865, 2869,     2863, 2838, 2841,
        NA_real_, 2709, 2704, NA_real_, 2717, 2717, NA_real_, 2725, 2728,
        2554,     2558, 2562,     2557, 2560, 2564,     2559, 2562, 2566,
        2932,     2920, 2908,     2927, 2916, 2903,     2923, 2911, 2899,
        2633,     2642, 2650,     2627, 2633, 2640,     2619, 2624, 2630,
        2548,     2548, 2550,     2549, 2548, 2550,     2550, 2548, 2550,
        2785,     2777, 2776,     2807, 2801, 2803,     2833, 2825, 2827,
        2489,     2482, 2475,     2479, 2475, 2470,     2474, 2471, 2468,
        2832,     2810, 2789,     2839, 2822, 2800,     2833, 2811, 2788)
    expected_values <- matrix(expected_values, nrow = 10, ncol = 9,
                              byrow = TRUE)
    expect_equal(extr_3x3, expected_values)

    # transform the xy
    pts_nad83 <- transform_xy(pts[-1], ds$getProjection(), "NAD83")
    extr <- pixel_extract(raster_file, pts_nad83, xy_srs = "NAD83")
    colnames(extr) <- NULL
    dim(extr) <- NULL
    expected_values <- c(2648, 2865, 2717, 2560, 2916,
                         2633, 2548, 2801, 2475, 2822)
    expect_equal(extr, expected_values)
    ds$close()
    rm(extr)


    # input validation
    expect_error(pixel_extract(ds, pts[-1]))  # ds closed
    expect_error(pixel_extract(NULL, pts[-1]))
    expect_error(pixel_extract(raster_file))
    expect_error(pixel_extract(raster_file, as.character(pts[-1])))
    expect_error(pixel_extract(raster_file, pts[, 2]))
    expect_error(pixel_extract(raster_file, pts, bands = c(1, 2, NA)))
    expect_no_error(pixel_extract(raster_file, pts, bands = 1.1))
    expect_error(pixel_extract(raster_file, pts,
                               interp = c("bilinear", "cubic")))
    expect_error(pixel_extract(raster_file, pts, krnl_dim = c(1, 1, 1)))
    expect_error(pixel_extract(raster_file, pts, xy_srs = 4326))
    expect_error(pixel_extract(raster_file, pts, max_ram = "invalid"))
    pts[, 2] <- as.character(pts[, 1])
    expect_error(pixel_extract(raster_file, pts))


    # test copy of remote raster to an in-memory dataset if not on CRAN
    skip_on_cran()
    skip_if(gdal_version_num() < gdal_compute_version(3, 6, 0))

    f <- "/vsicurl/https://raw.githubusercontent.com/usdaforestservice/gdalraster/main/sample-data/lf_fbfm40_220_mt_hood_utm.tif"
    pts <- c(604450.6, 5023283,
             611437.6, 5021571,
             591824.6, 5030855,
             613802.6, 5014220,
             600267.6, 5035648,
             613158.6, 5019868,
             616192.6, 5025546,
             597588.6, 5034797,
             595511.6, 5024383,
             591099.6, 5032626,
             600548.6, 5020417,
             598620.6, 5028004,
             599306.6, 5034376,
             612222.6, 5022980,
             604734.6, 5031885,
             601311.6, 5016818,
             611955.6, 5015290,
             605232.6, 5032306,
             615538.6, 5014872,
             600952.6, 5028567)
    pts <- matrix(pts, nrow = 20, ncol = 2, byrow = TRUE)
    expect_message(extr <- pixel_extract(f, pts), "copying to MEM")
    colnames(extr) <- NULL
    dim(extr) <- NULL
    expected_values <- c(99, 188, 185, 185, 185, 185, 185, 185, 162, 165, 162,
                         185, 185, 165, 185, 185, 165, 185, 165, 161)
    expect_equal(extr, expected_values)
    rm(extr)

    # force to use /vsimem/ instead
    expect_message(extr <- pixel_extract(f, pts, max_ram = 1),
                   "copy completed")
    colnames(extr) <- NULL
    dim(extr) <- NULL
    expected_values <- c(99, 188, 185, 185, 185, 185, 185, 185, 162, 165, 162,
                         185, 185, 165, 185, 185, 165, 185, 165, 161)
    expect_equal(extr, expected_values)
    rm(extr)
})
