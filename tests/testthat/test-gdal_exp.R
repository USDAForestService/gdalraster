# Tests for src/gdal_exp.cpp - Exported stand-alone functions for gdalraster
# Some functions in gdal_exp.cpp are called by gdalraster_proc.R and
# tested there.
test_that("gdal_version returns vector", {
	expect_length(gdal_version(), 4)
})

test_that("get_pixel_line gives correct results", {
	pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
	pts <- read.csv(pt_file)
	pix_line <- c(39, 23, 1, 58, 74, 94, 68, 92, 141, 23, 57, 68, 58, 52, 90,
					38, 31, 85, 20, 39)
	raster_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
	ds <- new(GDALRaster, raster_file, read_only=TRUE)
	gt <- ds$getGeoTransform()
	ds$close()
	res <- get_pixel_line(as.matrix(pts[,-1]), gt)
	expect_equal(as.vector(res), pix_line)
})

#test_that("warp writes correct output", {
#	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
#	args = c("-tr", "90", "90", "-r", "average", "-tap")
#	args = c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
#	alb83_file <- paste0(tempdir(), "/", "storml_elev_alb83.img")
#	on.exit(unlink(alb83_file))
#	warp(elev_file, alb83_file, t_srs="EPSG:5070", cl_arg = args)
#	ds <- new(GDALRaster, alb83_file, read_only=TRUE)
#	dm <- ds$dim()
#	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
#	ds$close()
#	expect_equal(chk, 26902)
#})

test_that("fillNodata writes correct output", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	mod_file <- paste0(tempdir(), "/", "storml_elev_fill.tif")
	file.copy(elev_file,  mod_file)
	on.exit(unlink(mod_file))
	fillNodata(mod_file, band=1)
	ds <- new(GDALRaster, mod_file, read_only=TRUE)
	dm <- ds$dim()
	chk <- ds$getChecksum(1, 0, 0, dm[1], dm[2])
	ds$close()
	expect_equal(chk, 49103)
})

