/* GEOS convenience functions opertaing on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.
   Chris Toney <chris.toney at usda.gov> */
   
#include <Rcpp.h> 
// [[Rcpp::plugins(cpp11)]]

#include <string>

#include "cpl_conv.h"
#include "ogrsf_frmts.h"

// [[Rcpp::export(name = ".has_geos")]]
bool _has_geos() {
	OGRGeometryH pt = OGR_G_CreateGeometry(wkbPoint);
	OGR_G_SetPoint_2D(pt, 0, 0, 0);
	return OGR_G_IsSimple(pt);
}


// *** binary predicates ***

// [[Rcpp::export(name = ".g_intersects")]]
bool _g_intersects(std::string this_geom, std::string other_geom) {

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Intersects(hGeom_this, hGeom_other);
}

// [[Rcpp::export(name = ".g_equals")]]
bool _g_equals(std::string this_geom, std::string other_geom) {

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Equals(hGeom_this, hGeom_other);
}

// [[Rcpp::export(name = ".g_disjoint")]]
bool _g_disjoint(std::string this_geom, std::string other_geom) {

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Disjoint(hGeom_this, hGeom_other);
}

// [[Rcpp::export(name = ".g_contains")]]
bool _g_contains(std::string this_geom, std::string other_geom) {

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Contains(hGeom_this, hGeom_other);
}

// [[Rcpp::export(name = ".g_within")]]
bool _g_within(std::string this_geom, std::string other_geom) {

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Within(hGeom_this, hGeom_other);
}

// [[Rcpp::export(name = ".g_crosses")]]
bool _g_crosses(std::string this_geom, std::string other_geom) {

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Crosses(hGeom_this, hGeom_other);
}

// [[Rcpp::export(name = ".g_overlaps")]]
bool _g_overlaps(std::string this_geom, std::string other_geom) {

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Overlaps(hGeom_this, hGeom_other);
}


// *** unary operations ***

// [[Rcpp::export(name = ".g_buffer")]]
std::string _g_buffer(std::string geom, double dist, int quad_segs = 30) {
//Compute buffer of geometry.

// Builds a new geometry containing the buffer region around the geometry on 
// which it is invoked. The buffer is a polygon containing the region within 
// the buffer distance of the original geometry.

// Some buffer sections are properly described as curves, but are converted to 
// approximate polygons. The nQuadSegs parameter can be used to control how 
// many segments should be used to define a 90 degree curve - a quadrant of a 
// circle. A value of 30 is a reasonable default. Large values result in large 
// numbers of vertices in the resulting buffer geometry while small numbers 
// reduce the accuracy of the result.

	OGRGeometryH hGeom;
	char* pszWKT = (char*) geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT, NULL, &hGeom) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from WKT string.");

	OGRGeometryH hBuffer = OGR_G_Buffer(hGeom, dist, quad_segs);
	if (hBuffer == NULL)
		Rcpp::stop("Failed to create buffer geometry.");
	
	char* pszWKT_out;
	OGR_G_ExportToWkt(hBuffer, &pszWKT_out);
	std::string wkt_out(pszWKT_out);
	CPLFree(pszWKT_out);
	
	return wkt_out;
}
