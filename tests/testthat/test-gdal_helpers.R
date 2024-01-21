test_that("addFilesInZip works", {
	# requires GDAL >= 3.7
	skip_if(as.integer(gdal_version()[2]) < 3070000)
	
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	evc_file <- system.file("extdata/storml_evc.tif", package="gdalraster")
	evh_file <- system.file("extdata/storml_evh.tif", package="gdalraster")
	files_to_add <- c(evt_file, evc_file, evh_file)
	zip_file <- paste0(tempdir(), "/", "storml.zip")
	addFilesInZip(
			zip_file,
			files_to_add,
			full_paths=FALSE,
			sozip_enabled="YES",
			sozip_chunk_size=16384,
			sozip_min_file_size=1024,
			content_type="TEST",
			num_threads=1)
	d <- unzip(zip_file, list=TRUE)
	expect_equal(nrow(d), 3)
	unlink(zip_file)
})

test_that("getCreationOptions works", {
	expect_no_error(getCreationOptions("GTiff"))
	expect_output(x <- getCreationOptions("GTiff", filter="COMPRESS"))
	expect_type(x, "character")
	expect_message(getCreationOptions("AIG"))
})
