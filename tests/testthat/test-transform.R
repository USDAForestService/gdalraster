# Tests for src/transform.cpp
test_that("transform/inv_project give correct results", {
	pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
	pts <- read.csv(pt_file)
	xy_alb83 <- c(-1330885, -1331408, -1331994, -1330297, -1329991, -1329167,
					-1329903, -1329432, -1327683, -1331265,  2684892,  2684660,
					2685048,  2684967,  2683777,  2685212, 2685550,  2683821,
					2685541,  2685514)
	xy_test <- transform_xy(pts = as.matrix(pts[,-1]), 
							srs_from = epsg_to_wkt(26912), 
							srs_to = epsg_to_wkt(5070))
	expect_equal(round(as.vector(xy_test)), xy_alb83, tolerance=1)
	
	xy_wgs84 <- c(-113.26707, -113.27315, -113.28150, -113.25978, -113.25312,
					-113.24600, -113.25613, -113.24613, -113.22794, -113.27334,
					46.06118, 46.05827, 46.06076, 46.06280, 46.05276, 46.06682,
					46.06862, 46.05405, 46.07214, 46.06607)
	inv_test <- inv_project(pts = as.matrix(pts[,-1]),
							srs = epsg_to_wkt(26912),
							well_known_gcs = "WGS84")
	expect_equal(as.vector(inv_test), xy_wgs84, tolerance = 0.001)
})
