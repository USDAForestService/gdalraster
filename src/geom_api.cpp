/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include <cpl_port.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <ogr_api.h>
#include <ogr_srs_api.h>

#include <Rcpp.h>

#include <memory>
#include <string>
#include <vector>

#include "geom_api.h"
#include "gdalraster.h"
#include "rcpp_util.h"
#include "srs_api.h"


//' get GEOS version
//' @noRd
// [[Rcpp::export(name = ".getGEOSVersion")]]
std::vector<int> getGEOSVersion() {
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
    OGRGeometryH hGeom = nullptr;
    hGeom = OGR_G_CreateGeometry(wkbPoint);
    if (hGeom == nullptr)
        Rcpp::stop("failed to create geometry object");
    OGR_G_SetPoint_2D(hGeom, 0, 0, 0);

    // If GDAL is built without the GEOS library, this function will
    // always return FALSE:
    bool ret = false;
    ret = OGR_G_IsSimple(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}


// *** geometry factory ***

// internal create OGRGeometryH from WKB raw vector
OGRGeometryH createGeomFromWkb(const Rcpp::RawVector &wkb) {
    if ((wkb.size() == 0))
        Rcpp::stop("'wkb' is empty");

    OGRGeometryH hGeom = nullptr;
    OGRErr err = OGRERR_NONE;
    std::string msg = "unknown error";

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
    err = OGR_G_CreateFromWkb(&wkb[0], nullptr, &hGeom,
                              static_cast<int>(wkb.size()));
#else
    err = OGR_G_CreateFromWkbEx(&wkb[0], nullptr, &hGeom, wkb.size());
#endif
    if (err == OGRERR_NOT_ENOUGH_DATA) {
        msg = "OGRERR_NOT_ENOUGH_DATA, failed to create geometry object";
    }
    else if (err == OGRERR_UNSUPPORTED_GEOMETRY_TYPE) {
        msg = "OGRERR_UNSUPPORTED_GEOMETRY_TYPE";
    }
    else if (err == OGRERR_CORRUPT_DATA) {
        msg = "OGRERR_CORRUPT_DATA, failed to create geometry object";
    }
    else if (err != OGRERR_NONE) {
        msg = "failed to create geometry object";
    }

    if (err != OGRERR_NONE) {
        Rcpp::warning(msg);
    }

    return hGeom;
}

// internal export OGRGeometryH to WKB raw vector
bool exportGeomToWkb(OGRGeometryH hGeom, unsigned char *wkb, bool as_iso,
                     const std::string &byte_order) {

    if (hGeom == nullptr)
        return Rcpp::RawVector::create();

    OGRwkbByteOrder eOrder;
    if (EQUAL(byte_order.c_str(), "LSB")) {
        eOrder = wkbNDR;
    }
    else if (EQUAL(byte_order.c_str(), "MSB")) {
        eOrder = wkbXDR;
    }
    else {
        Rcpp::Rcout << "invalid 'byte_order'\n";
        return false;
    }

    OGRErr err = OGRERR_NONE;
    if (as_iso)
        err = OGR_G_ExportToIsoWkb(hGeom, eOrder, wkb);
    else
        err = OGR_G_ExportToWkb(hGeom, eOrder, wkb);

    if (err != OGRERR_NONE)
        return false;
    else
        return true;
}

// WKB raw vector to WKT string
//
//' @noRd
// [[Rcpp::export(name = ".g_wkb2wkt")]]
Rcpp::String g_wkb2wkt(const Rcpp::RObject &geom, bool as_iso = false) {

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return NA_STRING;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return NA_STRING;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr)
        Rcpp::stop("failed to create geometry object from WKB");

    char *pszWKT_out = nullptr;
    if (as_iso)
        OGR_G_ExportToIsoWkt(hGeom, &pszWKT_out);
    else
        OGR_G_ExportToWkt(hGeom, &pszWKT_out);

    std::string wkt_out = "";
    if (pszWKT_out != nullptr) {
        wkt_out = pszWKT_out;
        CPLFree(pszWKT_out);
    }

    OGR_G_DestroyGeometry(hGeom);

    return wkt_out;
}

// list of WKB raw vectors to character vector of WKT strings
//
//' @noRd
// [[Rcpp::export(name = ".g_wkb_list2wkt")]]
Rcpp::CharacterVector g_wkb_list2wkt(const Rcpp::List &geom,
                                     bool as_iso = false) {

    if (geom.size() == 0)
        Rcpp::stop("'geom' is empty");

    Rcpp::CharacterVector wkt = Rcpp::no_init(geom.size());
    for (R_xlen_t i = 0; i < geom.size(); ++i) {
        if (!Rcpp::is<Rcpp::RawVector>(geom[i])) {
            Rcpp::warning("an input list element is not a raw vector");
            wkt[i] = NA_STRING;
        }
        else {
            const Rcpp::RawVector v = Rcpp::as<Rcpp::RawVector>(geom[i]);
            if (v.size() > 0) {
                wkt[i] = g_wkb2wkt(v, as_iso);
            }
            else {
                Rcpp::warning("an input list element is a length-0 raw vector");
                wkt[i] = NA_STRING;
            }
        }
    }

    return wkt;
}

// WKT string to WKB raw vector
//
//' @noRd
// [[Rcpp::export(name = ".g_wkt2wkb")]]
SEXP g_wkt2wkb(const std::string &geom, bool as_iso = false,
               const std::string &byte_order = "LSB") {

    if (geom.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom = nullptr;
    OGRErr err = OGRERR_NONE;

    char *pszWKT = const_cast<char *>(geom.c_str());
    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr) {
        if (hGeom != nullptr)
            OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to create geometry object from WKT string");
    }

    // special case for POINT EMPTY:
    // gdal/ogr/ogrgeometry.cpp, line 3303 in OGRGeometry::exportToGEOS():
    // #if (GEOS_VERSION_MAJOR == 3 && GEOS_VERSION_MINOR < 12)
    // POINT EMPTY is exported to WKB as if it were POINT(0 0),
    // so that particular case is necessary.
    // (note: this appears to also be true with GEOS 3.12.1, CT 2024-09-14)
    if (OGR_G_GetGeometryType(hGeom) == wkbPoint && OGR_G_IsEmpty(hGeom)) {
        Rcpp::warning(
            "POINT EMPTY is exported to WKB as if it were POINT(0 0)");
    }

    int nWKBSize = OGR_G_WkbSize(hGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to obtain WKB size of geometry object");
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    if (!result)
        Rcpp::stop("failed to export WKB raw vector");

    return wkb;
}

// vector of WKT strings to list of WKB raw vectors
//
//' @noRd
// [[Rcpp::export(name = ".g_wkt_vector2wkb")]]
Rcpp::List g_wkt_vector2wkb(const Rcpp::CharacterVector &geom,
                            bool as_iso = false,
                            const std::string &byte_order = "LSB") {

    if (geom.size() == 0)
        Rcpp::stop("'geom' is empty");

    Rcpp::List wkb = Rcpp::no_init(geom.size());
    for (R_xlen_t i = 0; i < geom.size(); ++i) {
        if (Rcpp::CharacterVector::is_na(geom[i]) || EQUAL(geom[i], "")) {
            Rcpp::warning("an input vector element is NA or empty string");
            wkb[i] = R_NilValue;
        }
        else {
            wkb[i] = g_wkt2wkb(Rcpp::as<std::string>(geom[i]), as_iso,
                               byte_order);
        }
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_create")]]
Rcpp::RawVector g_create(const std::string &geom_type,
                         const Rcpp::RObject &pts, bool as_iso = false,
                         const std::string &byte_order = "LSB") {
// Create a geometry from a list of points (vertices as xy, xyz or xyzm).
// Currently for POINT, MULTIPOINT, LINESTRING, LINEARRING, POLYGON, and
// creating empty GEOMETRYCOLLECTION for subsequent g_add_geom().
// Only simple polygons composed of one ring are supported.
// See also g_add_geom(), e.g., add LINEARRING (interior ring) to POLYGON.

    Rcpp::NumericMatrix pts_in(0, 2);
    if (!pts.isNULL())
        pts_in = xy_robject_to_matrix_(pts);

    if (pts_in.ncol() < 2 || pts_in.ncol() > 4)
        Rcpp::stop("input matrix must have 2, 3 or 4 columns");

    bool has_z = false;
    bool has_m = false;
    if (pts_in.ncol() == 3) {
        has_z = true;
    }
    else if (pts_in.ncol() == 4) {
        has_z = true;
        has_m = true;
    }

    R_xlen_t nPts = pts_in.nrow();

    OGRGeometryH hGeom = nullptr;
    OGRGeometryH hGeom_out = nullptr;
    OGRwkbGeometryType eType = wkbUnknown;

    if (EQUAL(geom_type.c_str(), "POINT")) {
        eType = wkbPoint;
        hGeom = OGR_G_CreateGeometry(wkbPoint);
    }
    else if (EQUAL(geom_type.c_str(), "MULTIPOINT")) {
        eType = wkbMultiPoint;
        hGeom = OGR_G_CreateGeometry(wkbMultiPoint);
    }
    else if (EQUAL(geom_type.c_str(), "LINESTRING")) {
        eType = wkbLineString;
        hGeom = OGR_G_CreateGeometry(wkbLineString);
    }
    else if (EQUAL(geom_type.c_str(), "LINEARRING")) {
        eType = wkbLinearRing;
        hGeom = OGR_G_CreateGeometry(wkbLinearRing);
    }
    else if (EQUAL(geom_type.c_str(), "POLYGON")) {
        eType = wkbPolygon;
        hGeom = OGR_G_CreateGeometry(wkbLinearRing);
    }
    else if (EQUAL(geom_type.c_str(), "GEOMETRYCOLLECTION")) {
        eType = wkbGeometryCollection;
        hGeom = OGR_G_CreateGeometry(wkbGeometryCollection);
    }
    else {
        Rcpp::stop("geometry type not supported");
    }

    if (hGeom == nullptr)
        Rcpp::stop("failed to create geometry object");

    if (eType == wkbGeometryCollection && nPts > 0) {
        Rcpp::Rcout << "g_create() only creates an empty geometry collection, "
            << "ignoring input points\n";
    }

    if (nPts == 1 && eType != wkbGeometryCollection) {
        if (eType != wkbPoint && eType != wkbMultiPoint) {
            OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("invalid number of points for geometry type");
        }
        if (has_m) {
            OGR_G_SetPointZM(hGeom, 0, pts_in(0, 0), pts_in(0, 1),
                             pts_in(0, 2), pts_in(0, 3));
        }
        else if (has_z) {
            OGR_G_SetPoint(hGeom, 0, pts_in(0, 0), pts_in(0, 1),
                           pts_in(0, 2));
        }
        else {
            OGR_G_SetPoint_2D(hGeom, 0, pts_in(0, 0), pts_in(0, 1));
        }
    }
    else if (nPts > 0 && eType != wkbGeometryCollection) {
        if (eType == wkbPoint) {
            OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("point geometry cannot have more than one xy");
        }
        if ((eType == wkbPolygon || eType == wkbLinearRing) && nPts < 4) {
            OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("polygon/linearring must have at least four points");
        }

        if (eType == wkbMultiPoint) {
            for (R_xlen_t i = 0; i < nPts; ++i) {
                OGRGeometryH hPt = OGR_G_CreateGeometry(wkbPoint);
                if (has_m) {
                    OGR_G_SetPointZM(hPt, 0, pts_in(i, 0), pts_in(i, 1),
                                     pts_in(i, 2), pts_in(i, 3));
                }
                else if (has_z) {
                    OGR_G_SetPoint(hPt, 0, pts_in(i, 0), pts_in(i, 1),
                                   pts_in(i, 2));
                }
                else {
                    OGR_G_SetPoint_2D(hPt, 0, pts_in(i, 0), pts_in(i, 1));
                }
                OGRErr err = OGR_G_AddGeometryDirectly(hGeom, hPt);
                if (err != OGRERR_NONE) {
                    if (hGeom != nullptr)
                        OGR_G_DestroyGeometry(hGeom);
                    Rcpp::stop("failed to add POINT to MULTIPOINT");
                }
            }
        }
        else {
            OGR_G_SetPointCount(hGeom, static_cast<int>(nPts));
            for (R_xlen_t i = 0; i < nPts; ++i) {
                if (has_m) {
                    OGR_G_SetPointZM(hGeom, i, pts_in(i, 0), pts_in(i, 1),
                                     pts_in(i, 2), pts_in(i, 3));
                }
                else if (has_z) {
                    OGR_G_SetPoint(hGeom, i, pts_in(i, 0), pts_in(i, 1),
                                   pts_in(i, 2));
                }
                else {
                    OGR_G_SetPoint_2D(hGeom, i, pts_in(i, 0), pts_in(i, 1));
                }
            }
        }
    }

    if (eType != wkbPolygon) {
        hGeom_out = hGeom;
    }
    else {
        hGeom_out = OGR_G_CreateGeometry(wkbPolygon);
        if (nPts == 0) {
            OGR_G_DestroyGeometry(hGeom);
        }
        else {
            const char *save_opt = nullptr;
            save_opt = CPLGetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING",
                                           nullptr);
            if (save_opt == nullptr)
                CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", "NO");

            OGRErr err = OGR_G_AddGeometry(hGeom_out, hGeom);

            if (save_opt == nullptr) {
                CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING",
                                   nullptr);
            }

            OGR_G_DestroyGeometry(hGeom);
            hGeom = nullptr;

            if (err != OGRERR_NONE) {
                if (hGeom_out != nullptr)
                    OGR_G_DestroyGeometry(hGeom_out);

                Rcpp::stop(
                    "failed to create polygon geometry (unclosed ring?)");
            }
        }
    }

    int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        Rcpp::stop("failed to obtain WKB size of output geometry");
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom_out, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom_out);
    if (!result)
        Rcpp::stop("failed to export WKB raw vector for output geometry");

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_add_geom")]]
Rcpp::RawVector g_add_geom(const Rcpp::RawVector &sub_geom,
                           const Rcpp::RawVector &container,
                           bool as_iso = false,
                           const std::string &byte_order = "LSB") {
// Add a geometry to a geometry container.
// POLYGON to POLYGON, POINT to MULTIPOINT, LINESTRING to MULTILINESTRING,
// or POLYGON to MULTIPOLYGON

    OGRGeometryH hSubGeom = nullptr;
    OGRGeometryH hGeom = nullptr;
    OGRErr err = OGRERR_NONE;

    if ((sub_geom.size() == 0))
        Rcpp::stop("'sub_geom' is an empty raw vector");
    if ((container.size() == 0))
        Rcpp::stop("'container' is an empty raw vector");

    hSubGeom = createGeomFromWkb(sub_geom);
    if (hSubGeom == nullptr) {
        Rcpp::stop("failed to create object from 'sub_geom' WKB");
    }

    hGeom = createGeomFromWkb(container);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hSubGeom);
        Rcpp::stop("failed to create object from 'container' WKB");
    }

    const char *save_opt =
        CPLGetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", nullptr);

    if (save_opt == nullptr)
        CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", "NO");

    if (EQUAL(OGR_G_GetGeometryName(hGeom), "POLYGON") &&
        EQUAL(OGR_G_GetGeometryName(hSubGeom), "POLYGON")) {
        // interpret sub_geom as one linearring

        OGRGeometryH hRing = nullptr;
        hRing = OGR_G_GetGeometryRef(hSubGeom, 0);
        err = OGR_G_AddGeometry(hGeom, hRing);
        OGR_G_DestroyGeometry(hSubGeom);
        hSubGeom = nullptr;
        if (err != OGRERR_NONE) {
            if (hGeom != nullptr)
                OGR_G_DestroyGeometry(hGeom);

            Rcpp::stop("failed to add 'sub_geom' to 'container'");
        }
    }
    else {
        err = OGR_G_AddGeometry(hGeom, hSubGeom);
        OGR_G_DestroyGeometry(hSubGeom);
        hSubGeom = nullptr;
        if (err != OGRERR_NONE || hGeom == nullptr) {
            if (hGeom != nullptr)
                OGR_G_DestroyGeometry(hGeom);

            Rcpp::stop("failed to add 'sub_geom' to 'container'");
        }
    }

    if (save_opt == nullptr)
        CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", nullptr);

    int nWKBSize = OGR_G_WkbSize(hGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to obtain WKB size of output geometry");
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    if (!result)
        Rcpp::stop("failed to export WKB raw vector for output geometry");

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_geom_count")]]
int g_geom_count(const Rcpp::RObject &geom, bool quiet = false) {
// Fetch the number of elements in a geometry or number of geometries in
// container. Only geometries of type wkbPolygon[25D], wkbMultiPoint[25D],
// wkbMultiLineString[25D], wkbMultiPolygon[25D] or wkbGeometryCollection[25D]
// may return a valid value. Other geometry types will silently return 0.

    int ret = 0;

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return ret;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return ret;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return NA_INTEGER;
    }

    ret = OGR_G_GetGeometryCount(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_get_geom")]]
SEXP g_get_geom(const Rcpp::RawVector &container, int sub_geom_idx,
                bool as_iso, const std::string &byte_order) {
// Fetch geometry from a geometry container. For a polygon,
// OGR_G_GetGeometryRef(iSubGeom) returns the exterior ring if iSubGeom == 0,
// and the interior rings for iSubGeom > 0.

    if (container.isNULL() || !Rcpp::is<Rcpp::RawVector>(container))
        return R_NilValue;

    const Rcpp::RawVector geom_in(container);
    if (geom_in.size() == 0)
        return geom_in;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        Rcpp::warning(
            "failed to create geometry object from WKB, NULL returned");
        return R_NilValue;
    }

    if (sub_geom_idx < 0)
        Rcpp::stop("'sub_geom_idx' must be >= 0");

    OGRGeometryH hSubGeom = nullptr;
    bool destroy_sub_geom = false;
    hSubGeom = OGR_G_GetGeometryRef(hGeom, sub_geom_idx);
    if (hSubGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::warning("failed to get sub-geometry reference");
        return R_NilValue;
    }

    OGRwkbGeometryType container_type = OGR_G_GetGeometryType(hGeom);
    OGRwkbGeometryType sub_geom_type = OGR_G_GetGeometryType(hSubGeom);
    if (wkbFlatten(container_type) == wkbPolygon &&
            (wkbFlatten(sub_geom_type) == wkbLineString ||
             wkbFlatten(sub_geom_type) == wkbLinearRing)) {

        hSubGeom = OGR_G_ForceTo(OGR_G_Clone(hSubGeom), container_type,
                                 nullptr);

        destroy_sub_geom = true;
    }

    int nWKBSize = OGR_G_WkbSize(hSubGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        if (destroy_sub_geom)
            OGR_G_DestroyGeometry(hSubGeom);

        Rcpp::warning("failed to obtain WKB size of output geometry");
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hSubGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    if (destroy_sub_geom)
        OGR_G_DestroyGeometry(hSubGeom);

    if (!result) {
        Rcpp::warning("failed to export WKB raw vector for output geometry");
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_is_valid")]]
Rcpp::LogicalVector g_is_valid(const Rcpp::RObject &geom,
                               bool quiet = false) {
// Test if the geometry is valid.
// This function is built on the GEOS library, check it for the definition
// of the geometry operation.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = false;
    ret = OGR_G_IsValid(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_make_valid")]]
SEXP g_make_valid(const Rcpp::RObject &geom,
                  const std::string &method = "LINEWORK",
                  bool keep_collapsed = false,
                  bool as_iso = false,
                  const std::string &byte_order = "LSB",
                  bool quiet = false) {

// Attempts to make an invalid geometry valid without losing vertices.
// Already-valid geometries are cloned without further intervention.
// Running OGRGeometryFactory::removeLowerDimensionSubGeoms() as a
// post-processing step is often desired.
// This function is built on the GEOS >= 3.8 library, check it for the
// definition of the geometry operation.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return geom_in;

    int geos_maj_ver = getGEOSVersion()[0];
    int geos_min_ver = getGEOSVersion()[1];
    bool geos_3_10_min = false;

    if (geos_maj_ver > 3 || (geos_maj_ver == 3 && geos_min_ver >= 10)) {
        geos_3_10_min = true;
    }
    else if ((geos_maj_ver == 3 && geos_min_ver < 8) ||
             geos_maj_ver < 3) {

        if (!quiet) {
            Rcpp::warning(
                "GEOS < 3.8 detected: g_make_valid() requires GEOS >= 3.8");
        }
        // will return a clone of the input geometry if it is valid, or
        // NULL if it is invalid
    }

    // begin options
    std::vector<const char *> opt{};

    // method
    if (EQUAL(method.c_str(), "LINEWORK")) {
        opt.push_back("METHOD=LINEWORK");
    }
    else if (EQUAL(method.c_str(), "STRUCTURE")) {
        if (GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 4, 0) ||
            !geos_3_10_min) {

            if (!quiet) {
                Rcpp::warning(
                    "STRUCTURE method requires GEOS >= 3.10 and GDAL >= 3.4");
            }

            opt.push_back("METHOD=LINEWORK");
        }
        else {
            opt.push_back("METHOD=STRUCTURE");
        }
    }
    else {
        if (!quiet) {
            Rcpp::warning(
                "value given for 'method' not recognized, using LINEWORK");
        }

        opt.push_back("METHOD=LINEWORK");
    }

    // keep_collapsed
    if (keep_collapsed)
        opt.push_back("KEEP_COLLAPSED=YES");
    else
        opt.push_back("KEEP_COLLAPSED=NO");

    opt.push_back(nullptr);
    // end options

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hGeomValid = nullptr;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 4, 0)
    if (geos_3_10_min)
        hGeomValid = OGR_G_MakeValidEx(hGeom, opt.data());
    else
        hGeomValid = OGR_G_MakeValid(hGeom);
#else
    hGeomValid = OGR_G_MakeValid(hGeom);
#endif

    if (hGeomValid == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR MakeValid() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hGeomValid);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hGeomValid);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeomValid, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeomValid);
    if (!result) {
        if (!quiet) {
            Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_normalize")]]
SEXP g_normalize(const Rcpp::RObject &geom, bool as_iso,
                 const std::string &byte_order, bool quiet) {
// GEOS doc:
// Organize the elements, rings, and coordinate order of geometries in a
// consistent way, so that geometries that represent the same object can be
// easily compared. Normalization ensures the following:
// * Lines are oriented to have smallest coordinate first (apart from duplicate
//   endpoints)
// * Rings start with their smallest coordinate (using XY ordering)
// * Polygon shell rings are oriented CW, and holes CCW
// * Collection elements are sorted by their first coordinate

// Requires GDAL >= 3.3
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 3, 0)
    Rcpp::stop("g_normalize() requires GDAL >= 3.3");

#else
    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hNormal = OGR_G_Normalize(hGeom);

    if (hNormal == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_UnaryUnion() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hNormal);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hNormal);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hNormal, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hNormal);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
#endif
}

//' @noRd
// [[Rcpp::export(name = ".g_set_3D")]]
SEXP g_set_3D(const Rcpp::RObject &geom, bool is_3d, bool as_iso,
              const std::string &byte_order, bool quiet) {
// Add or remove the Z coordinate dimension.
// This method adds or removes the explicit Z coordinate dimension. Removing
// the Z coordinate dimension of a geometry will remove any existing Z values.
// Adding the Z dimension to a geometry collection, a compound curve, a
// polygon, etc. will affect the children geometries.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return geom_in;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    if (is_3d)
        OGR_G_Set3D(hGeom, TRUE);
    else
        OGR_G_Set3D(hGeom, FALSE);

    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_Set3D() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    if (!result) {
        if (!quiet) {
            Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_set_measured")]]
SEXP g_set_measured(const Rcpp::RObject &geom, bool is_measured, bool as_iso,
                    const std::string &byte_order, bool quiet) {
// Add or remove the M coordinate dimension.
// This method adds or removes the explicit M coordinate dimension. Removing
// the M coordinate dimension of a geometry will remove any existing M values.
// Adding the M dimension to a geometry collection, a compound curve, a
// polygon, etc. will affect the children geometries.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return geom_in;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    if (is_measured)
        OGR_G_SetMeasured(hGeom, TRUE);
    else
        OGR_G_SetMeasured(hGeom, FALSE);

    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_SetMeasured() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    if (!result) {
        if (!quiet) {
            Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_swap_xy")]]
SEXP g_swap_xy(const Rcpp::RObject &geom, bool as_iso = false,
               const std::string &byte_order = "LSB",
               bool quiet = false) {
// Swap x and y coordinates.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return geom_in;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGR_G_SwapXY(hGeom);

    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_SwapXY() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    if (!result) {
        if (!quiet) {
            Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_is_empty")]]
Rcpp::LogicalVector g_is_empty(const Rcpp::RObject &geom,
                               bool quiet = false) {
// Test if the geometry is empty.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = false;
    ret = OGR_G_IsEmpty(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_is_3D")]]
Rcpp::LogicalVector g_is_3D(const Rcpp::RObject &geom,
                               bool quiet = false) {
// See if the geometry has Z coordinates.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = false;
    ret = OGR_G_Is3D(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_is_measured")]]
Rcpp::LogicalVector g_is_measured(const Rcpp::RObject &geom,
                               bool quiet = false) {
// See if the geometry is measured (M values).

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = false;
    ret = OGR_G_IsMeasured(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_is_ring")]]
Rcpp::LogicalVector g_is_ring(const Rcpp::RObject &geom,
                              bool quiet = false) {
// Test if the geometry is a ring.
// returns TRUE if the coordinates of the geometry form a ring, by checking
// length and closure (self-intersection is not checked), otherwise FALSE.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = false;
    ret = OGR_G_IsRing(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_name")]]
Rcpp::String g_name(const Rcpp::RObject &geom, bool quiet = false) {
// extract the geometry type name from a WKB/WKT geometry

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return NA_STRING;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return NA_STRING;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return NA_STRING;
    }

    std::string ret = "";
    ret = OGR_G_GetGeometryName(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_summary")]]
Rcpp::String g_summary(const Rcpp::RObject &geom, bool quiet = false) {
// "dump readable" summary of a WKB/WKT geometry

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return NA_STRING;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return NA_STRING;

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 7, 0)
    Rcpp::stop("`g_summary()` requires GDAL >= 3.7");
#else
    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return NA_STRING;
    }

    const auto poGeom = std::unique_ptr<OGRGeometry>(
        OGRGeometry::FromHandle(hGeom));

    std::vector<const char *> options = {"DISPLAY_GEOMETRY=SUMMARY", nullptr};
    CPLString s = poGeom->dumpReadable(nullptr, options.data());
    s.replaceAll('\n', ' ');
    std::string ret = s.Trim();
    return ret;
#endif
}

//' @noRd
// [[Rcpp::export(name = ".g_envelope")]]
Rcpp::NumericVector g_envelope(const Rcpp::RObject &geom, bool as_3d = false,
                               bool quiet = false) {
// Computes and returns the bounding envelope for this geometry.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL, NA_REAL, NA_REAL);

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL, NA_REAL, NA_REAL);

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL, NA_REAL, NA_REAL);
    }

    Rcpp::NumericVector ret;
    if (as_3d) {
        OGREnvelope3D sEnv3D;
        OGR_G_GetEnvelope3D(hGeom, &sEnv3D);
        double minZ = NA_REAL;
        double maxZ = NA_REAL;
        if (!std::isinf(sEnv3D.MinZ))
            minZ = sEnv3D.MinZ;
        if (!std::isinf(sEnv3D.MaxZ))
            maxZ = sEnv3D.MaxZ;

        ret = {sEnv3D.MinX, sEnv3D.MaxX,
               sEnv3D.MinY, sEnv3D.MaxY,
               minZ, maxZ};
    }
    else {
        OGREnvelope sEnv;
        OGR_G_GetEnvelope(hGeom, &sEnv);
        ret = {sEnv.MinX, sEnv.MaxX, sEnv.MinY, sEnv.MaxY};
    }

    OGR_G_DestroyGeometry(hGeom);
    return ret;
}


// *** binary predicates ***


//' @noRd
// [[Rcpp::export(name = ".g_intersects")]]
Rcpp::LogicalVector g_intersects(const Rcpp::RObject &this_geom,
                                 const Rcpp::RObject &other_geom,
                                 bool quiet = false) {
// Determines whether two geometries intersect. If GEOS is enabled, then this
// is done in rigorous fashion otherwise TRUE is returned if the envelopes
// (bounding boxes) of the two geometries overlap.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = OGR_G_Intersects(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_equals")]]
Rcpp::LogicalVector g_equals(const Rcpp::RObject &this_geom,
                             const Rcpp::RObject &other_geom,
                             bool quiet = false) {
// Returns TRUE if two geometries are equivalent.
// This operation implements the SQL/MM ST_OrderingEquals() operation.
// The comparison is done in a structural way, that is to say that the
// geometry types must be identical, as well as the number and ordering of
// sub-geometries and vertices. Or equivalently, two geometries are
// considered equal by this method if their WKT/WKB representation is equal.
// Note: this must be distinguished from equality in a spatial way.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = OGR_G_Equals(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_disjoint")]]
Rcpp::LogicalVector g_disjoint(const Rcpp::RObject &this_geom,
                               const Rcpp::RObject &other_geom,
                               bool quiet = false) {
// Tests if this geometry and the other geometry are disjoint.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = OGR_G_Disjoint(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_touches")]]
Rcpp::LogicalVector g_touches(const Rcpp::RObject &this_geom,
                              const Rcpp::RObject &other_geom,
                              bool quiet = false) {
// Tests if this geometry and the other geometry are touching.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = OGR_G_Touches(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_contains")]]
Rcpp::LogicalVector g_contains(const Rcpp::RObject &this_geom,
                               const Rcpp::RObject &other_geom,
                               bool quiet = false) {
// Tests if this geometry contains the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = OGR_G_Contains(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_within")]]
Rcpp::LogicalVector g_within(const Rcpp::RObject &this_geom,
                             const Rcpp::RObject &other_geom,
                             bool quiet = false) {
// Tests if this geometry is within the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = OGR_G_Within(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_crosses")]]
Rcpp::LogicalVector g_crosses(const Rcpp::RObject &this_geom,
                              const Rcpp::RObject &other_geom,
                              bool quiet = false) {
// Tests if this geometry and the other geometry are crossing.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = OGR_G_Crosses(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_overlaps")]]
Rcpp::LogicalVector g_overlaps(const Rcpp::RObject &this_geom,
                               const Rcpp::RObject &other_geom,
                               bool quiet = false) {
// Tests if this geometry and the other geometry overlap, that is their
// intersection has a non-zero area (they have some but not all points in
// common).
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return Rcpp::LogicalVector::create(NA_LOGICAL);

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    bool ret = OGR_G_Overlaps(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}


// *** unary operations ***

//' @noRd
// [[Rcpp::export(name = ".g_boundary")]]
SEXP g_boundary(const Rcpp::RObject &geom, bool as_iso,
                const std::string &byte_order, bool quiet) {

// Compute boundary.

// A new geometry object is created and returned containing the boundary of the
// geometry on which the method is invoked.

// This function is built on the GEOS library, check it for the definition of
// the geometry operation:
// Returns the "boundary" of a geometry, as defined by the DE9IM:
//   * the boundary of a polygon is the linear rings dividing the exterior from
//     the interior
//   * the boundary of a linestring is the end points
//   * the boundary of a point is the point
// https://libgeos.org/doxygen/geos__c_8h.html#a2830fb255d1aec3fdee665d9fa6eb07f

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hBoundaryGeom = OGR_G_Boundary(hGeom);

    if (hBoundaryGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_Boundary() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hBoundaryGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hBoundaryGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hBoundaryGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hBoundaryGeom);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_buffer")]]
SEXP g_buffer(const Rcpp::RObject &geom, double dist, int quad_segs = 30,
              bool as_iso = false, const std::string &byte_order = "LSB",
              bool quiet = false) {
// Compute buffer of geometry.

// Builds a new geometry containing the buffer region around the geometry on
// which it is invoked. The buffer is a polygon containing the region within
// the buffer distance of the original geometry.

// Some buffer sections are properly described as curves, but are converted to
// approximate polygons. The nQuadSegs parameter can be used to control how
// many segments should be used to define a 90 degree curve - a quadrant of a
// circle. A value of 30 is a reasonable default. Large values result in large
// numbers of vertices in the resulting buffer geometry while small numbers
// reduce the accuracy of the result.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hBufferGeom = OGR_G_Buffer(hGeom, dist, quad_segs);

    if (hBufferGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_Buffer() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hBufferGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hBufferGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hBufferGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hBufferGeom);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_convex_hull")]]
SEXP g_convex_hull(const Rcpp::RObject &geom, bool as_iso,
                   const std::string &byte_order, bool quiet) {
// Compute convex hull.

// A new geometry object is created and returned containing the convex hull of
// the geometry on which the method is invoked.

// This function is built on the GEOS library, check it for the definition the
// operation:
// The convex hull is the smallest convex Geometry that contains all the points
// in the input Geometry. Uses the Graham Scan algorithm.
// https://libgeos.org/doxygen/classgeos_1_1algorithm_1_1ConvexHull.html

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hConvHullGeom = OGR_G_ConvexHull(hGeom);

    if (hConvHullGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_ConvexHull() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hConvHullGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hConvHullGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hConvHullGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hConvHullGeom);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_concave_hull")]]
SEXP g_concave_hull(const Rcpp::RObject &geom, double ratio, bool allow_holes,
                    bool as_iso, const std::string &byte_order, bool quiet) {
// https://libgeos.org/doxygen/geos__c_8h.html
// Returns a "concave hull" of a geometry. A concave hull is a polygon which
// contains all the points of the input, but is a better approximation than the
// convex hull to the area occupied by the input. Frequently used to convert a
// multi-point into a polygonal area that contains all the points in the input
// geometry.

// A set of points has a sequence of hulls of increasing concaveness, determined
// by a numeric target parameter. The concave hull is constructed by removing
// the longest outer edges of the Delaunay Triangulation of the space between
// the polygons, until the target criterion parameter is reached. This can be
// expressed as a ratio between the lengths of the longest and shortest edges.
// 1 produces the convex hull; 0 produces a hull with maximum concaveness.

// Requires GDAL >= 3.6. This function is built on the GEOS >= 3.11 library. If
// OGR is built without the GEOS >= 3.11 library, this function will always
// fail, issuing a CPLE_NotSupported error.

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 6, 0)
    Rcpp::stop("g_concave_hull() requires GDAL >= 3.6");
#else

    std::vector<int> geos_ver = getGEOSVersion();
    int geos_maj_ver = geos_ver[0];
    int geos_min_ver = geos_ver[1];
    if (!(geos_maj_ver > 3 || (geos_maj_ver == 3 && geos_min_ver >= 11)))
        Rcpp::stop("g_concave_hull() requires GEOS >= 3.11");

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return R_NilValue;

    if (ratio < 0 || ratio > 1)
        Rcpp::stop("'ratio' must be a numeric value >= 0 and <= 1");

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hHullGeom = OGR_G_ConcaveHull(hGeom, ratio, allow_holes);

    if (hHullGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_ConcaveHull() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hHullGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hHullGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hHullGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hHullGeom);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
#endif
}

//' @noRd
// [[Rcpp::export(name = ".g_delaunay_triangulation")]]
SEXP g_delaunay_triangulation(const Rcpp::RObject &geom,
                              bool constrained = false,
                              double tolerance = 0.0,
                              bool only_edges = false,
                              bool as_iso = false,
                              const std::string &byte_order = "LSB",
                              bool quiet = false) {
// Return a Delaunay triangulation of the vertices of the geometry.

    std::vector<int> geos_ver = getGEOSVersion();
    int geos_maj_ver = geos_ver[0];
    int geos_min_ver = geos_ver[1];

    if (constrained) {
        if (GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 12, 0)) {
            Rcpp::stop(
                "constrained Delaunay triangulation requires GDAL >= 3.12");

        }
        if (!(geos_maj_ver > 3 || (geos_maj_ver == 3 && geos_min_ver >= 10))) {
            Rcpp::stop("'constrained = TRUE' requires GEOS >= 3.10");
        }
    }
    else {
        if (!(geos_maj_ver > 3 || (geos_maj_ver == 3 && geos_min_ver >= 4))) {
            Rcpp::stop("g_delaunay_triangulation() requires GEOS >= 3.4");
        }
    }

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hTriangulatedGeom = nullptr;
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)
    if (constrained)
        hTriangulatedGeom = OGR_G_ConstrainedDelaunayTriangulation(hGeom);
#endif
    if (!constrained) {
        int only_edges_in = only_edges ? TRUE : FALSE;
        hTriangulatedGeom = OGR_G_DelaunayTriangulation(hGeom, tolerance,
                                                        only_edges_in);
    }

    if (hTriangulatedGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("the OGR API call returned NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hTriangulatedGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hTriangulatedGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hTriangulatedGeom, &wkb[0], as_iso,
                                  byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hTriangulatedGeom);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_simplify")]]
SEXP g_simplify(const Rcpp::RObject &geom, double tolerance,
                bool preserve_topology = true, bool as_iso = false,
                const std::string &byte_order = "LSB", bool quiet = false) {
// Compute a simplified geometry. By default (preserve_topology = TRUE),
// simplify the geometry while preserving topology.
//
// GEOS definitions:
// (https://libgeos.org/doxygen/classgeos_1_1simplify_1_1TopologyPreservingSimplifier.html)
// Simplifies a geometry, ensuring that the result is a valid geometry having
// the same dimension and number of components as the input. The simplification
// uses a maximum distance difference algorithm similar to the one used in the
// Douglas-Peucker algorithm. In particular, if the input is an areal geometry
// (Polygon or MultiPolygon), the result has the same number of shells and
// holes (rings) as the input, in the same order. The result rings touch at no
// more than the number of touching point in the input (although they may touch
// at fewer points).
// If preserve_topology = FALSE:
// (https://libgeos.org/doxygen/classgeos_1_1simplify_1_1DouglasPeuckerSimplifier.html)
// Simplifies a Geometry using the standard Douglas-Peucker algorithm. Ensures
// that any polygonal geometries returned are valid. Simple lines are not
// guaranteed to remain simple after simplification. Note that in general D-P
// does not preserve topology - e.g. polygons can be split, collapse to lines
// or disappear, holes can be created or disappear, and lines can cross. To
// simplify geometry while preserving topology use TopologyPreservingSimplifier.
// (However, using D-P is significantly faster).
//
// PostGIS notes:
// https://postgis.net/docs/ST_SimplifyPreserveTopology.html
// The simplification tolerance is a distance value, in the units of the input
// SRS. Simplification removes vertices which are within the tolerance distance
// of the simplified linework, as long as topology is preserved.
// This function does not preserve boundaries shared between polygons.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hSimplifiedGeom = nullptr;
    if (preserve_topology)
        hSimplifiedGeom = OGR_G_SimplifyPreserveTopology(hGeom, tolerance);
    else
        hSimplifiedGeom = OGR_G_Simplify(hGeom, tolerance);

    if (hSimplifiedGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR API call gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hSimplifiedGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hSimplifiedGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hSimplifiedGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hSimplifiedGeom);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_unary_union")]]
SEXP g_unary_union(const Rcpp::RObject &geom, bool as_iso,
                   const std::string &byte_order, bool quiet) {
// Returns the union of all components of a single geometry. Usually used to
// convert a collection into the smallest set of polygons that cover the same
// area.
//
// See https://postgis.net/docs/ST_UnaryUnion.html for more details.
//
// Requires GDAL >= 3.7
#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 7, 0)
    Rcpp::stop("g_unary_union() requires GDAL >= 3.7");

#else
    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return R_NilValue;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NULL returned");
        }
        return R_NilValue;
    }

    OGRGeometryH hUnion = OGR_G_UnaryUnion(hGeom);

    if (hUnion == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_UnaryUnion() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hUnion);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hUnion);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hUnion, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hUnion);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
#endif
}


// *** binary operations ***


//' @noRd
// [[Rcpp::export(name = ".g_intersection")]]
SEXP g_intersection(const Rcpp::RObject &this_geom,
                    const Rcpp::RObject &other_geom,
                    bool as_iso = false,
                    const std::string &byte_order = "LSB",
                    bool quiet = false) {
// Generates a new geometry which is the region of intersection of the two
// geometries operated on. The g_intersects() function can be used to test
// if two geometries intersect.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call IsValid() before, otherwise the result might
// be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return R_NilValue;

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return R_NilValue;
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return R_NilValue;
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return R_NilValue;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return R_NilValue;
    }

    OGRGeometryH hGeom_out = nullptr;
    hGeom_out = OGR_G_Intersection(hGeom_this, hGeom_other);
    if (hGeom_out == nullptr) {
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "OGR_G_Intersection() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom_out, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom_out);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_union")]]
SEXP g_union(const Rcpp::RObject &this_geom,
             const Rcpp::RObject &other_geom,
             bool as_iso = false,
             const std::string &byte_order = "LSB",
             bool quiet = false) {
// Generates a new geometry which is the region of union of the two
// geometries operated on.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call IsValid() before, otherwise the result might
// be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return R_NilValue;

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return R_NilValue;
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return R_NilValue;
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return R_NilValue;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return R_NilValue;
    }

    OGRGeometryH hGeom_out = nullptr;
    hGeom_out = OGR_G_Union(hGeom_this, hGeom_other);
    if (hGeom_out == nullptr) {
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "OGR_G_Union() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom_out, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom_out);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_difference")]]
SEXP g_difference(const Rcpp::RObject &this_geom,
                  const Rcpp::RObject &other_geom,
                  bool as_iso = false,
                  const std::string &byte_order = "LSB",
                  bool quiet = false) {
// Generates a new geometry which is the region of this geometry with the
// region of the other geometry removed.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call IsValid() before, otherwise the result might
// be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return R_NilValue;

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return R_NilValue;
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return R_NilValue;
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return R_NilValue;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return R_NilValue;
    }

    OGRGeometryH hGeom_out = nullptr;
    hGeom_out = OGR_G_Difference(hGeom_this, hGeom_other);
    if (hGeom_out == nullptr) {
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "OGR_G_Difference() gave NULL geometry");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom_out, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom_out);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_sym_difference")]]
SEXP g_sym_difference(const Rcpp::RObject &this_geom,
                      const Rcpp::RObject &other_geom,
                      bool as_iso = false,
                      const std::string &byte_order = "LSB",
                      bool quiet = false) {
// Generates a new geometry which is the symmetric difference of this geometry
// and the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call IsValid() before, otherwise the result might
// be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return R_NilValue;

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return R_NilValue;

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return R_NilValue;
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return R_NilValue;
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return R_NilValue;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return R_NilValue;
    }

    OGRGeometryH hGeom_out = nullptr;
    hGeom_out = OGR_G_SymDifference(hGeom_this, hGeom_other);
    if (hGeom_out == nullptr) {
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "OGR_G_SymDifference() gave NULL geometry, NA returned");
        }
        return R_NilValue;
    }

    int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return R_NilValue;
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom_out, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom_out);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                "failed to export WKB raw vector for output geometry");
        }
        return R_NilValue;
    }

    return wkb;
}


// *** measures ***


//' @noRd
// [[Rcpp::export(name = ".g_distance")]]
double g_distance(const Rcpp::RObject &this_geom,
                  const Rcpp::RObject &other_geom,
                  bool quiet = false) {
// Returns the distance between the geometries or -1 if an error occurs.
// Returns the shortest distance between the two geometries. The distance is
// expressed into the same unit as the coordinates of the geometries.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    double ret = -1;

    if (this_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(this_geom))
        return NA_REAL;

    const Rcpp::RawVector this_geom_in(this_geom);
    if (this_geom_in.size() == 0)
        return NA_REAL;

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom_in);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return ret;
    }

    if (other_geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(other_geom)) {
        OGR_G_DestroyGeometry(hGeom_this);
        return NA_REAL;
    }

    const Rcpp::RawVector other_geom_in(other_geom);
    if (other_geom_in.size() == 0) {
        OGR_G_DestroyGeometry(hGeom_this);
        return NA_REAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom_in);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to create geometry object from WKB");
        }
        return ret;
    }

    ret = OGR_G_Distance(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_length")]]
double g_length(const Rcpp::RObject &geom, bool quiet = false) {
// Computes the length for OGRCurve (LineString) or MultiCurve objects.
// Undefined for all other geometry types (returns zero).

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return NA_REAL;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return NA_REAL;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return NA_REAL;
    }

    double ret = 0;
    ret = OGR_G_Length(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_area")]]
double g_area(const Rcpp::RObject &geom, bool quiet = false) {
// Computes the area for an OGRLinearRing, OGRPolygon or OGRMultiPolygon.
// Undefined for all other geometry types (returns zero).

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return NA_REAL;

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return NA_REAL;

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return NA_REAL;
    }

    double ret = 0;
    ret = OGR_G_Area(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_geodesic_area")]]
double g_geodesic_area(const Rcpp::RObject &geom, const std::string &srs,
                       bool traditional_gis_order = true, bool quiet = false) {
// Compute geometry area, considered as a surface on the underlying ellipsoid
// of the SRS attached to the geometry.
// The returned area will always be in square meters, and assumes that polygon
// edges describe geodesic lines on the ellipsoid.
// Requires GDAL >= 3.9

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 9, 0)
    Rcpp::stop("g_geodesic_area() requires GDAL >= 3.9");

#else
    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return NA_REAL;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0) {
        OSRDestroySpatialReference(hSRS);
        return NA_REAL;
    }

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        OSRDestroySpatialReference(hSRS);
        return NA_REAL;
    }

    OSRAxisMappingStrategy strategy = OAMS_TRADITIONAL_GIS_ORDER;
    std::string save_opt =
        get_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER");

    if (!traditional_gis_order) {
        strategy = OAMS_AUTHORITY_COMPLIANT;
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "NO");
    }
    OSRSetAxisMappingStrategy(hSRS, strategy);
    OGR_G_AssignSpatialReference(hGeom, hSRS);

    double ret = -1.0;
    ret = OGR_G_GeodesicArea(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    if (hSRS != nullptr)
        OSRDestroySpatialReference(hSRS);
    if (!traditional_gis_order)
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", save_opt);

    if (ret < 0)
        return NA_REAL;
    else
        return ret;
#endif
}

//' @noRd
// [[Rcpp::export(name = ".g_geodesic_length")]]
double g_geodesic_length(const Rcpp::RObject &geom, const std::string &srs,
                         bool traditional_gis_order = true,
                         bool quiet = false) {
// Get the length of the curve, considered as a geodesic line on the underlying
// ellipsoid of the SRS attached to the geometry.
// The returned length will always be in meters.
// Requires GDAL >= 3.10

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 10, 0)
    Rcpp::stop("g_geodesic_length() requires GDAL >= 3.10");

#else
    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return NA_REAL;

    OGRSpatialReferenceH hSRS = OSRNewSpatialReference(nullptr);
    if (OSRSetFromUserInput(hSRS, srs.c_str()) != OGRERR_NONE) {
        if (hSRS != nullptr)
            OSRDestroySpatialReference(hSRS);
        Rcpp::stop("error importing SRS from user input");
    }

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0) {
        OSRDestroySpatialReference(hSRS);
        return NA_REAL;
    }

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        OSRDestroySpatialReference(hSRS);
        return NA_REAL;
    }

    OSRAxisMappingStrategy strategy = OAMS_TRADITIONAL_GIS_ORDER;
    std::string save_opt =
        get_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER");

    if (!traditional_gis_order) {
        strategy = OAMS_AUTHORITY_COMPLIANT;
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "NO");
    }
    OSRSetAxisMappingStrategy(hSRS, strategy);
    OGR_G_AssignSpatialReference(hGeom, hSRS);

    double ret = -1.0;
    ret = OGR_G_GeodesicLength(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    if (hSRS != nullptr)
        OSRDestroySpatialReference(hSRS);
    if (!traditional_gis_order)
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", save_opt);

    if (ret < 0)
        return NA_REAL;
    else
        return ret;
#endif
}

//' @noRd
// [[Rcpp::export(name = ".g_centroid")]]
Rcpp::NumericVector g_centroid(const Rcpp::RObject &geom,
                               bool quiet = false) {
// Returns a vector of ptX, ptY.
// This method relates to the SFCOM ISurface::get_Centroid() method however
// the current implementation based on GEOS can operate on other geometry
// types such as multipoint, linestring, geometrycollection such as
// multipolygons. OGC SF SQL 1.1 defines the operation for surfaces
// (polygons). SQL/MM-Part 3 defines the operation for surfaces and
// multisurfaces (multipolygons).
// This function is built on the GEOS library, check it for the definition of
// the geometry operation.

    if (geom.isNULL() || !Rcpp::is<Rcpp::RawVector>(geom))
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL);

    const Rcpp::RawVector geom_in(geom);
    if (geom_in.size() == 0)
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL);

    OGRGeometryH hGeom = createGeomFromWkb(geom_in);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL);
    }

    OGRGeometryH hPoint = nullptr;
    hPoint = OGR_G_CreateGeometry(wkbPoint);
    if (hPoint == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning(
                "failed to create point geometry object, NA returned");
        }
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL);
    }
    if (OGR_G_Centroid(hGeom, hPoint) ==  OGRERR_FAILURE) {
        OGR_G_DestroyGeometry(hPoint);
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning(
                "failed to compute centroid for the geometry, NA returned");
        }
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL);
    }

    const double x = OGR_G_GetX(hPoint, 0);
    const double y = OGR_G_GetY(hPoint, 0);
    Rcpp::NumericVector pt = {x, y};
    OGR_G_DestroyGeometry(hPoint);
    OGR_G_DestroyGeometry(hGeom);

    return pt;
}


// *** spatial reference ***


//' @noRd
// [[Rcpp::export(name = ".g_transform")]]
SEXP g_transform(const Rcpp::RObject &geom, const std::string &srs_from,
                 const std::string &srs_to, bool wrap_date_line = false,
                 int date_line_offset = 10, bool traditional_gis_order = true,
                 bool as_iso = false, const std::string &byte_order = "LSB",
                 bool quiet = false) {
// Returns a transformed geometry as WKB
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
// This function uses the OGR_GeomTransformer_Create() and
// OGR_GeomTransformer_Transform() functions: this is a enhanced version of
// OGR_G_Transform(). When reprojecting geometries from a Polar Stereographic
// projection or a projection naturally crossing the antimeridian (like UTM
// Zone 60) to a geographic CRS, it will cut geometries along the antimeridian.
// So a LineString might be returned as a MultiLineString.

    if (geom.isNULL() ||
        (!Rcpp::is<Rcpp::RawVector>(geom) && !Rcpp::is<Rcpp::List>(geom))) {

        return R_NilValue;
    }

    const std::string srs_from_in = srs_to_wkt(srs_from, false);
    const std::string srs_to_in = srs_to_wkt(srs_to, false);

    OGRSpatialReferenceH hSRS_from = OSRNewSpatialReference(nullptr);
    OGRSpatialReferenceH hSRS_to = OSRNewSpatialReference(nullptr);

    char *pszWKT1 = const_cast<char*>(srs_from_in.c_str());
    if (OSRImportFromWkt(hSRS_from, &pszWKT1) != OGRERR_NONE) {
        if (hSRS_from != nullptr)
            OSRDestroySpatialReference(hSRS_from);
        if (hSRS_to != nullptr)
            OSRDestroySpatialReference(hSRS_to);
        Rcpp::stop("error importing 'srs_from' from user input");
    }

    char *pszWKT2 = const_cast<char*>(srs_to_in.c_str());
    if (OSRImportFromWkt(hSRS_to, &pszWKT2) != OGRERR_NONE) {
        if (hSRS_from != nullptr)
            OSRDestroySpatialReference(hSRS_from);
        if (hSRS_to != nullptr)
            OSRDestroySpatialReference(hSRS_to);
        Rcpp::stop("error importing 'srs_to' from user input");
    }

    const std::string save_opt =
            get_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER");

    if (traditional_gis_order) {
        OSRSetAxisMappingStrategy(hSRS_from, OAMS_TRADITIONAL_GIS_ORDER);
        OSRSetAxisMappingStrategy(hSRS_to, OAMS_TRADITIONAL_GIS_ORDER);
    }
    else {
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", "NO");
        OSRSetAxisMappingStrategy(hSRS_from, OAMS_AUTHORITY_COMPLIANT);
        OSRSetAxisMappingStrategy(hSRS_to, OAMS_AUTHORITY_COMPLIANT);
    }

    OGRCoordinateTransformationH hCT = nullptr;
    hCT = OCTNewCoordinateTransformation(hSRS_from, hSRS_to);
    if (hCT == nullptr) {
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", save_opt);
        if (hSRS_from != nullptr)
            OSRDestroySpatialReference(hSRS_from);
        if (hSRS_to != nullptr)
            OSRDestroySpatialReference(hSRS_to);
        Rcpp::stop("failed to create coordinate transformer");
    }

    std::vector<char *> options;
    std::string dl_offset = "DATELINEOFFSET=";
    if (wrap_date_line) {
        options.push_back(const_cast<char *>("WRAPDATELINE=YES"));
        dl_offset += std::to_string(date_line_offset);
        options.push_back(const_cast<char *>(dl_offset.c_str()));
    }
    options.push_back(nullptr);

    OGRGeomTransformerH hGeomTransformer = nullptr;
    hGeomTransformer = OGR_GeomTransformer_Create(hCT, options.data());
    if (hGeomTransformer == nullptr) {
        OCTDestroyCoordinateTransformation(hCT);
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", save_opt);
        if (hSRS_from != nullptr)
            OSRDestroySpatialReference(hSRS_from);
        if (hSRS_to != nullptr)
            OSRDestroySpatialReference(hSRS_to);
        Rcpp::stop("failed to create geometry transformer");
    }

    bool input_is_list = false;
    Rcpp::List list_in;
    if (Rcpp::is<Rcpp::List>(geom)) {
        list_in = Rcpp::as<Rcpp::List>(geom);
        input_is_list = true;
    }
    else {
        list_in = Rcpp::List::create(Rcpp::as<Rcpp::RawVector>(geom));
    }

    const R_xlen_t num_geom = list_in.size();

    Rcpp::List list_out(num_geom);

    for (R_xlen_t i = 0; i < num_geom; ++i) {
        if (list_in[i] == R_NilValue ||
            !Rcpp::is<Rcpp::RawVector>(list_in[i])) {

            list_out[i] = R_NilValue;
            continue;
        }

        const Rcpp::RawVector geom_in = list_in[i];
        if (geom_in.size() == 0) {
            list_out[i] = R_NilValue;
            continue;
        }

        OGRGeometryH hGeom = createGeomFromWkb(geom_in);
        if (hGeom == nullptr) {
            if (!quiet) {
                Rcpp::warning(
                    "failed to create geometry object from WKB, NULL returned");
            }
            list_out[i] = R_NilValue;
            continue;
        }

        OGRGeometryH hGeom2 = nullptr;
        hGeom2 = OGR_GeomTransformer_Transform(hGeomTransformer, hGeom);

        if (hGeom2 == nullptr) {
            if (!quiet) {
                Rcpp::warning("transformation failed, NULL returned");
            }
            list_out[i] = R_NilValue;
            OGR_G_DestroyGeometry(hGeom);
            continue;
        }

        int nWKBSize = OGR_G_WkbSize(hGeom2);
        if (!nWKBSize) {
            OGR_G_DestroyGeometry(hGeom2);
            OGR_G_DestroyGeometry(hGeom);
            if (!quiet) {
                Rcpp::warning("failed to obtain WKB size of output geometry");
            }
            list_out[i] = R_NilValue;
            continue;
        }

        Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
        bool result = exportGeomToWkb(hGeom2, &wkb[0], as_iso, byte_order);
        OGR_G_DestroyGeometry(hGeom2);
        OGR_G_DestroyGeometry(hGeom);
        if (!result) {
            if (!quiet) {
                Rcpp::warning(
                    "failed to export WKB raw vector for output geometry");
            }
            list_out[i] = R_NilValue;
        }
        else {
            list_out[i] = wkb;
        }
    }

    OGR_GeomTransformer_Destroy(hGeomTransformer);
    OCTDestroyCoordinateTransformation(hCT);
    OSRDestroySpatialReference(hSRS_from);
    OSRDestroySpatialReference(hSRS_to);
    if (!traditional_gis_order)
        set_config_option("OGR_CT_FORCE_TRADITIONAL_GIS_ORDER", save_opt);

    if (input_is_list) {
        return list_out;
    }
    else {
        if (Rcpp::is<Rcpp::RawVector>(list_out[0]))
            return Rcpp::as<Rcpp::RawVector>(list_out[0]);
        else
            return R_NilValue;
    }
}

//' Get the bounding box of a geometry specified in OGC WKT format
//'
//' `bbox_from_wkt()` returns the bounding box of a WKT 2D geometry
//' (e.g., LINE, POLYGON, MULTIPOLYGON).
//'
//' @param wkt Character. OGC WKT string for a simple feature 2D geometry.
//' @param extend_x Numeric scalar. Distance to extend the output bounding box
//' in both directions along the x-axis
//' (results in `xmin = bbox[1] - extend_x`, `xmax = bbox[3] + extend_x`).
//' @param extend_y Numeric scalar. Distance to extend the output bounding box
//' in both directions along the y-axis
//' (results in `ymin = bbox[2] - extend_y`, `ymax = bbox[4] + extend_y`).
//' @return Numeric vector of length four containing the xmin, ymin,
//' xmax, ymax of the geometry specified by `wkt` (possibly extended by values
//' in `extend_x`, `extend_y`).
//'
//' @seealso
//' [bbox_to_wkt()]
//'
//' @examples
//' bnd <- "POLYGON ((324467.3 5104814.2, 323909.4 5104365.4, 323794.2
//' 5103455.8, 324970.7 5102885.8, 326420.0 5103595.3, 326389.6 5104747.5,
//' 325298.1 5104929.4, 325298.1 5104929.4, 324467.3 5104814.2))"
//' bbox_from_wkt(bnd, 100, 100)
// [[Rcpp::export]]
Rcpp::NumericVector bbox_from_wkt(const std::string &wkt, double extend_x = 0,
                                  double extend_y = 0) {

    OGRGeometryH hGeometry = nullptr;
    char *pszWKT = nullptr;
    pszWKT = const_cast<char *>(wkt.c_str());

    if (OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeometry) != OGRERR_NONE) {
        if (hGeometry != nullptr)
            OGR_G_DestroyGeometry(hGeometry);
        Rcpp::Rcout << "failed to create geometry object from WKT string\n";
        Rcpp::NumericVector ret(4, NA_REAL);
        return ret;
    }

    OGREnvelope sBbox;
    OGR_G_GetEnvelope(hGeometry, &sBbox);
    Rcpp::NumericVector bbox = {
        sBbox.MinX - extend_x,
        sBbox.MinY - extend_y,
        sBbox.MaxX + extend_x,
        sBbox.MaxY + extend_y
    };

    OGR_G_DestroyGeometry(hGeometry);

    return bbox;
}

//' Convert a bounding box to POLYGON in OGC WKT format
//'
//' `bbox_to_wkt()` returns a WKT POLYGON string for the given bounding box.
//'
//' @param bbox Numeric vector of length four containing xmin, ymin,
//' xmax, ymax.
//' @param extend_x Numeric scalar. Distance in units of `bbox` to extend the
//' rectangle in both directions along the x-axis
//' (results in `xmin = bbox[1] - extend_x`, `xmax = bbox[3] + extend_x`).
//' @param extend_y Numeric scalar. Distance in units of `bbox` to extend the
//' rectangle in both directions along the y-axis
//' (results in `ymin = bbox[2] - extend_y`, `ymax = bbox[4] + extend_y`).
//' @return Character string for an OGC WKT polygon.
//'
//' @seealso
//' [bbox_from_wkt()], [g_buffer()]
//'
//' @examples
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' ds <- new(GDALRaster, elev_file, read_only=TRUE)
//' bbox_to_wkt(ds$bbox())
//' ds$close()
// [[Rcpp::export]]
Rcpp::String bbox_to_wkt(const Rcpp::NumericVector &bbox, double extend_x = 0,
                         double extend_y = 0) {

    if (bbox.size() != 4)
        Rcpp::stop("invalid bounding box");

    Rcpp::NumericVector bbox_in = Rcpp::clone(bbox);
    bbox_in[0] -= extend_x;
    bbox_in[1] -= extend_y;
    bbox_in[2] += extend_x;
    bbox_in[3] += extend_y;

    Rcpp::NumericMatrix poly_xy(5, 2);
    poly_xy.row(0) = Rcpp::NumericVector::create(bbox_in(0), bbox_in(1));
    poly_xy.row(1) = Rcpp::NumericVector::create(bbox_in(2), bbox_in(1));
    poly_xy.row(2) = Rcpp::NumericVector::create(bbox_in(2), bbox_in(3));
    poly_xy.row(3) = Rcpp::NumericVector::create(bbox_in(0), bbox_in(3));
    poly_xy.row(4) = Rcpp::NumericVector::create(bbox_in(0), bbox_in(1));

    return g_wkb2wkt(g_create("POLYGON", poly_xy));
}

// helper function for geom type conversions
OGRwkbGeometryType getTargetGeomType(OGRwkbGeometryType geom_type,
                                     bool convert_to_linear,
                                     bool promote_to_multi) {

    OGRwkbGeometryType out_type = geom_type;

    if (convert_to_linear) {
        out_type = OGR_GT_GetLinear(out_type);
    }

    if (promote_to_multi) {
        if (out_type == wkbTriangle || out_type == wkbTIN ||
            out_type == wkbPolyhedralSurface) {

            out_type = wkbMultiPolygon;
        }
        else if (!OGR_GT_IsSubClassOf(out_type, wkbGeometryCollection)) {
            out_type = OGR_GT_GetCollection(out_type);
        }
    }

    return out_type;
}
