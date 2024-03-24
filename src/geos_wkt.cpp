/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.
   Chris Toney <chris.toney at usda.gov> */

#include "cpl_conv.h"
#include "ogr_api.h"
#include "ogr_srs_api.h"
#include "ogr_spatialref.h"

#include "geos_wkt.h"

//' get GEOS version
//' @noRd
// [[Rcpp::export(name = ".getGEOSVersion")]]
std::vector<int> _getGEOSVersion() {
    int major, minor, patch;
    major = minor = patch = NA_INTEGER;
#if GDAL_VERSION_NUM >= 3040000
    if (!OGRGetGEOSVersion(&major, &minor, &patch))
        Rcpp::warning("GDAL not built against GEOS");
#endif
    std::vector<int> ret = {major, minor, patch};
    return ret;
}

//' Is GEOS available?
//'
//' `has_geos()` returns a logical value indicating whether GDAL was built
//' against the GEOS library. GDAL built with GEOS is a system requirement
//' as of `gdalraster` 1.10.0, so this function will always return `TRUE`
//' (may be removed in a future version).
//'
//' @return Logical. `TRUE` if GEOS is available, otherwise `FALSE`.
//'
//' @examples
//' has_geos()
// [[Rcpp::export]]
bool has_geos() {
// test if GDAL built against GEOS
// this is now obsolete:
// GDAL built against GEOS is required at gdalraster 1.10
    OGRGeometryH hGeom = OGR_G_CreateGeometry(wkbPoint);
    if (hGeom == nullptr)
        Rcpp::stop("failed to create geometry object");
    OGR_G_SetPoint_2D(hGeom, 0, 0, 0);

    // If GDAL is built without the GEOS library, this function will
    // always return FALSE:
    bool ret = OGR_G_IsSimple(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}


// *** geometry factory ***


//' @noRd
// [[Rcpp::export(name = ".g_create")]]
std::string _g_create(Rcpp::NumericMatrix xy, std::string geom_type) {
// Create a geometry from a list of points (vertices).
// Currently only for POINT, LINESTRING, POLYGON.
// Only simple polygons composed of one exterior ring are supported.

    OGRGeometryH hGeom = nullptr;

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
        hGeom = OGR_G_CreateGeometry(wkbLinearRing);
    }
    else {
        Rcpp::stop("geometry type is not valid");
    }

    if (hGeom == nullptr)
        Rcpp::stop("failed to create geometry object");

    R_xlen_t nPts = xy.nrow();

    if (nPts == 1) {
        if (geom_type != "POINT") {
            OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("invalid number of points for geometry type");
        }

        OGR_G_SetPoint_2D(hGeom, 0, xy(0, 0), xy(0, 1));
    }
    else {
        if (geom_type == "POINT") {
            OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("point geometry cannot have more than one xy");
        }
        if (geom_type == "POLYGON" && nPts < 4) {
            OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("polygon geometry must have at least four points");
        }

        OGR_G_SetPointCount(hGeom, (int) nPts);
        for (R_xlen_t i=0; i < nPts; ++i)
            OGR_G_SetPoint_2D(hGeom, i, xy(i, 0), xy(i, 1));
    }

    if (geom_type == "POLYGON") {
        OGRGeometryH hPoly = OGR_G_CreateGeometry(wkbPolygon);

        CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", "NO");
        if (OGR_G_AddGeometryDirectly(hPoly, hGeom) != OGRERR_NONE)
            Rcpp::stop("failed to create polygon geometry (unclosed ring?)");
        CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", nullptr);

        char* pszWKT;
        OGR_G_ExportToWkt(hPoly, &pszWKT);
        std::string wkt(pszWKT);
        CPLFree(pszWKT);
        OGR_G_DestroyGeometry(hPoly);
        return wkt;
    }
    else {
        char* pszWKT;
        OGR_G_ExportToWkt(hGeom, &pszWKT);
        std::string wkt(pszWKT);
        CPLFree(pszWKT);
        OGR_G_DestroyGeometry(hGeom);
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

    OGRGeometryH hGeom = nullptr;
    OGRErr err;
    char* pszWKT = (char*) geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr)
        Rcpp::stop("failed to create geometry object from WKT string");

    bool ret = OGR_G_IsValid(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}


// *** binary predicates ***


//' @noRd
// [[Rcpp::export(name = ".g_intersects")]]
bool _g_intersects(std::string this_geom, std::string other_geom) {
// Determines whether two geometries intersect. If GEOS is enabled, then this
// is done in rigorous fashion otherwise TRUE is returned if the envelopes
// (bounding boxes) of the two geometries overlap.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Intersects(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Equals(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Disjoint(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Touches(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Contains(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Within(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Crosses(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Overlaps(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
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

    OGRGeometryH hGeom = nullptr;
    OGRErr err;
    char* pszWKT = (char*) geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr)
        Rcpp::stop("failed to create geometry object from WKT string");

    OGRGeometryH hBufferGeom = OGR_G_Buffer(hGeom, dist, quad_segs);
    if (hBufferGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to create buffer geometry");
    }

    char* pszWKT_out;
    OGR_G_ExportToWkt(hBufferGeom, &pszWKT_out);
    std::string wkt_out(pszWKT_out);
    CPLFree(pszWKT_out);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hBufferGeom);

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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    OGRGeometryH hGeom = OGR_G_Intersection(hGeom_this, hGeom_other);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        OGR_G_DestroyGeometry(hGeom_other);
        return "";
    }

    char* pszWKT_out;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out(pszWKT_out);
    CPLFree(pszWKT_out);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);

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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    OGRGeometryH hGeom = OGR_G_Union(hGeom_this, hGeom_other);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        OGR_G_DestroyGeometry(hGeom_other);
        return "";
    }

    char* pszWKT_out;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out(pszWKT_out);
    CPLFree(pszWKT_out);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);

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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    OGRGeometryH hGeom = OGR_G_Difference(hGeom_this, hGeom_other);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        OGR_G_DestroyGeometry(hGeom_other);
        return "";
    }

    char* pszWKT_out;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out(pszWKT_out);
    CPLFree(pszWKT_out);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);

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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    OGRGeometryH hGeom = OGR_G_SymDifference(hGeom_this, hGeom_other);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        OGR_G_DestroyGeometry(hGeom_other);
        return "";
    }

    char* pszWKT_out;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out(pszWKT_out);
    CPLFree(pszWKT_out);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);

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

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err;
    char* pszWKT_this = (char*) this_geom.c_str();
    char* pszWKT_other = (char*) other_geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr)
        Rcpp::stop("failed to create geometry object from first WKT string");

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    double ret = OGR_G_Distance(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_length")]]
double _g_length(std::string geom) {
// Computes the length for OGRCurve (LineString) or MultiCurve objects.
// Undefined for all other geometry types (returns zero).

    OGRGeometryH hGeom = nullptr;
    OGRErr err;
    char* pszWKT = (char*) geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr)
        Rcpp::stop("failed to create geometry object from WKT string");

    double ret = OGR_G_Length(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_area")]]
double _g_area(std::string geom) {
// Computes the area for an OGRLinearRing, OGRPolygon or OGRMultiPolygon.
// Undefined for all other geometry types (returns zero).

    OGRGeometryH hGeom = nullptr;
    OGRErr err;
    char* pszWKT = (char*) geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr)
        Rcpp::stop("failed to create geometry object from WKT string");

    double ret = OGR_G_Area(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
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

    OGRGeometryH hGeom = nullptr;
    OGRErr err;
    char* pszWKT = (char*) geom.c_str();

    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr)
        Rcpp::stop("failed to create geometry object from WKT string");

    OGRGeometryH hPoint = OGR_G_CreateGeometry(wkbPoint);
    if (hPoint == nullptr) {
        Rcpp::stop("failed to create point geometry object");
        OGR_G_DestroyGeometry(hGeom);
    }
    if (OGR_G_Centroid(hGeom, hPoint) ==  OGRERR_FAILURE) {
        OGR_G_DestroyGeometry(hPoint);
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to compute centroid for the geometry");
    }

    double x = OGR_G_GetX(hPoint, 0);
    double y = OGR_G_GetY(hPoint, 0);
    Rcpp::NumericVector pt = {x, y};
    OGR_G_DestroyGeometry(hPoint);
    OGR_G_DestroyGeometry(hGeom);

    return pt;
}


// *** spatial reference ***


//' @noRd
// [[Rcpp::export(name = ".g_transform")]]
std::string _g_transform(std::string geom, std::string srs_from,
        std::string srs_to) {
// Returns a transformed geometry as WKT
// Apply arbitrary coordinate transformation to geometry.
// This function will transform the coordinates of a geometry from their
// current spatial reference system to a new target spatial reference system.
// Normally this means reprojecting the vectors, but it could include datum
// shifts, and changes of units.
// Note that this function does not require that the geometry already have a
// spatial reference system. It will be assumed that they can be treated as
// having the source spatial reference system of the
// OGRCoordinateTransformation object, and the actual SRS of the geometry will
// be ignored. On successful completion the output OGRSpatialReference of the
// OGRCoordinateTransformation will be assigned to the geometry.
// This function only does reprojection on a point-by-point basis. It does not
// include advanced logic to deal with discontinuities at poles or antimeridian.
// For that, use the OGR_GeomTransformer_Create() and
// OGR_GeomTransformer_Transform() functions.

    OGRSpatialReference oSourceSRS, oDestSRS;
    OGRCoordinateTransformation *poCT;
    OGRErr err;

    err = oSourceSRS.importFromWkt(srs_from.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import source SRS from WKT string");

    err = oDestSRS.importFromWkt(srs_to.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import destination SRS from WKT string");

    poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oDestSRS);
    if (poCT == nullptr)
        Rcpp::stop("failed to create coordinate transformer");

    OGRGeometryH hGeom = nullptr;
    char* pszWKT = (char*) geom.c_str();
    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr) {
        OGRCoordinateTransformation::DestroyCT(poCT);
        Rcpp::stop("failed to create geometry object from WKT string");
    }

    err = OGR_G_Transform(hGeom, OGRCoordinateTransformation::ToHandle(poCT));
    if (err != OGRERR_NONE) {
        OGRCoordinateTransformation::DestroyCT(poCT);
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("transformation failed");
    }

    char* pszWKT_out;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out(pszWKT_out);
    CPLFree(pszWKT_out);

    OGRCoordinateTransformation::DestroyCT(poCT);
    OGR_G_DestroyGeometry(hGeom);

    return wkt_out;
}

