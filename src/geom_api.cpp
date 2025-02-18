/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include "rcpp_util.h"
#include "geom_api.h"

#include "cpl_port.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_api.h"
#include "ogr_spatialref.h"
#include "ogr_srs_api.h"

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

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3,3,0)
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
        Rcpp::Rcerr << "invalid 'byte_order'" << std::endl;
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
std::string g_wkb2wkt(const Rcpp::RawVector &geom, bool as_iso = false) {

    if (geom.size() == 0)
        return "";

    OGRGeometryH hGeom = createGeomFromWkb(geom);
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
            Rcpp::RawVector v = Rcpp::as<Rcpp::RawVector>(geom[i]);
            if (v.size() > 0) {
                wkt[i] = g_wkb2wkt(v, as_iso);
            }
            else {
                Rcpp::warning("an input list element is a length-0 raw vector");
                wkt[i] = "";
            }
        }
    }

    return wkt;
}

// WKT string to WKB raw vector
//
//' @noRd
// [[Rcpp::export(name = ".g_wkt2wkb")]]
Rcpp::RawVector g_wkt2wkb(const std::string &geom,
                          bool as_iso = false,
                          const std::string &byte_order = "LSB") {

    if (geom.size() == 0)
        Rcpp::stop("'geom' is empty");

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
    if (OGR_G_GetGeometryType(hGeom) == wkbPoint && OGR_G_IsEmpty(hGeom))
        Rcpp::warning("POINT EMPTY is exported to WKB as if it were POINT(0 0)");

    const int nWKBSize = OGR_G_WkbSize(hGeom);
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
            wkb[i] = NA_LOGICAL;
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
Rcpp::RawVector g_create(std::string geom_type, const Rcpp::RObject &pts,
                         bool as_iso = false,
                         const std::string &byte_order = "LSB") {
// Create a geometry from a list of points (vertices as xy, xyz or xyzm).
// Currently for POINT, MULTIPOINT, LINESTRING, LINEARRING, POLYGON
// Only simple polygons composed of one ring are supported
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

    if (EQUAL(geom_type.c_str(), "POINT")) {
        geom_type = "POINT";
        hGeom = OGR_G_CreateGeometry(wkbPoint);
    }
    else if (EQUAL(geom_type.c_str(), "MULTIPOINT")) {
        geom_type = "MULTIPOINT";
        hGeom = OGR_G_CreateGeometry(wkbMultiPoint);
    }
    else if (EQUAL(geom_type.c_str(), "LINESTRING")) {
        geom_type = "LINESTRING";
        hGeom = OGR_G_CreateGeometry(wkbLineString);
    }
    else if (EQUAL(geom_type.c_str(), "LINEARRING")) {
        geom_type = "LINEARRING";
        hGeom = OGR_G_CreateGeometry(wkbLinearRing);
    }
    else if (EQUAL(geom_type.c_str(), "POLYGON")) {
        geom_type = "POLYGON";
        hGeom = OGR_G_CreateGeometry(wkbLinearRing);
    }
    else {
        Rcpp::stop("geometry type not supported");
    }

    if (hGeom == nullptr)
        Rcpp::stop("failed to create geometry object");

    if (nPts == 1) {
        if (geom_type != "POINT" && geom_type != "MULTIPOINT") {
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
    else if (nPts > 0) {
        if (geom_type == "POINT") {
            OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("point geometry cannot have more than one xy");
        }
        if ((geom_type == "POLYGON" || geom_type == "LINEARRING") && nPts < 4) {
            OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("polygon/linearring must have at least four points");
        }

        if (geom_type == "MULTIPOINT") {
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

    if (geom_type != "POLYGON") {
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

    const int nWKBSize = OGR_G_WkbSize(hGeom_out);
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
        Rcpp::stop("failed to create geometry object from WKB, NA returned");
    }

    hGeom = createGeomFromWkb(container);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hSubGeom);
        Rcpp::stop("failed to create geometry object from WKB, NA returned");
    }

    const char *save_opt = nullptr;
    save_opt = CPLGetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING",
                                   nullptr);

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

    const int nWKBSize = OGR_G_WkbSize(hGeom);
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
// [[Rcpp::export(name = ".g_is_valid")]]
Rcpp::LogicalVector g_is_valid(const Rcpp::RawVector &geom,
                               bool quiet = false) {
// Test if the geometry is valid.
// This function is built on the GEOS library, check it for the definition
// of the geometry operation. If OGR is built without the GEOS library,
// this function will always return FALSE.

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = false;
    ret = OGR_G_IsValid(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_make_valid")]]
SEXP g_make_valid(const Rcpp::RawVector &geom,
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
// definition of the geometry operation. If OGR is built without GEOS >= 3.8,
// this function will return a clone of the input geometry if it is valid, or
// NULL if it is invalid

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

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
        if (GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3,4,0) || !geos_3_10_min) {
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

    OGRGeometryH hGeom = createGeomFromWkb(geom);
    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeomValid = nullptr;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3,4,0)
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
            Rcpp::warning("OGR MakeValid() gave NULL geometry, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const int nWKBSize = OGR_G_WkbSize(hGeomValid);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hGeomValid);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
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
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_is_empty")]]
Rcpp::LogicalVector g_is_empty(const Rcpp::RawVector &geom,
                               bool quiet = false) {
// Test if the geometry is empty.

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = false;
    ret = OGR_G_IsEmpty(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_name")]]
SEXP g_name(const Rcpp::RawVector &geom, bool quiet = false) {
// extract the geometry type name from a WKB/WKT geometry

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::wrap(NA_STRING);
    }

    std::string ret = "";
    ret = OGR_G_GetGeometryName(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return Rcpp::wrap(ret);
}

//' @noRd
// [[Rcpp::export(name = ".g_summary")]]
SEXP g_summary(const Rcpp::RawVector &geom, bool quiet = false) {
// "dump readable" summary of a WKB/WKT geometry

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 7, 0)
    Rcpp::stop("`g_summary()` requires GDAL >= 3.7");
#else
    OGRGeometryH hGeom = createGeomFromWkb(geom);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::wrap(NA_STRING);
    }

    const auto poGeom = OGRGeometry::FromHandle(hGeom);
    std::vector<const char *> options = {"DISPLAY_GEOMETRY=SUMMARY", nullptr};
    CPLString s = poGeom->dumpReadable(nullptr, options.data());
    s.replaceAll('\n', ' ');
    std::string ret = s.Trim();
    delete poGeom;
    return Rcpp::wrap(ret);

#endif
}

//' @noRd
// [[Rcpp::export(name = ".g_envelope")]]
Rcpp::NumericVector g_envelope(const Rcpp::RawVector &geom,
                               bool quiet = false) {
// Computes and returns the bounding envelope for this geometry.

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::NumericVector::create(NA_REAL, NA_REAL, NA_REAL, NA_REAL);
    }

    OGREnvelope sEnv;
    OGR_G_GetEnvelope(hGeom, &sEnv);
    Rcpp::NumericVector ret = {sEnv.MinX, sEnv.MinY, sEnv.MaxX, sEnv.MaxY};

    OGR_G_DestroyGeometry(hGeom);
    return ret;
}


// *** binary predicates ***


//' @noRd
// [[Rcpp::export(name = ".g_intersects")]]
Rcpp::LogicalVector g_intersects(const Rcpp::RawVector &this_geom,
                                 const Rcpp::RawVector &other_geom,
                                 bool quiet = false) {
// Determines whether two geometries intersect. If GEOS is enabled, then this
// is done in rigorous fashion otherwise TRUE is returned if the envelopes
// (bounding boxes) of the two geometries overlap.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = OGR_G_Intersects(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_equals")]]
Rcpp::LogicalVector g_equals(const Rcpp::RawVector &this_geom,
                             const Rcpp::RawVector &other_geom,
                             bool quiet = false) {
// Returns TRUE if two geometries are equivalent.
// This operation implements the SQL/MM ST_OrderingEquals() operation.
// The comparison is done in a structural way, that is to say that the
// geometry types must be identical, as well as the number and ordering of
// sub-geometries and vertices. Or equivalently, two geometries are
// considered equal by this method if their WKT/WKB representation is equal.
// Note: this must be distinguished from equality in a spatial way.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = OGR_G_Equals(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_disjoint")]]
Rcpp::LogicalVector g_disjoint(const Rcpp::RawVector &this_geom,
                               const Rcpp::RawVector &other_geom,
                               bool quiet = false) {
// Tests if this geometry and the other geometry are disjoint.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = OGR_G_Disjoint(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_touches")]]
Rcpp::LogicalVector g_touches(const Rcpp::RawVector &this_geom,
                              const Rcpp::RawVector &other_geom,
                              bool quiet = false) {
// Tests if this geometry and the other geometry are touching.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = OGR_G_Touches(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_contains")]]
Rcpp::LogicalVector g_contains(const Rcpp::RawVector &this_geom,
                               const Rcpp::RawVector &other_geom,
                               bool quiet = false) {
// Tests if this geometry contains the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = OGR_G_Contains(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_within")]]
Rcpp::LogicalVector g_within(const Rcpp::RawVector &this_geom,
                             const Rcpp::RawVector &other_geom,
                             bool quiet = false) {
// Tests if this geometry is within the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = OGR_G_Within(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_crosses")]]
Rcpp::LogicalVector g_crosses(const Rcpp::RawVector &this_geom,
                              const Rcpp::RawVector &other_geom,
                              bool quiet = false) {
// Tests if this geometry and the other geometry are crossing.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = OGR_G_Crosses(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_overlaps")]]
Rcpp::LogicalVector g_overlaps(const Rcpp::RawVector &this_geom,
                               const Rcpp::RawVector &other_geom,
                               bool quiet = false) {
// Tests if this geometry and the other geometry overlap, that is their
// intersection has a non-zero area (they have some but not all points in
// common).
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return NA_LOGICAL;
    }

    bool ret = OGR_G_Overlaps(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}


// *** unary operations ***


//' @noRd
// [[Rcpp::export(name = ".g_buffer")]]
SEXP g_buffer(const Rcpp::RawVector &geom, double dist, int quad_segs = 30,
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

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hBufferGeom = OGR_G_Buffer(hGeom, dist, quad_segs);

    if (hBufferGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_Buffer() gave NULL geometry, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const int nWKBSize = OGR_G_WkbSize(hBufferGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hBufferGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
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
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    return wkb;
}


// *** binary operations ***


//' @noRd
// [[Rcpp::export(name = ".g_intersection")]]
SEXP g_intersection(const Rcpp::RawVector &this_geom,
                    const Rcpp::RawVector &other_geom,
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
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_out = nullptr;
    hGeom_out = OGR_G_Intersection(hGeom_this, hGeom_other);
    if (hGeom_out == nullptr) {
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "OGR_G_Intersection() gave NULL geometry, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
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
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_union")]]
SEXP g_union(const Rcpp::RawVector &this_geom,
             const Rcpp::RawVector &other_geom,
             bool as_iso = false,
             const std::string &byte_order = "LSB",
             bool quiet = false) {
// Generates a new geometry which is the region of union of the two
// geometries operated on.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call IsValid() before, otherwise the result might
// be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_out = nullptr;
    hGeom_out = OGR_G_Union(hGeom_this, hGeom_other);
    if (hGeom_out == nullptr) {
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "OGR_G_Union() gave NULL geometry, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
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
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_difference")]]
SEXP g_difference(const Rcpp::RawVector &this_geom,
                  const Rcpp::RawVector &other_geom,
                  bool as_iso = false,
                  const std::string &byte_order = "LSB",
                  bool quiet = false) {
// Generates a new geometry which is the region of this geometry with the
// region of the other geometry removed.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call IsValid() before, otherwise the result might
// be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_out = nullptr;
    hGeom_out = OGR_G_Difference(hGeom_this, hGeom_other);
    if (hGeom_out == nullptr) {
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                "OGR_G_Difference() gave NULL geometry, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
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
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_sym_difference")]]
SEXP g_sym_difference(const Rcpp::RawVector &this_geom,
                      const Rcpp::RawVector &other_geom,
                      bool as_iso = false,
                      const std::string &byte_order = "LSB",
                      bool quiet = false) {
// Generates a new geometry which is the symmetric difference of this geometry
// and the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call IsValid() before, otherwise the result might
// be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
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
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    const int nWKBSize = OGR_G_WkbSize(hGeom_out);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom_out);
        OGR_G_DestroyGeometry(hGeom_other);
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
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
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    return wkb;
}


// *** measures ***


//' @noRd
// [[Rcpp::export(name = ".g_distance")]]
double g_distance(const Rcpp::RawVector &this_geom,
                  const Rcpp::RawVector &other_geom,
                  bool quiet = false) {
// Returns the distance between the geometries or -1 if an error occurs.
// Returns the shortest distance between the two geometries. The distance is
// expressed into the same unit as the coordinates of the geometries.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    if ((this_geom.size() == 0))
        Rcpp::stop("'this_geom' is empty");
    if ((other_geom.size() == 0))
        Rcpp::stop("'other_geom' is empty");

    OGRGeometryH hGeom_this = createGeomFromWkb(this_geom);
    if (hGeom_this == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB");
        }
        return -1;
    }

    OGRGeometryH hGeom_other = createGeomFromWkb(other_geom);
    if (hGeom_other == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB");
        }
        return -1;
    }

    double ret = -1;
    ret = OGR_G_Distance(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_length")]]
double g_length(const Rcpp::RawVector &geom, bool quiet = false) {
// Computes the length for OGRCurve (LineString) or MultiCurve objects.
// Undefined for all other geometry types (returns zero).

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

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
double g_area(const Rcpp::RawVector &geom, bool quiet = false) {
// Computes the area for an OGRLinearRing, OGRPolygon or OGRMultiPolygon.
// Undefined for all other geometry types (returns zero).

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

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
// [[Rcpp::export(name = ".g_centroid")]]
Rcpp::NumericVector g_centroid(const Rcpp::RawVector &geom,
                               bool quiet = false) {
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

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

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
SEXP g_transform(const Rcpp::RawVector &geom, const std::string &srs_from,
                 const std::string &srs_to, bool wrap_date_line = false,
                 int date_line_offset = 10, bool as_iso = false,
                 const std::string &byte_order = "LSB", bool quiet = false) {
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

    OGRSpatialReference oSourceSRS, oDestSRS;
    OGRCoordinateTransformation *poCT = nullptr;
    OGRGeomTransformerH hGeomTransformer = nullptr;
    OGRErr err = OGRERR_NONE;

    std::string srs_from_in = srs_to_wkt(srs_from, false);
    std::string srs_to_in = srs_to_wkt(srs_to, false);

    err = oSourceSRS.importFromWkt(srs_from_in.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import source SRS");

    err = oDestSRS.importFromWkt(srs_to_in.c_str());
    if (err != OGRERR_NONE)
        Rcpp::stop("failed to import destination SRS");

    poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oDestSRS);
    if (poCT == nullptr)
        Rcpp::stop("failed to create coordinate transformer");

    std::vector<char *> options;
    std::string offset;
    if (wrap_date_line) {
        options.push_back(const_cast<char *>("WRAPDATELINE=YES"));
        offset = "DATELINEOFFSET=" + std::to_string(date_line_offset);
        options.push_back(const_cast<char *>(offset.c_str()));
    }
    options.push_back(nullptr);

    hGeomTransformer = OGR_GeomTransformer_Create(
            OGRCoordinateTransformation::ToHandle(poCT), options.data());
    if (hGeomTransformer == nullptr) {
        OGRCoordinateTransformation::DestroyCT(poCT);
        Rcpp::stop("failed to create geometry transformer, NA returned");
    }

    if ((geom.size() == 0))
        Rcpp::stop("'geom' is empty");

    OGRGeometryH hGeom = createGeomFromWkb(geom);

    if (hGeom == nullptr) {
        if (!quiet) {
            Rcpp::warning(
                    "failed to create geometry object from WKB, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRGeometryH hGeom2 = nullptr;
    hGeom2 = OGR_GeomTransformer_Transform(hGeomTransformer, hGeom);
    if (hGeom2 == nullptr) {
        OGRCoordinateTransformation::DestroyCT(poCT);
        OGR_GeomTransformer_Destroy(hGeomTransformer);
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning(
                    "transformation failed, NA returned");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    OGRCoordinateTransformation::DestroyCT(poCT);
    OGR_GeomTransformer_Destroy(hGeomTransformer);

    const int nWKBSize = OGR_G_WkbSize(hGeom2);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hGeom2);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeom2, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom2);
    if (!result) {
        if (!quiet) {
           Rcpp::warning(
                    "failed to export WKB raw vector for output geometry");
        }
        return Rcpp::LogicalVector::create(NA_LOGICAL);
    }

    return wkb;
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
Rcpp::NumericVector bbox_from_wkt(const std::string &wkt,
        double extend_x = 0, double extend_y = 0) {

    OGRGeometryH hGeometry = nullptr;
    char* pszWKT;
    pszWKT = (char*) wkt.c_str();

    if (OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeometry) != OGRERR_NONE) {
        if (hGeometry != nullptr)
            OGR_G_DestroyGeometry(hGeometry);
        Rcpp::Rcerr << "failed to create geometry object from WKT string\n";
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
//' `NA` is returned if GDAL was built without the GEOS library.
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
Rcpp::String bbox_to_wkt(const Rcpp::NumericVector &bbox,
        double extend_x = 0, double extend_y = 0) {

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
