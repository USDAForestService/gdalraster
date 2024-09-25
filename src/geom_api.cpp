/* GEOS wrapper functions operating on WKT geometries
   Called via GDAL ogr headers, requires GDAL built against GEOS.

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include "geom_api.h"

#include "cpl_port.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_api.h"
#include "ogr_spatialref.h"
#include "ogr_srs_api.h"

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
        if (hGeom != nullptr)
            OGR_G_DestroyGeometry(hGeom);

        Rcpp::stop(msg);
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
std::string g_create(const Rcpp::NumericMatrix &xy, std::string geom_type) {
// Create a geometry from a list of points (vertices).
// Currently for POINT, MULTIPOINT, LINESTRING, POLYGON.
// Only simple polygons composed of one ring are supported.

    OGRGeometryH hGeom = nullptr;
    OGRGeometryH hPoly = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT = nullptr;
    std::string wkt = "";

    if (EQUALN(geom_type.c_str(), "POINT", 5)) {
        geom_type = "POINT";
        hGeom = OGR_G_CreateGeometry(wkbPoint);
    }
    else if (EQUALN(geom_type.c_str(), "MULTIPOINT", 10)) {
        geom_type = "MULTIPOINT";
        hGeom = OGR_G_CreateGeometry(wkbMultiPoint);
    }
    else if (EQUALN(geom_type.c_str(), "LINESTRING", 10)) {
        geom_type = "LINESTRING";
        hGeom = OGR_G_CreateGeometry(wkbLineString);
    }
    else if (EQUALN(geom_type.c_str(), "POLYGON", 7)) {
        geom_type = "POLYGON";
        hGeom = OGR_G_CreateGeometry(wkbLinearRing);
    }
    else {
        Rcpp::stop("geometry type not supported");
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
            Rcpp::stop("polygon must have at least four points");
        }

        if (geom_type == "MULTIPOINT") {
            for (R_xlen_t i=0; i < nPts; ++i) {
                OGRGeometryH hPt = OGR_G_CreateGeometry(wkbPoint);
                OGR_G_SetPoint_2D(hPt, 0, xy(i, 0), xy(i, 1));
                err = OGR_G_AddGeometryDirectly(hGeom, hPt);
                if (err != OGRERR_NONE) {
                    if (hGeom != nullptr)
                        OGR_G_DestroyGeometry(hGeom);
                    Rcpp::stop("failed to add POINT to MULTIPOINT");
                }
            }
        }
        else {
            OGR_G_SetPointCount(hGeom, static_cast<int>(nPts));
            for (R_xlen_t i=0; i < nPts; ++i) {
                OGR_G_SetPoint_2D(hGeom, i, xy(i, 0), xy(i, 1));
            }
        }
    }

    if (geom_type == "POLYGON") {
        hPoly = OGR_G_CreateGeometry(wkbPolygon);
        if (hPoly == nullptr) {
            if (hGeom != nullptr)
                OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("failed to create polygon geometry object");
        }
        CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", "NO");
        err = OGR_G_AddGeometryDirectly(hPoly, hGeom);
        CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", nullptr);
        if (err != OGRERR_NONE) {
            if (hPoly != nullptr)
                OGR_G_DestroyGeometry(hPoly);
            Rcpp::stop("failed to create polygon geometry (unclosed ring?)");
        }

        OGR_G_ExportToWkt(hPoly, &pszWKT);
        OGR_G_DestroyGeometry(hPoly);
    }
    else {
        OGR_G_ExportToWkt(hGeom, &pszWKT);
        OGR_G_DestroyGeometry(hGeom);
    }

    if (pszWKT != nullptr) {
        wkt = pszWKT;
        CPLFree(pszWKT);
    }
    return wkt;
}

//' @noRd
// [[Rcpp::export(name = ".g_add_geom")]]
std::string g_add_geom(const std::string &sub_geom,
                       const std::string &container) {
// Add a geometry to a geometry container.
// LINEARRING (as POLYGON) to POLYGON, POINT to MULTIPOINT, LINESTRING to
// MULTILINESTRING, or POLYGON to MULTIPOLYGON

    OGRGeometryH hSubGeom = nullptr;
    OGRGeometryH hGeom = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_sub = const_cast<char *>(sub_geom.c_str());
    char *pszWKT_container = const_cast<char *>(container.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_sub, nullptr, &hSubGeom);
    if (err != OGRERR_NONE || hSubGeom == nullptr) {
        if (hSubGeom != nullptr)
            OGR_G_DestroyGeometry(hSubGeom);
        Rcpp::stop("failed to create geometry object for 'sub_geom'");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_container, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr) {
        if (hGeom != nullptr)
            OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hSubGeom);
        Rcpp::stop("failed to create geometry object for 'container'");
    }

    CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", "NO");

    if (EQUALN(OGR_G_GetGeometryName(hGeom), "POLYGON", 7) &&
            EQUALN(OGR_G_GetGeometryName(hSubGeom), "POLYGON", 7)) {
        // interpret sub_geom as one linearring

        OGRGeometryH hRing = OGR_G_GetGeometryRef(hSubGeom, 0);
        err = OGR_G_AddGeometry(hGeom, hRing);
        if (err != OGRERR_NONE) {
            if (hGeom != nullptr)
                OGR_G_DestroyGeometry(hGeom);
            if (hSubGeom != nullptr)
                OGR_G_DestroyGeometry(hSubGeom);
            Rcpp::stop("failed to add 'sub_geom' to 'container'");
        }
    }
    else {
        err = OGR_G_AddGeometryDirectly(hGeom, hSubGeom);
        if (err != OGRERR_NONE) {
            if (hGeom != nullptr)
                OGR_G_DestroyGeometry(hGeom);
            Rcpp::stop("failed to add 'sub_geom' to 'container'");
        }
    }

    CPLSetConfigOption("OGR_GEOMETRY_ACCEPT_UNCLOSED_RING", nullptr);

    char* pszWKT = nullptr;
    OGR_G_ExportToWkt(hGeom, &pszWKT);
    std::string wkt = "";
    if (pszWKT != nullptr) {
        wkt = pszWKT;
        CPLFree(pszWKT);
    }
    OGR_G_DestroyGeometry(hGeom);
    return wkt;
}

//' @noRd
// [[Rcpp::export(name = ".g_is_valid")]]
SEXP g_is_valid(const Rcpp::RawVector &geom, bool quiet = false) {
// Test if the geometry is valid.
// This function is built on the GEOS library, check it for the definition
// of the geometry operation. If OGR is built without the GEOS library,
// this function will always return FALSE.

    OGRGeometryH hGeom = createGeomFromWkb(geom);
    std::string msg = "";

    if (hGeom == nullptr) {
        if (!quiet) {
            msg = "failed to create geometry object from WKB, NA returned";
            Rcpp::warning(msg);
        }
        return Rcpp::wrap(NA_LOGICAL);
    }

    bool ret = false;
    ret = OGR_G_IsValid(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return Rcpp::wrap(ret);
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

    int geos_maj_ver = getGEOSVersion()[0];
    int geos_min_ver = getGEOSVersion()[1];
    bool geos_3_10_min = false;
    std::string msg = "";

    if (geos_maj_ver > 3 || (geos_maj_ver == 3 && geos_min_ver >= 10)) {
        geos_3_10_min = true;
    }
    else if ((geos_maj_ver == 3 && geos_min_ver < 8) ||
             geos_maj_ver < 3) {

        if (!quiet) {
            msg = "GEOS < 3.8 detected: g_make_valid() requires GEOS >= 3.8";
            Rcpp::warning(msg);
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
                msg = "STRUCTURE method requires GEOS >= 3.10 and GDAL >= 3.4";
                Rcpp::warning(msg);
            }

            opt.push_back("METHOD=LINEWORK");
        }
        else {
            opt.push_back("METHOD=STRUCTURE");
        }
    }
    else {
        if (!quiet) {
            msg = "value given for 'method' not recognized, using LINEWORK";
            Rcpp::warning(msg);
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
            msg = "failed to create geometry object from WKB, NA returned";
            Rcpp::warning(msg);
        }
        return Rcpp::wrap(NA_LOGICAL);
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
        return Rcpp::wrap(NA_LOGICAL);
    }

    const int nWKBSize = OGR_G_WkbSize(hGeomValid);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hGeomValid);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::wrap(NA_LOGICAL);
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hGeomValid, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeomValid);
    if (!result) {
        if (!quiet) {
            msg = "failed to export WKB raw vector for output geometry";
            Rcpp::warning(msg);
        }
        return Rcpp::wrap(NA_LOGICAL);
    }

    return wkb;
}

//' @noRd
// [[Rcpp::export(name = ".g_is_empty")]]
SEXP g_is_empty(const Rcpp::RawVector &geom, bool quiet = false) {
// Test if the geometry is empty.

    OGRGeometryH hGeom = createGeomFromWkb(geom);
    std::string msg = "";

    if (hGeom == nullptr) {
        if (!quiet) {
            msg = "failed to create geometry object from WKB, NA returned";
            Rcpp::warning(msg);
        }
        return Rcpp::wrap(NA_LOGICAL);
    }

    bool ret = false;
    ret = OGR_G_IsEmpty(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return Rcpp::wrap(ret);
}

//' @noRd
// [[Rcpp::export(name = ".g_name")]]
SEXP g_name(const Rcpp::RawVector &geom, bool quiet = false) {
// extract the geometry type name from a WKB/WKT geometry

    OGRGeometryH hGeom = createGeomFromWkb(geom);
    std::string msg = "";

    if (hGeom == nullptr) {
        if (!quiet) {
            msg = "failed to create geometry object from WKB, NA returned";
            Rcpp::warning(msg);
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

#if GDAL_VERSION_NUM < GDAL_COMPUTE_VERSION(3, 7, 0)
    Rcpp::stop("`g_summary()` requires GDAL >= 3.7");
#else
    OGRGeometryH hGeom = createGeomFromWkb(geom);
    std::string msg = "";

    if (hGeom == nullptr) {
        if (!quiet) {
            msg = "failed to create geometry object from WKB, NA returned";
            Rcpp::warning(msg);
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


// *** binary predicates ***


//' @noRd
// [[Rcpp::export(name = ".g_intersects")]]
bool g_intersects(const std::string &this_geom, const std::string &other_geom) {
// Determines whether two geometries intersect. If GEOS is enabled, then this
// is done in rigorous fashion otherwise TRUE is returned if the envelopes
// (bounding boxes) of the two geometries overlap.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Intersects(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_equals")]]
bool g_equals(const std::string &this_geom, const std::string &other_geom) {
// Returns TRUE if two geometries are equivalent.
// This operation implements the SQL/MM ST_OrderingEquals() operation.
// The comparison is done in a structural way, that is to say that the
// geometry types must be identical, as well as the number and ordering of
// sub-geometries and vertices. Or equivalently, two geometries are
// considered equal by this method if their WKT/WKB representation is equal.
// Note: this must be distinguished from equality in a spatial way.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Equals(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_disjoint")]]
bool g_disjoint(const std::string &this_geom, const std::string &other_geom) {
// Tests if this geometry and the other geometry are disjoint.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Disjoint(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_touches")]]
bool g_touches(const std::string &this_geom, const std::string &other_geom) {
// Tests if this geometry and the other geometry are touching.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Touches(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_contains")]]
bool g_contains(const std::string &this_geom, const std::string &other_geom) {
// Tests if this geometry contains the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Contains(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_within")]]
bool g_within(const std::string &this_geom, const std::string &other_geom) {
// Tests if this geometry is within the other geometry.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Within(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_crosses")]]
bool g_crosses(const std::string &this_geom, const std::string &other_geom) {
// Tests if this geometry and the other geometry are crossing.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    bool ret = OGR_G_Crosses(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_overlaps")]]
bool g_overlaps(const std::string &this_geom, const std::string &other_geom) {
// Tests if this geometry and the other geometry overlap, that is their
// intersection has a non-zero area (they have some but not all points in
// common).
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call g_is_valid() before, otherwise the result
// might be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
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

    OGRGeometryH hGeom = createGeomFromWkb(geom);
    std::string msg = "";

    if (hGeom == nullptr) {
        if (!quiet) {
            msg = "failed to create geometry object from WKB, NA returned";
            Rcpp::warning(msg);
        }
        return Rcpp::wrap(NA_LOGICAL);
    }

    OGRGeometryH hBufferGeom = OGR_G_Buffer(hGeom, dist, quad_segs);

    if (hBufferGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        if (!quiet) {
            Rcpp::warning("OGR_G_Buffer() gave NULL geometry, NA returned");
        }
        return Rcpp::wrap(NA_LOGICAL);
    }

    const int nWKBSize = OGR_G_WkbSize(hBufferGeom);
    if (!nWKBSize) {
        OGR_G_DestroyGeometry(hGeom);
        OGR_G_DestroyGeometry(hBufferGeom);
        if (!quiet) {
            Rcpp::warning("failed to obtain WKB size of output geometry");
        }
        return Rcpp::wrap(NA_LOGICAL);
    }

    Rcpp::RawVector wkb = Rcpp::no_init(nWKBSize);
    bool result = exportGeomToWkb(hBufferGeom, &wkb[0], as_iso, byte_order);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hBufferGeom);
    if (!result) {
        if (!quiet) {
            msg = "failed to export WKB raw vector for output geometry";
            Rcpp::warning(msg);
        }
        return Rcpp::wrap(NA_LOGICAL);
    }

    return wkb;
}


// *** binary operations ***


//' @noRd
// [[Rcpp::export(name = ".g_intersection")]]
std::string g_intersection(const std::string &this_geom,
                           const std::string &other_geom) {
// Generates a new geometry which is the region of intersection of the two
// geometries operated on. The g_intersects() function can be used to test
// if two geometries intersect.
// Geometry validity is not checked. In case you are unsure of the validity
// of the input geometries, call IsValid() before, otherwise the result might
// be wrong.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    OGRGeometryH hGeom = nullptr;
    hGeom = OGR_G_Intersection(hGeom_this, hGeom_other);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        OGR_G_DestroyGeometry(hGeom_other);
        return "";
    }

    char* pszWKT_out = nullptr;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out = "";
    if (pszWKT_out != nullptr) {
        wkt_out = std::string(pszWKT_out);
        CPLFree(pszWKT_out);
    }
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);

    return wkt_out;
}

//' @noRd
// [[Rcpp::export(name = ".g_union")]]
std::string g_union(const std::string &this_geom,
                    const std::string &other_geom) {
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
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    OGRGeometryH hGeom = nullptr;
    hGeom = OGR_G_Union(hGeom_this, hGeom_other);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        OGR_G_DestroyGeometry(hGeom_other);
        return "";
    }

    char* pszWKT_out = nullptr;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out = "";
    if (pszWKT_out != nullptr) {
        wkt_out = pszWKT_out;
        CPLFree(pszWKT_out);
    }
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);

    return wkt_out;
}

//' @noRd
// [[Rcpp::export(name = ".g_difference")]]
std::string g_difference(const std::string &this_geom,
                         const std::string &other_geom) {
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
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    OGRGeometryH hGeom = nullptr;
    hGeom = OGR_G_Difference(hGeom_this, hGeom_other);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        OGR_G_DestroyGeometry(hGeom_other);
        return "";
    }

    char* pszWKT_out = nullptr;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out = "";
    if (pszWKT_out != nullptr) {
        wkt_out = pszWKT_out;
        CPLFree(pszWKT_out);
    }
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);

    return wkt_out;
}

//' @noRd
// [[Rcpp::export(name = ".g_sym_difference")]]
std::string g_sym_difference(const std::string &this_geom,
                             const std::string &other_geom) {
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
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    OGRGeometryH hGeom = nullptr;
    hGeom = OGR_G_SymDifference(hGeom_this, hGeom_other);
    if (hGeom == nullptr) {
        OGR_G_DestroyGeometry(hGeom_this);
        OGR_G_DestroyGeometry(hGeom_other);
        return "";
    }

    char* pszWKT_out = nullptr;
    OGR_G_ExportToWkt(hGeom, &pszWKT_out);
    std::string wkt_out = "";
    if (pszWKT_out != nullptr) {
        wkt_out = pszWKT_out;
        CPLFree(pszWKT_out);
    }
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);

    return wkt_out;
}


// *** measures ***


//' @noRd
// [[Rcpp::export(name = ".g_distance")]]
double g_distance(const std::string &this_geom, const std::string &other_geom) {
// Returns the distance between the geometries or -1 if an error occurs.
// Returns the shortest distance between the two geometries. The distance is
// expressed into the same unit as the coordinates of the geometries.
// This function is built on the GEOS library, check it for the definition of
// the geometry operation. If OGR is built without the GEOS library, this
// function will always fail, issuing a CPLE_NotSupported error.

    OGRGeometryH hGeom_this = nullptr;
    OGRGeometryH hGeom_other = nullptr;
    OGRErr err = OGRERR_NONE;
    char *pszWKT_this = const_cast<char *>(this_geom.c_str());
    char *pszWKT_other = const_cast<char *>(other_geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT_this, nullptr, &hGeom_this);
    if (err != OGRERR_NONE || hGeom_this == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        Rcpp::stop("failed to create geometry object from first WKT string");
    }

    err = OGR_G_CreateFromWkt(&pszWKT_other, nullptr, &hGeom_other);
    if (err != OGRERR_NONE || hGeom_other == nullptr) {
        if (hGeom_this != nullptr)
            OGR_G_DestroyGeometry(hGeom_this);
        if (hGeom_other != nullptr)
            OGR_G_DestroyGeometry(hGeom_other);
        Rcpp::stop("failed to create geometry object from second WKT string");
    }

    double ret = -1;
    ret = OGR_G_Distance(hGeom_this, hGeom_other);
    OGR_G_DestroyGeometry(hGeom_this);
    OGR_G_DestroyGeometry(hGeom_other);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_length")]]
double g_length(const std::string &geom) {
// Computes the length for OGRCurve (LineString) or MultiCurve objects.
// Undefined for all other geometry types (returns zero).

    OGRGeometryH hGeom = nullptr;
    OGRErr err;
    char *pszWKT = const_cast<char *>(geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr) {
        if (hGeom != nullptr)
            OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to create geometry object from WKT string");
    }

    double ret = 0;
    ret = OGR_G_Length(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_area")]]
double g_area(const std::string &geom) {
// Computes the area for an OGRLinearRing, OGRPolygon or OGRMultiPolygon.
// Undefined for all other geometry types (returns zero).

    OGRGeometryH hGeom = nullptr;
    OGRErr err;
    char *pszWKT = const_cast<char *>(geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr) {
        if (hGeom != nullptr)
            OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to create geometry object from WKT string");
    }

    double ret = 0;
    ret = OGR_G_Area(hGeom);
    OGR_G_DestroyGeometry(hGeom);
    return ret;
}

//' @noRd
// [[Rcpp::export(name = ".g_centroid")]]
Rcpp::NumericVector g_centroid(const std::string &geom) {
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
    OGRErr err = OGRERR_NONE;
    char *pszWKT = const_cast<char *>(geom.c_str());

    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr) {
        if (hGeom != nullptr)
            OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to create geometry object from WKT string");
    }

    OGRGeometryH hPoint = nullptr;
    hPoint = OGR_G_CreateGeometry(wkbPoint);
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
std::string g_transform(const std::string &geom, const std::string &srs_from,
                        const std::string &srs_to, bool wrap_date_line = false,
                        int date_line_offset = 10) {
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
// This function uses the OGR_GeomTransformer_Create() and
// OGR_GeomTransformer_Transform() functions: this is a enhanced version of
// OGR_G_Transform(). When reprojecting geometries from a Polar Stereographic
// projection or a projection naturally crossing the antimeridian (like UTM
// Zone 60) to a geographic CRS, it will cut geometries along the antimeridian.
// So a LineString might be returned as a MultiLineString.

    OGRSpatialReference oSourceSRS, oDestSRS;
    OGRCoordinateTransformation *poCT = nullptr;
    OGRGeomTransformerH hGeomTransformer = nullptr;
    OGRGeometryH hGeom = nullptr;
    OGRGeometryH hGeom2 = nullptr;
    OGRErr err = OGRERR_NONE;

    char *pszWKT = const_cast<char *>(geom.c_str());
    err = OGR_G_CreateFromWkt(&pszWKT, nullptr, &hGeom);
    if (err != OGRERR_NONE || hGeom == nullptr) {
        if (hGeom != nullptr)
            OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to create geometry object from WKT string");
    }

    err = oSourceSRS.importFromWkt(srs_from.c_str());
    if (err != OGRERR_NONE) {
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to import source SRS from WKT string");
    }

    err = oDestSRS.importFromWkt(srs_to.c_str());
    if (err != OGRERR_NONE) {
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to import destination SRS from WKT string");
    }

    poCT = OGRCreateCoordinateTransformation(&oSourceSRS, &oDestSRS);
    if (poCT == nullptr) {
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to create coordinate transformer");
    }

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
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("failed to create geometry transformer");
    }

    hGeom2 = OGR_GeomTransformer_Transform(hGeomTransformer, hGeom);
    if (hGeom2 == nullptr) {
        OGRCoordinateTransformation::DestroyCT(poCT);
        OGR_GeomTransformer_Destroy(hGeomTransformer);
        OGR_G_DestroyGeometry(hGeom);
        Rcpp::stop("transformation failed");
    }

    char* pszWKT_out = nullptr;
    OGR_G_ExportToWkt(hGeom2, &pszWKT_out);
    std::string wkt_out = "";
    if (pszWKT_out != nullptr) {
        wkt_out = pszWKT_out;
        CPLFree(pszWKT_out);
    }

    OGRCoordinateTransformation::DestroyCT(poCT);
    OGR_GeomTransformer_Destroy(hGeomTransformer);
    OGR_G_DestroyGeometry(hGeom);
    OGR_G_DestroyGeometry(hGeom2);

    return wkt_out;
}

