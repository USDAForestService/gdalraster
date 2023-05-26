/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.
   Chris Toney <chris.toney at usda.gov> */

#include "cpl_conv.h"
#include "ogrsf_frmts.h"

#include "geos_wkt.h"

//' Is GEOS available?
//'
//' `has_geos()` returns a logical value indicating whether GDAL was built
//' against the GEOS library.
//'
//' @return Logical. `TRUE` if GEOS is available, otherwise `FALSE`.
//'
//' @examples
//' has_geos()
// [[Rcpp::export]]
bool has_geos() {
// Test if GDAL built against GEOS
	OGRGeometryH pt = OGR_G_CreateGeometry(wkbPoint);
	OGR_G_SetPoint_2D(pt, 0, 0, 0);
	
	// If GDAL is built without the GEOS library, this function will 
	//always return FALSE:
	return OGR_G_IsSimple(pt);
}


// *** geometry factory ***


//' @noRd
// [[Rcpp::export(name = ".g_create")]]
std::string _g_create(Rcpp::NumericMatrix xy, std::string geom_type) {
// Create a geometry from a list of points (vertices).
// Currently only for POINT, LINESTRING, POLYGON.
// Only simple polygons composed of one exterior ring are supported.

	OGRGeometryH hGeom;
	
	if (geom_type == "point" || geom_type == "POINT") {
		geom_type = "POINT";
		hGeom = OGR_G_CreateGeometry(wkbPoint);
	}
	else if (geom_type == "linestring" || geom_type == "LINESTRING") {
		geom_type = "LINESTRING";
		hGeom = OGR_G_CreateGeometry(wkbLineString);
	}
	else if (geom_type == "polygon" || geom_type == "POLYGON") {
		geom_type = "POLYGON";
		hGeom = OGR_G_CreateGeometry(wkbLineString);
	}
	else {
		Rcpp::stop("Geometry type is not valid.");
	}
	
	R_xlen_t nPts = xy.nrow();
	
	if (nPts == 1) {
		if (geom_type != "POINT")
			Rcpp::stop("Invalid number of points for geometry type.");
			
		OGR_G_SetPoint_2D(hGeom, 0, xy(0, 0), xy(0, 1));
	}
	else {
		if (geom_type == "POINT")
			Rcpp::stop("Point geometry cannot have more than one xy.");
		if (geom_type == "POLYGON" && nPts < 4)
			Rcpp::stop("Polygon geometry must have at least four points.");
			
		OGR_G_SetPointCount(hGeom, (int) nPts);
		for (R_xlen_t i=0; i < nPts; ++i)
			OGR_G_SetPoint_2D(hGeom, i, xy(i, 0), xy(i, 1));
	}
	
	if (geom_type == "POLYGON") {
		OGRGeometryH hPoly = OGR_G_ForceToPolygon(hGeom);
		if (!OGR_G_IsValid(hPoly))
			Rcpp::stop("The resulting geometry is not valid.");
		char* pszWKT;
		OGR_G_ExportToWkt(hPoly, &pszWKT);
		std::string wkt(pszWKT);
		CPLFree(pszWKT);
		return wkt;
	}
	else {
		if (!OGR_G_IsValid(hGeom))
			Rcpp::stop("The resulting geometry is not valid.");
		char* pszWKT;
		OGR_G_ExportToWkt(hGeom, &pszWKT);
		std::string wkt(pszWKT);
		CPLFree(pszWKT);
		return wkt;
	}
}

//' @noRd
// [[Rcpp::export(name = ".g_is_valid")]]
bool _g_is_valid(std::string geom) {
// Test if the geometry is valid.
// This function is built on the GEOS library, check it for the definition
// of the geometry operation. If OGR is built without the GEOS library, 
// this function will always return FALSE.

	OGRGeometryH hGeom;
	char* pszWKT = (char*) geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT, NULL, &hGeom) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from WKT string.");
		
	return OGR_G_IsValid(hGeom);
}


// *** binary predicates ***


//' @noRd
// [[Rcpp::export(name = ".g_intersects")]]
bool _g_intersects(std::string this_geom, std::string other_geom) {
// Determines whether two geometries intersect. If GEOS is enabled, then this 
// is done in rigorous fashion otherwise TRUE is returned if the envelopes 
// (bounding boxes) of the two geometries overlap.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Intersects(hGeom_this, hGeom_other);
}

//' @noRd
// [[Rcpp::export(name = ".g_equals")]]
bool _g_equals(std::string this_geom, std::string other_geom) {
// Returns TRUE if two geometries are equivalent.
// This operation implements the SQL/MM ST_OrderingEquals() operation.
// The comparison is done in a structural way, that is to say that the 
// geometry types must be identical, as well as the number and ordering of 
// sub-geometries and vertices. Or equivalently, two geometries are 
// considered equal by this method if their WKT/WKB representation is equal. 
// Note: this must be distinguished from equality in a spatial way.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Equals(hGeom_this, hGeom_other);
}

//' @noRd
// [[Rcpp::export(name = ".g_disjoint")]]
bool _g_disjoint(std::string this_geom, std::string other_geom) {
// Tests if this geometry and the other geometry are disjoint.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call _g_is_valid() before, otherwise the result 
// might be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Disjoint(hGeom_this, hGeom_other);
}

//' @noRd
// [[Rcpp::export(name = ".g_touches")]]
bool _g_touches(std::string this_geom, std::string other_geom) {
// Tests if this geometry and the other geometry are touching.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call _g_is_valid() before, otherwise the result 
// might be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Touches(hGeom_this, hGeom_other);
}

//' @noRd
// [[Rcpp::export(name = ".g_contains")]]
bool _g_contains(std::string this_geom, std::string other_geom) {
// Tests if this geometry contains the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call _g_is_valid() before, otherwise the result 
// might be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Contains(hGeom_this, hGeom_other);
}

//' @noRd
// [[Rcpp::export(name = ".g_within")]]
bool _g_within(std::string this_geom, std::string other_geom) {
// Tests if this geometry is within the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call _g_is_valid() before, otherwise the result 
// might be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Within(hGeom_this, hGeom_other);
}

//' @noRd
// [[Rcpp::export(name = ".g_crosses")]]
bool _g_crosses(std::string this_geom, std::string other_geom) {
// Tests if this geometry and the other geometry are crossing.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call _g_is_valid() before, otherwise the result 
// might be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Crosses(hGeom_this, hGeom_other);
}

//' @noRd
// [[Rcpp::export(name = ".g_overlaps")]]
bool _g_overlaps(std::string this_geom, std::string other_geom) {
// Tests if this geometry and the other geometry overlap, that is their 
// intersection has a non-zero area.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call _g_is_valid() before, otherwise the result 
// might be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

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


//' @noRd
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


// *** binary operations ***


//' @noRd
// [[Rcpp::export(name = ".g_intersection")]]
std::string _g_intersection(std::string this_geom, std::string other_geom) {
// Generates a new geometry which is the region of intersection of the two 
// geometries operated on. The _g_intersects() function can be used to test 
// if two geometries intersect.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call IsValid() before, otherwise the result might 
// be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	OGRGeometryH hGeom = OGR_G_Intersection(hGeom_this, hGeom_other);
	if (hGeom == NULL)
		return "";
	
	char* pszWKT_out;
	OGR_G_ExportToWkt(hGeom, &pszWKT_out);
	std::string wkt_out(pszWKT_out);
	CPLFree(pszWKT_out);
	
	return wkt_out;
}

//' @noRd
// [[Rcpp::export(name = ".g_union")]]
std::string _g_union(std::string this_geom, std::string other_geom) {
// Generates a new geometry which is the region of union of the two 
// geometries operated on.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call IsValid() before, otherwise the result might 
// be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	OGRGeometryH hGeom = OGR_G_Union(hGeom_this, hGeom_other);
	if (hGeom == NULL)
		return "";
	
	char* pszWKT_out;
	OGR_G_ExportToWkt(hGeom, &pszWKT_out);
	std::string wkt_out(pszWKT_out);
	CPLFree(pszWKT_out);
	
	return wkt_out;
}

//' @noRd
// [[Rcpp::export(name = ".g_difference")]]
std::string _g_difference(std::string this_geom, std::string other_geom) {
// Generates a new geometry which is the region of this geometry with the 
// region of the other geometry removed.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call IsValid() before, otherwise the result might 
// be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	OGRGeometryH hGeom = OGR_G_Difference(hGeom_this, hGeom_other);
	if (hGeom == NULL)
		return "";
	
	char* pszWKT_out;
	OGR_G_ExportToWkt(hGeom, &pszWKT_out);
	std::string wkt_out(pszWKT_out);
	CPLFree(pszWKT_out);
	
	return wkt_out;
}

//' @noRd
// [[Rcpp::export(name = ".g_sym_difference")]]
std::string _g_sym_difference(std::string this_geom, std::string other_geom) {
// Generates a new geometry which is the symmetric difference of this geometry
// and the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity 
// of the input geometries, call IsValid() before, otherwise the result might 
// be wrong.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	OGRGeometryH hGeom = OGR_G_SymDifference(hGeom_this, hGeom_other);
	if (hGeom == NULL)
		return "";
	
	char* pszWKT_out;
	OGR_G_ExportToWkt(hGeom, &pszWKT_out);
	std::string wkt_out(pszWKT_out);
	CPLFree(pszWKT_out);
	
	return wkt_out;
}


// *** measures ***


//' @noRd
// [[Rcpp::export(name = ".g_distance")]]
double _g_distance(std::string this_geom, std::string other_geom) {
// Returns the distance between the geometries or -1 if an error occurs.
// Returns the shortest distance between the two geometries. The distance is 
// expressed into the same unit as the coordinates of the geometries.
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom_this, hGeom_other;
	char* pszWKT_this = (char*) this_geom.c_str();
	char* pszWKT_other = (char*) other_geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT_this, NULL, &hGeom_this) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from first WKT string.");
		
	if (OGR_G_CreateFromWkt(&pszWKT_other, NULL, &hGeom_other) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from second WKT string.");
		
	return OGR_G_Distance(hGeom_this, hGeom_other);
}

//' @noRd
// [[Rcpp::export(name = ".g_length")]]
double _g_length(std::string geom) {
// Computes the length for OGRCurve (LineString) or MultiCurve objects. 
// Undefined for all other geometry types (returns zero).

	OGRGeometryH hGeom;
	char* pszWKT = (char*) geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT, NULL, &hGeom) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from WKT string.");
		
	return OGR_G_Length(hGeom);
}

//' @noRd
// [[Rcpp::export(name = ".g_area")]]
double _g_area(std::string geom) {
// Computes the area for an OGRLinearRing, OGRPolygon or OGRMultiPolygon.
// Undefined for all other geometry types (returns zero).

	OGRGeometryH hGeom;
	char* pszWKT = (char*) geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT, NULL, &hGeom) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from WKT string.");
		
	return OGR_G_Area(hGeom);
}

//' @noRd
// [[Rcpp::export(name = ".g_centroid")]]
Rcpp::NumericVector _g_centroid(std::string geom) {
// Returns a vector of ptX, ptY.
// This method relates to the SFCOM ISurface::get_Centroid() method however 
// the current implementation based on GEOS can operate on other geometry 
// types such as multipoint, linestring, geometrycollection such as 
// multipolygons. OGC SF SQL 1.1 defines the operation for surfaces 
// (polygons). SQL/MM-Part 3 defines the operation for surfaces and 
// multisurfaces (multipolygons).
// This function is built on the GEOS library, check it for the definition of 
// the geometry operation. If OGR is built without the GEOS library, this 
// function will always fail, issuing a CPLE_NotSupported error.

	OGRGeometryH hGeom;
	char* pszWKT = (char*) geom.c_str();
	
	if (OGR_G_CreateFromWkt(&pszWKT, NULL, &hGeom) != OGRERR_NONE)
		Rcpp::stop("Failed to create geometry object from WKT string.");
		
	OGRGeometryH hPoint = OGR_G_CreateGeometry(wkbPoint);
	if (OGR_G_Centroid(hGeom, hPoint) ==  OGRERR_FAILURE)
		Rcpp::stop("Failed to compute centroid for the geometry.");

	double x = OGR_G_GetX(hPoint, 0);
	double y = OGR_G_GetY(hPoint, 0);
	Rcpp::NumericVector pt = {x, y};
	return pt;
}

