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
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    expr <- "round( ((ELEV_M * 3.281 - 5449) / 100) +
                ((pixelLat - 42.16) * 4) +
                ((-116.39 - pixelLon) * 1.25) )"
    hi_file <- calc(expr = expr,
                    rasterfiles = elev_file,
                    var.names = "ELEV_M",
                    dtName = "Int16",
                    setRasterNodataValue = TRUE)
    on.exit(unlink(hi_file))
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
    on.exit(unlink(ndvi_file))
    ds <- new(GDALRaster, ndvi_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 63632)

    # recode
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
    on.exit(unlink(tif_file))
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
    on.exit(unlink(cmb_file))
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
    on.exit(unlink(slpp_file))
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
    on.exit(unlink(slpp_file_img))
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
    on.exit(unlink(slpp_file_envi))
    on.exit(unlink(paste0(slpp_file_envi, ".hdr")))
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
    on.exit(unlink(vrt_file))
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
    on.exit(unlink(vrt_file))
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 18148)

    # src_align = FALSE
    vrt_file <- rasterToVRT(evt_file,
                            subwindow = bbox_from_wkt(bnd),
                            src_align=FALSE)
    on.exit(unlink(vrt_file))
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 17631)

    # subwindow outside raster extent
    expect_error(rasterToVRT(evt_file,
                subwindow = bbox_from_wkt(.g_buffer(bnd, 10000)),
                src_align=TRUE))

    ## subset and pixel align two rasters
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    ds_lcp <- new(GDALRaster, lcp_file, read_only=TRUE)
    b5_file <- system.file("extdata/sr_b5_20200829.tif", package="gdalraster")
    vrt_file <- rasterToVRT(b5_file,
                            resolution = ds_lcp$res(),
                            subwindow = ds_lcp$bbox(),
                            src_align = FALSE)
    on.exit(unlink(vrt_file))
    ds_lcp$close()
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 49589)

    ## kernel filter
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    krnl <- c(0.11111, 0.11111, 0.11111,
              0.11111, 0.11111, 0.11111,
              0.11111, 0.11111, 0.11111)
    vrt_file <- rasterToVRT(elev_file, krnl=krnl)
    on.exit(unlink(vrt_file))
    ds <- new(GDALRaster, vrt_file, read_only=TRUE)
    dm <- ds$dim()
    chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
    ds$close()
    expect_equal(chk, 46590)

    expect_error(rasterToVRT(elev_file, resolution=c(90,90), krnl=krnl))
})

test_that("dem_proc runs without error", {
    elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
    hs_file <- paste0(tempdir(), "/", "storml_hillshade.tif")
    on.exit(unlink(hs_file))
    expect_true(dem_proc("hillshade", elev_file, hs_file))
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
})
