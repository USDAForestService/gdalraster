/* Functions for coordinate transformation using PROJ via GDAL headers
   Chris Toney <chris.toney at usda.gov> */

#include "rcpp_util.h"

#include <string>

#include "ogr_core.h"
#include "ogr_srs_api.h"
#include "ogr_spatialref.h"

//' get PROJ version
//' @noRd
// [[Rcpp::export(name = ".getPROJVersion")]]
std::vector<int> _getPROJVersion() {
	int major, minor, patch;
	major = minor = patch = NA_INTEGER;
#if GDAL_VERSION_NUM >= 3000100	
	OSRGetPROJVersion(&major, &minor, &patch);
#endif
	std::vector<int> ret = {major, minor, patch};
	return ret;
}

//' get search path(s) for PROJ resource files
//' @noRd
// [[Rcpp::export(name = ".getPROJSearchPaths")]]
Rcpp::CharacterVector _getPROJSearchPaths() {
#if GDAL_VERSION_NUM >= 3000300
	char **papszPaths;
	papszPaths = OSRGetPROJSearchPaths();
	
	int items = CSLCount(papszPaths);
	if (items > 0) {
		Rcpp::CharacterVector paths(items);
		for (int i=0; i < items; ++i) {
			paths(i) = papszPaths[i];
		}
		CSLDestroy(papszPaths);
		return paths;
	}
	else {
		CSLDestroy(papszPaths);
		return "";	
	}
#endif
	return NA_STRING;
}

//' set search path(s) for PROJ resource files
//' @noRd
// [[Rcpp::export(name = ".setPROJSearchPaths")]]
void _setPROJSearchPaths(Rcpp::CharacterVector paths) {
#if GDAL_VERSION_NUM >= 3000000
	std::vector<char *> path_list = {NULL};
	path_list.resize(paths.size() + 1);
	for (R_xlen_t i = 0; i < paths.size(); ++i) {
		path_list[i] = (char *) (paths[i]);
	}
	path_list[paths.size()] = NULL;
	OSRSetPROJSearchPaths(path_list.data());
#else
	Rcpp::Rcerr << "OSRSetPROJSearchPaths requires GDAL 3.0 or later.\n";
#endif
	return;
}

//' get whether PROJ networking capabilities are enabled
//' returns logical NA if GDAL < 3.4
//' @noRd
// [[Rcpp::export(name = ".getPROJEnableNetwork")]]
Rcpp::LogicalVector _getPROJEnableNetwork() {
	Rcpp::LogicalVector ret = Rcpp::LogicalVector::create(NA_LOGICAL);
#if GDAL_VERSION_NUM >= 3040000
	if (_getPROJVersion()[0] >= 7) {
		ret[0] = OSRGetPROJEnableNetwork();
		return ret;
	}
	else {
		ret[0] = false;
		return ret;
	}
#endif
	return ret;
}

//' enable or disable PROJ networking capabilities
//' @noRd
// [[Rcpp::export(name = ".setPROJEnableNetwork")]]
void _setPROJEnableNetwork(int enabled) {
#if GDAL_VERSION_NUM >= 3040000
	if (_getPROJVersion()[0] >= 7)
		OSRSetPROJEnableNetwork(enabled);
	else
		Rcpp::Rcerr << "OSRSetPROJEnableNetwork requires PROJ 7 or later.\n";
#else
	Rcpp::Rcerr << "OSRSetPROJEnableNetwork requires GDAL 3.4 or later.\n";
#endif
	return;
}

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
//'  EPSG:n \tab where n is the code of a geographic coordinate system\cr
//'  WGS84  \tab same as EPSG:4326\cr
//'  WGS72  \tab same as EPSG:4322\cr
//'  NAD83  \tab same as EPSG:4269\cr
//'  NAD27  \tab same as EPSG:4267\cr
//'  CRS84  \tab same as WGS84\cr
//'  CRS72  \tab same as WGS72\cr
//'  CRS27  \tab same as NAD27
//' }
//' The returned array will always be in longitude, latitude order
//' (traditional GIS order) regardless of the axis order defined for the
//' names above.
//'
//' @param pts A two-column data frame or numeric matrix containing geospatial
//' x/y coordinates.
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
//' inv_project(pts[,-1], epsg_to_wkt(26912))
//' inv_project(pts[,-1], epsg_to_wkt(26912), "NAD27")
// [[Rcpp::export]]
Rcpp::NumericMatrix inv_project(Rcpp::RObject &pts, 
								std::string srs,
								std::string well_known_gcs = "") {

	Rcpp::NumericMatrix pts_m;
	if (Rcpp::is<Rcpp::DataFrame>(pts))
		pts_m = _df_to_matrix(pts);
	else if (Rcpp::is<Rcpp::NumericVector>(pts)) {
		if (Rf_isMatrix(pts))
			pts_m = Rcpp::as<Rcpp::NumericMatrix>(pts);
	}
	else
		Rcpp::stop("pts must be a data frame or matrix.");
		
	if (pts_m.nrow() == 0)
		Rcpp::stop("Input matrix is empty.");

	Rcpp::NumericMatrix pts_in = Rcpp::clone(pts_m);

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
//' @param pts A two-column data frame or numeric matrix containing geospatial
//' x/y coordinates.
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
//' transform_xy(pts = pts[,-1], 
//'              srs_from = epsg_to_wkt(26912), 
//'              srs_to = epsg_to_wkt(5070))
// [[Rcpp::export]]
Rcpp::NumericMatrix transform_xy(Rcpp::RObject &pts, 
								std::string srs_from,
								std::string srs_to) {

	Rcpp::NumericMatrix pts_m;
	if (Rcpp::is<Rcpp::DataFrame>(pts))
		pts_m = _df_to_matrix(pts);
	else if (Rcpp::is<Rcpp::NumericVector>(pts)) {
		if (Rf_isMatrix(pts))
			pts_m = Rcpp::as<Rcpp::NumericMatrix>(pts);
	}
	else
		Rcpp::stop("pts must be a data frame or matrix.");

	if (pts_m.nrow() == 0)
		Rcpp::stop("Input matrix is empty.");

	Rcpp::NumericMatrix pts_in = Rcpp::clone(pts_m);

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


