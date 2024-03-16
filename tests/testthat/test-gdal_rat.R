test_that("buildRAT/displayRAT work", {
    evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
    evt_csv <- system.file("extdata/LF20_EVT_220.csv", package="gdalraster")
    evt_tbl <- read.csv(evt_csv)
    evt_tbl <- evt_tbl[,1:7]
    rat <- buildRAT(evt_file, table_type="thematic", na_value=-9999,
            join_df=evt_tbl)
    expect_equal(nrow(rat), 24)
    expect_equal(ncol(rat), 8)
    expect_equal(attr(rat, "GDALRATTableType"), "thematic")
    expect_equal(attr(rat$VALUE, "GFU"), "MinMax")
    expect_equal(attr(rat$COUNT, "GFU"), "PixelCount")
    expect_equal(sum(rat$COUNT), 15301)
    tbl <- displayRAT(rat)
    expect_true(is(tbl, "gt_tbl"))

    # with alpha
    rat$A <- 255
    attr(rat$A, "GFU") <- "Alpha"
    tbl <- displayRAT(rat)
    expect_true(is(tbl, "gt_tbl"))

    # uint32 raster
    lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
    rasterfiles <- c(lcp_file, lcp_file)
    bands <- c(4, 5)
    var.names <- c("fbfm", "tree_cov")
    cmb_file <- paste0(tempdir(), "/", "cmb_uint32.tif")
    d <- combine(rasterfiles, var.names, bands, cmb_file)
    tbl <- buildRAT(cmb_file)
    expect_equal(nrow(tbl), nrow(d))
    expect_equal(sum(tbl$COUNT), sum(d$count))
    deleteDataset(cmb_file)
})

