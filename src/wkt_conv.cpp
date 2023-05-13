/* WKT-related convenience functions
   Chris Toney <chris.toney at usda.gov> */

#include <Rcpp.h> 
// [[Rcpp::plugins(cpp11)]]

#include <string>

#include "cpl_conv.h"
#include "ogr_srs_api.h"
#include "ogr_spatialref.h"
#include "ogrsf_frmts.h"

//' Convert EPSG spatial reference to Well Known Text (WKT)
//'
//' `epsg_to_wkt()` exports the spatial reference for the specified EPSG code 
//' to WKT format.
//'
//' @details
//' As of GDAL 3.0, the default format for WKT export is OGC WKT 1.
//' The WKT version can be overridden by using the `OSR_WKT_FORMAT` 
//' configuration option (see [set_config_option()]).
//' Valid values are one of: `SFSQL`, `WKT1_SIMPLE`, `WKT1`, `WKT1_GDAL`, 
//' `WKT1_ESRI`, `WKT2_2015`, `WKT2_2018`, `WKT2`, `DEFAULT`.
//' If `SFSQL`, a WKT1 string without AXIS, TOWGS84, AUTHORITY or 
//' EXTENSION node is returned. If `WKT1_SIMPLE`, a WKT1 string without 
//' AXIS, AUTHORITY or EXTENSION node is returned. `WKT1` is an alias of 
//' `WKT1_GDAL`. `WKT2` will default to the latest revision implemented 
//' (currently `WKT2_2018`). `WKT2_2019` can be used as an alias of 
//' `WKT2_2018` since GDAL 3.2
//'
//' @param epsg Integer EPSG code.
//' @param pretty Logical. `TRUE` to return a nicely formatted WKT string 
//' for display to a person. `FALSE` for a regular WKT string (the default).
//' @return Character string containing OGC WKT.
//'
//' @seealso
//' [srs_to_wkt()]
//'
//' @examples
//' epsg_to_wkt(5070)
//' writeLines(epsg_to_wkt(5070, pretty=TRUE))
//' set_config_option("OSR_WKT_FORMAT", "WKT2")
//' writeLines(epsg_to_wkt(5070, pretty=TRUE))
//' set_config_option("OSR_WKT_FORMAT", "")
// [[Rcpp::export]]
std::string epsg_to_wkt(int epsg, bool pretty = false) {

	OGRSpatialReferenceH hSRS = OSRNewSpatialReference(NULL);
	char *pszSRS_WKT = NULL;
	
	if (OSRImportFromEPSG(hSRS, epsg) != OGRERR_NONE)
		Rcpp::stop("Error importing from EPSG code.");
	
	if (pretty) {
		if (OSRExportToPrettyWkt(hSRS, &pszSRS_WKT, false) != OGRERR_NONE) {
			OSRDestroySpatialReference(hSRS);
			Rcpp::stop("Error exporting to pretty WKT.");
		}
	}
	else {
		if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
			OSRDestroySpatialReference(hSRS);
			Rcpp::stop("Error exporting to WKT.");
		}
	}
	
	std::string wkt(pszSRS_WKT);
	OSRDestroySpatialReference(hSRS);
	CPLFree(pszSRS_WKT);
	
	return wkt;
}

//' Convert spatial reference definition to Well Known Text (WKT)
//'
//' `srs_to_wkt()` converts a spatial reference system (SRS) definition 
//' in various text formats to WKT. The function will examine the input SRS, 
//' try to deduce the format, and then export it to WKT.
//'
//' @details
//' The input SRS may take the following forms:
//'   * `WKT` - to convert WKT versions (see below)
//'   * `EPSG:n` - EPSG code n
//'   * \code{AUTO:proj_id,unit_id,lon0,lat0} - WMS auto projections
//'   * `urn:ogc:def:crs:EPSG::n` - OGC urns
//'   * PROJ.4 definitions
//'   * `filename` - file read for WKT, XML or PROJ.4 definition
//'   * well known name such as `NAD27`, `NAD83`, `WGS84` or `WGS72`
//'   * `IGNF:xxxx`, `ESRI:xxxx` - definitions from the PROJ database
//'   * PROJJSON (PROJ >= 6.2)
//'
//' This function is intended to be flexible, but by its nature it is 
//' imprecise as it must guess information about the format intended. 
//' [epsg_to_wkt()] should be used if the input is known to be an integer 
//' EPSG code.
//'
//' As of GDAL 3.0, the default format for WKT export is OGC WKT 1.
//' The WKT version can be overridden by using the `OSR_WKT_FORMAT` 
//' configuration option (see [set_config_option()]).
//' Valid values are one of: `SFSQL`, `WKT1_SIMPLE`, `WKT1`, `WKT1_GDAL`, 
//' `WKT1_ESRI`, `WKT2_2015`, `WKT2_2018`, `WKT2`, `DEFAULT`.
//' If `SFSQL`, a WKT1 string without AXIS, TOWGS84, AUTHORITY or 
//' EXTENSION node is returned. If `WKT1_SIMPLE`, a WKT1 string without 
//' AXIS, AUTHORITY or EXTENSION node is returned. `WKT1` is an alias of 
//' `WKT1_GDAL`. `WKT2` will default to the latest revision implemented 
//' (currently `WKT2_2018`). `WKT2_2019` can be used as an alias of 
//' `WKT2_2018` since GDAL 3.2
//'
//' @param srs Character string containing an SRS definition in various
//' formats (see Details).
//' @param pretty Logical. `TRUE` to return a nicely formatted WKT string 
//' for display to a person. `FALSE` for a regular WKT string (the default).
//' @return Character string containing OGC WKT.
//'
//' @seealso
//' [epsg_to_wkt()]
//'
//' @examples
//' srs_to_wkt("NAD83")
//' writeLines(srs_to_wkt("NAD83", pretty=TRUE))
//' set_config_option("OSR_WKT_FORMAT", "WKT2")
//' writeLines(srs_to_wkt("NAD83", pretty=TRUE))
//' set_config_option("OSR_WKT_FORMAT", "")
// [[Rcpp::export]]
std::string srs_to_wkt(std::string srs, bool pretty = false) {

	OGRSpatialReferenceH hSRS = OSRNewSpatialReference(NULL);
	char *pszSRS_WKT = NULL;
	
	if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE)
		Rcpp::stop("Error importing SRS from user input.");
	
	if (pretty) {
		if (OSRExportToPrettyWkt(hSRS, &pszSRS_WKT, false) != OGRERR_NONE) {
			OSRDestroySpatialReference(hSRS);
			Rcpp::stop("Error exporting to pretty WKT.");
		}
	}
	else {
		if (OSRExportToWkt(hSRS, &pszSRS_WKT) != OGRERR_NONE) {
			OSRDestroySpatialReference(hSRS);
			Rcpp::stop("Error exporting to WKT.");
		}
	}
	
	std::string wkt(pszSRS_WKT);
	OSRDestroySpatialReference(hSRS);
	CPLFree(pszSRS_WKT);
	
	return wkt;
}

//' Get the bounding box of a geometry in OGC WKT format.
//'
//' Returns the bounding box of a WKT 2D geometry (e.g., LINE, POLYGON, 
//' MULTIPOLYGON).
//'
//' @param wkt Character. OGC WKT string for a simple feature 2D geometry.
//' @return Numeric vector of length four containing the minX, minY, maxX, maxY
//' of the geometry specified by `wkt`.
//'
//' @examples
//' bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2 
//' 5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5, 
//' 325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
//' bbox_from_wkt(bnd)
// [[Rcpp::export]]
Rcpp::NumericVector bbox_from_wkt(std::string wkt) {

	OGRGeometryH hGeometry;
	char* pszWKT;
	pszWKT = (char*) wkt.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT, NULL, &hGeometry) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from WKT string.");
		
	OGREnvelope sBbox;
	OGR_G_GetEnvelope(hGeometry, &sBbox);
	Rcpp::NumericVector bbox = {
		sBbox.MinX,
		sBbox.MinY,
		sBbox.MaxX,
		sBbox.MaxY
	};

	return bbox;
}
