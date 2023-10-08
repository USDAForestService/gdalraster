test_that("buildRAT works", {
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	df <- buildRAT(evt_file, table_type="thematic")
	expect_equal(nrow(df), 24)
	expect_equal(ncol(df), 2)
	expect_equal(attr(df, "GDALRATTableType"), "thematic")
	expect_equal(attr(df$VALUE, "GFU"), "MinMax")
	expect_equal(attr(df$COUNT, "GFU"), "PixelCount")
	expect_equal(sum(df$COUNT), 15301)
})

