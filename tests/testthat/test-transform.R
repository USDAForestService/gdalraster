# Tests for src/transform.cpp
test_that("transform_xy gives correct results", {
	pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
	pts <- read.csv(pt_file)
	xy_alb83 <- c(-1330885, -1331408, -1331994, -1330297, -1329991, -1329167,
					-1329903, -1329432, -1327683, -1331265,  2684892,  2684660,
					2685048,  2684967,  2683777,  2685212, 2685550,  2683821,
					2685541,  2685514)
	xy_test <- transform_xy(pts = as.matrix(pts[,-1]), 
							srs_from = epsg_to_wkt(26912), 
							srs_to = epsg_to_wkt(5070))
	expect_equal(round(as.vector(xy_test)), xy_alb83)
})
