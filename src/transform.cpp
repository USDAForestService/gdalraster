/* Functions for coordinate transformation 
   Chris Toney <chris.toney at usda.gov> */

#include <Rcpp.h> 
// [[Rcpp::plugins(cpp11)]]

#include <string>

#include "ogr_core.h"
#include "ogr_srs_api.h"
#include "ogr_spatialref.h"

//' Inverse project geospatial x/y coordinates to longitude/latitude
//'
//' `inv_project()` transforms geospatial x/y coordinates to 
//' longitude/latitude in the same geographic coordinate system used by the 
//' given projected spatial reference system. The output long/lat can 
//' optionally be set to a specific geographic coordinate system by specifying 
//' a well known name (see Details).
//'
//' @details
//' By default, the geographic coordinate system of the projection specified 
//' by `srs` will be used. If a specific geographic coordinate system is 
//' desired, then `well_known_gcs` can be set to one of the values below:
//' \tabular{rl}{
//'  `EPSG:n` \tab where `n` is the code of a geographic CRS\cr
//'  `WGS84`  \tab same as `EPSG:4326`\cr
//'  `WGS72`  \tab same as `EPSG:4322`\cr
//'  `NAD83`  \tab same as `EPSG:4269`\cr
//'  `NAD27`  \tab same as `EPSG:4267`\cr
//'  `CRS84`  \tab same as `WGS84`\cr
//'  `CRS72`  \tab same as `WGS72`\cr
//'  `CRS27`  \tab same as `NAD27`
//' }
//' The returned array will always be in longitude, latitude order 
//' (traditional GIS order) regardless of the axis order defined for the 
//' names above.
//'
//' `inv_project()` is included here as a convenience function mainly for 
//' internal use. See package `sf` for more full-featured  
//' coordinate transformation (\url{https://r-spatial.github.io/sf/}).
//'
//' @param pts Numeric array of geospatial x/y coordinates 
//' @param srs Character string in OGC WKT format specifying the projected 
//' spatial reference system for `pts`.
//' @param well_known_gcs Optional character string containing a supported 
//' well known name of a geographic coordinate system (see Details for 
//' supported values).
//' @returns Numeric array of longitude, latitude. An error is raised if the 
//' transformation cannot be performed.
//' @seealso
//' [transform_xy()]
//' @examples
//' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
//' ## id, x, y in NAD83 / UTM zone 12N
//' pts <- read.csv(pt_file)
//' print(pts)
//' inv_project(as.matrix(pts[,-1]), epsg_to_wkt(26912))
//' inv_project(as.matrix(pts[,-1]), epsg_to_wkt(26912), "NAD27")
// [[Rcpp::export]]
Rcpp::NumericMatrix inv_project(Rcpp::NumericMatrix &pts, 
								std::string srs,
								std::string well_known_gcs = "") {

	if (pts.nrow() == 0)
		Rcpp::stop("Input array is empty.");

	Rcpp::NumericMatrix pts_in = Rcpp::clone(pts);

	OGRSpatialReference oSourceSRS;
	OGRSpatialReference *poLongLat;
	OGRCoordinateTransformation *poCT;
	OGRErr err;
	
	err = oSourceSRS.importFromWkt(srs.c_str());
	if (err != OGRERR_NONE)
		Rcpp::stop("Failed to import SRS from WKT string.");
	
	if (well_known_gcs == "") {
		poLongLat = oSourceSRS.CloneGeogCS();
	}
	else {
		poLongLat = new OGRSpatialReference();
		err = poLongLat->SetWellKnownGeogCS(well_known_gcs.c_str());
		if (err == OGRERR_FAILURE)
			Rcpp::stop("Failed to set well known GCS.");
	}
#if GDAL_VERSION_NUM >= 3000000
	poLongLat->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif

	poCT = OGRCreateCoordinateTransformation(&oSourceSRS, poLongLat);
	if (poCT == NULL)
		Rcpp::stop("Failed to create coordinate transformer.");
	
	poLongLat->Release();
	
	Rcpp::NumericVector x = pts_in(Rcpp::_ , 0);
	Rcpp::NumericVector y = pts_in(Rcpp::_ , 1);
	std::vector<double> xbuf = Rcpp::as<std::vector<double>>(x);
	std::vector<double> ybuf = Rcpp::as<std::vector<double>>(y);
	if( !poCT->Transform(pts_in.nrow(), xbuf.data(), ybuf.data()) )
		Rcpp::stop("Coordinate transformation failed.");
	
	Rcpp::NumericMatrix pts_out(pts_in.nrow(), 2);
	for (R_xlen_t i=0; i < pts_in.nrow(); ++i) {
		pts_out(i,0) = xbuf[i];
		pts_out(i,1) = ybuf[i];
	}
	return pts_out;
}

//' Transform geospatial x/y coordinates
//'
//' `transform_xy()` transforms geospatial x/y coordinates to a new projection.
//'
//' @note
//' `transform_xy()` is included here as a convenience function mainly for 
//' internal use. See package `sf` for more full-featured 
//' coordinate transformation (\url{https://r-spatial.github.io/sf/}).
//'
//' @param pts Numeric array of geospatial x/y coordinates 
//' @param srs_from Character string in OGC WKT format specifying the  
//' spatial reference system for `pts`.
//' @param srs_to Character string in OGC WKT format specifying the output 
//' spatial reference system.
//' @returns Numeric array of geospatial x/y coordinates in the projection 
//' specified by `srs_to`.
//'
//' @seealso
//' [epsg_to_wkt()], [srs_to_wkt()], [inv_project()]
//' @examples
//' pt_file <- system.file("extdata/storml_pts.csv", package="gdalraster")
//' pts <- read.csv(pt_file)
//' print(pts)
//' ## id, x, y in NAD83 / UTM zone 12N
//' ## transform to NAD83 / CONUS Albers
//' transform_xy( pts = as.matrix(pts[,-1]), 
//'               srs_from = epsg_to_wkt(26912), 
//'               srs_to = epsg_to_wkt(5070) )
// [[Rcpp::export]]
Rcpp::NumericMatrix transform_xy(Rcpp::NumericMatrix &pts, 
								std::string srs_from,
								std::string srs_to) {

	if (pts.nrow() == 0)
		Rcpp::stop("Input array is empty.");

	Rcpp::NumericMatrix pts_in = Rcpp::clone(pts);

	OGRSpatialReference oSourceSRS, oDestSRS;
	OGRCoordinateTransformation *poCT;
	OGRErr err;
	
	err = oSourceSRS.importFromWkt(srs_from.c_str());
	if (err != OGRERR_NONE)
		Rcpp::stop("Failed to import source SRS from WKT string.");
	
	err = oDestSRS.importFromWkt(srs_to.c_str());
	if (err != OGRERR_NONE)
		Rcpp::stop("Failed to import destination SRS from WKT string.");
#if GDAL_VERSION_NUM >= 3000000
	oDestSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
#endif

	poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oDestSRS);
	if (poCT == NULL)
		Rcpp::stop("Failed to create coordinate transformer.");
	
	Rcpp::NumericVector x = pts_in(Rcpp::_ , 0);
	Rcpp::NumericVector y = pts_in(Rcpp::_ , 1);
	std::vector<double> xbuf = Rcpp::as<std::vector<double>>(x);
	std::vector<double> ybuf = Rcpp::as<std::vector<double>>(y);
	if( !poCT->Transform(pts_in.nrow(), xbuf.data(), ybuf.data()) )
		Rcpp::stop("Coordinate transformation failed.");
	
	Rcpp::NumericMatrix ret(pts_in.nrow(), 2);
	for (R_xlen_t i=0; i < pts_in.nrow(); ++i) {
		ret(i,0) = xbuf[i];
		ret(i,1) = ybuf[i];
	}
	return ret;
}


