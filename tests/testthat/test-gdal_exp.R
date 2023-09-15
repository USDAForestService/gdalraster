# Tests for src/gdal_exp.cpp - Exported stand-alone functions for gdalraster
# Some functions in gdal_exp.cpp are called by gdalraster_proc.R and
# tested there.
test_that("gdal_version returns vector", {
	expect_length(gdal_version(), 4)
})

test_that("get/set_config_option work", {
	co <- get_config_option("GDAL_CACHEMAX")
	set_config_option("GDAL_CACHEMAX", "64")
	expect_equal(get_config_option("GDAL_CACHEMAX"), "64")
	set_config_option("GDAL_CACHEMAX", co)
})

test_that("get_cache_used returns integer", {
	expect_type(get_cache_used(), "integer")
})

test_that("createCopy writes correct output", {
	lcp_file <- system.file("extdata/storm_lake.lcp", package="gdalraster")
	tif_file <- paste0(tempdir(), "/", "storml_lndscp.tif")
	options <- c("COMPRESS=LZW")
	createCopy(format="GTiff", dst_filename=tif_file, src_filename=lcp_file,
				options=options)
	ds <- new(GDALRaster, tif_file, read_only=FALSE)
	files <- ds$getFileList()
	on.exit(unlink(files))
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

test_that("_apply_geotransform gives correct result", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	ds <- new(GDALRaster, elev_file, read_only=TRUE)
	gt <- ds$getGeoTransform()
	ds$close()
	expect_equal(.apply_geotransform(gt, 1, 1), c(323506.1, 5105052.0))
})

test_that("warp runs without error", {
	elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
	args = c("-tr", "90", "90", "-r", "average", "-tap")
	args = c(args, "-of", "HFA", "-co", "COMPRESSED=YES")
	alb83_file <- paste0(tempdir(), "/", "storml_elev_alb83.img")
	on.exit(unlink(alb83_file))
	expect_true(warp(elev_file, alb83_file, t_srs="EPSG:5070", cl_arg = args))
})

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

test_that("sieveFilter runs without error", {
	evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
	evt_mmu_file <- paste0(tempdir(), "/", "storml_evt_mmu2.tif")
	on.exit(unlink(evt_mmu_file))
	rasterFromRaster(srcfile=evt_file, dstfile=evt_mmu_file, init=32767)
	expr <- "ifelse(EVT == 7292, 0, EVT)"
	mask_file <- calc(expr, rasterfiles=evt_file, var.names="EVT")
	on.exit(unlink(mask_file))
	expect_true(sieveFilter(evt_file, 1, evt_mmu_file, 1, 2, 8, mask_file, 1))
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
	files <- ds$getFileList()
	on.exit(unlink(files))
	src_stats <- ds$getStatistics(band=1, approx_ok=FALSE, force=TRUE)
	ds$close()
	ds <- new(GDALRaster, dst_file, read_only=TRUE)
	files <- ds$getFileList()
	on.exit(unlink(files))
	dst_stats <- ds$getStatistics(band=5, approx_ok=FALSE, force=TRUE)
	ds$close()
	expect_equal(src_stats, dst_stats)
})

