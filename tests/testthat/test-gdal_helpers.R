test_that("getCreationOptions works", {
	expect_no_error(getCreationOptions("GTiff"))
	expect_output(x <- getCreationOptions("GTiff", filter="COMPRESS"))
	expect_type(x, "character")
	expect_message(getCreationOptions("AIG"))
})

