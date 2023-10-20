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
})

