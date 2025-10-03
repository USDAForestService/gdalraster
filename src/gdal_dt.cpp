/* Exported convenience functions for GDAL data types
   Wrappers of GDAL data type functions in gdal.h

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include "gdal_dt.h"

#include <gdal.h>

#include <Rcpp.h>

#include <string>


//' Helper functions for GDAL raster data types
//'
//' These are convenience functions that return information about a raster
//' data type, return the smallest data type that can fully express two input
//' data types, or find the smallest data type able to support specified
//' requirements.
//'
//' @name data_type_helpers
//'
//' @details
//' `dt_size()` returns the data type size in bytes by default, optionally in
//' bits (returns zero if `dt` is not recognized).
//'
//' `dt_is_complex()` returns `TRUE` if the passed type is complex (one of
//' CInt16, CInt32, CFloat32 or CFloat64), i.e., if it consists of a real and
//' imaginary component.
//'
//' `dt_is_integer()` returns `TRUE` if the passed type is integer (one of
//' Byte, Int16, UInt16, Int32, UInt32, CInt16, CInt32).
//'
//' `dt_is_floating()` returns `TRUE` if the passed type is floating (one of
//' Float32, Float16, Float64, CFloat16, CFloat32, CFloat64).
//'
//' `dt_is_signed()` returns `TRUE` if the passed type is signed.
//'
//' `dt_union()` returns the smallest data type that can fully express both
//' input data types (returns a data type name as character string).
//'
//' `dt_union_with_value()` unions a data type with the data type found for a
//' given value, and returns the resulting data type name as character string.
//'
//' `dt_find()` finds the smallest data type able to support the given
//' requirements (returns a data type name as character string).
//'
//' `dt_find_for_value()` finds the smallest data type able to support the
//' given `value` (returns a data type name as character string).
//'
//' @param dt Character string containing a GDAL data type name (e.g.,
//' `"Byte"`, `"Int16"`, `"UInt16"`, `"Int32"`, `"UInt32"`, `"Float32"`,
//' `"Float64"`, etc.)
//' @param as_bytes Logical value, `TRUE` to return data type size in bytes
//' (the default), `FALSE` to return the size in bits.
//' @param dt_other Character string containing a GDAL data type name.
//' @param value Numeric value for which to find a data type (passing the real
//' part if `is_complex = TRUE`).
//' @param is_complex Logical value, `TRUE` if `value` is complex (default is
//' `FALSE`), or if complex values are necessary in `dt_find()`.
//' @param bits Integer value specifying the number of bits necessary.
//' @param is_signed Logical value, `TRUE` if negative values are necessary.
//' @param is_floating Logical value, `TRUE` if non-integer values are
//' necessary.
//'
//' @seealso
//' [`GDALRaster$getDataTypeName()`][GDALRaster]
//'
//' @examples
//' elev_file <- system.file("extdata/storml_elev.tif", package="gdalraster")
//' ds <- new(GDALRaster, elev_file)
//'
//' ds$getDataTypeName(band = 1) |> dt_size()
//' ds$getDataTypeName(band = 1) |> dt_size(as_bytes = FALSE)
//' ds$getDataTypeName(band = 1) |> dt_is_complex()
//' ds$getDataTypeName(band = 1) |> dt_is_integer()
//' ds$getDataTypeName(band = 1) |> dt_is_floating()
//' ds$getDataTypeName(band = 1) |> dt_is_signed()
//'
//' ds$close()
//'
//' f <- system.file("extdata/complex.tif", package="gdalraster")
//' ds <- new(GDALRaster, f)
//'
//' ds$getDataTypeName(band = 1) |> dt_size()
//' ds$getDataTypeName(band = 1) |> dt_size(as_bytes = FALSE)
//' ds$getDataTypeName(band = 1) |> dt_is_complex()
//' ds$getDataTypeName(band = 1) |> dt_is_integer()
//' ds$getDataTypeName(band = 1) |> dt_is_floating()
//' ds$getDataTypeName(band = 1) |> dt_is_signed()
//'
//' ds$close()
//'
//' dt_union("Byte", "Int16")
//' dt_union_with_value("Byte", -1)
//' dt_union_with_value("Byte", 256)
//'
//' dt_find(bits = 32, is_signed = FALSE, is_floating = FALSE)
//' dt_find_for_value(0)
//' dt_find_for_value(-1)
//' dt_find_for_value(NaN)
//' dt_find_for_value(.Machine$integer.max)
// [[Rcpp::export]]
int dt_size(const std::string &dt, bool as_bytes = true) {
    GDALDataType eDT = GDALGetDataTypeByName(dt.c_str());
    if (as_bytes)
        return GDALGetDataTypeSizeBytes(eDT);
    else
        return GDALGetDataTypeSizeBits(eDT);
}

//' @rdname data_type_helpers
// [[Rcpp::export]]
bool dt_is_complex(const std::string &dt) {
    GDALDataType eDT = GDALGetDataTypeByName(dt.c_str());
    return static_cast<bool>(GDALDataTypeIsComplex(eDT));
}

//' @rdname data_type_helpers
// [[Rcpp::export]]
bool dt_is_integer(const std::string &dt) {
    GDALDataType eDT = GDALGetDataTypeByName(dt.c_str());
    return static_cast<bool>(GDALDataTypeIsInteger(eDT));
}

//' @rdname data_type_helpers
// [[Rcpp::export]]
bool dt_is_floating(const std::string &dt) {
    GDALDataType eDT = GDALGetDataTypeByName(dt.c_str());
    return static_cast<bool>(GDALDataTypeIsFloating(eDT));
}

//' @rdname data_type_helpers
// [[Rcpp::export]]
bool dt_is_signed(const std::string &dt) {
    GDALDataType eDT = GDALGetDataTypeByName(dt.c_str());
    return static_cast<bool>(GDALDataTypeIsSigned(eDT));
}

//' @rdname data_type_helpers
// [[Rcpp::export]]
std::string dt_union(const std::string &dt, const std::string &dt_other) {
    GDALDataType eDT1 = GDALGetDataTypeByName(dt.c_str());
    GDALDataType eDT2 = GDALGetDataTypeByName(dt_other.c_str());
    GDALDataType eDT_out = GDALDataTypeUnion(eDT1, eDT2);
    return GDALGetDataTypeName(eDT_out);
}

//' @rdname data_type_helpers
// [[Rcpp::export]]
std::string dt_union_with_value(const std::string &dt, double value,
                                bool is_complex = false) {

    GDALDataType eDT = GDALGetDataTypeByName(dt.c_str());
    GDALDataType eDT_out = GDALDataTypeUnionWithValue(eDT, value, is_complex);
    return GDALGetDataTypeName(eDT_out);
}

//' @rdname data_type_helpers
// [[Rcpp::export]]
std::string dt_find(int bits, bool is_signed, bool is_floating,
                    bool is_complex = false) {

    GDALDataType eDT_out = GDALFindDataType(bits, is_signed, is_floating,
                                            is_complex);

    return GDALGetDataTypeName(eDT_out);
}

//' @rdname data_type_helpers
// [[Rcpp::export]]
std::string dt_find_for_value(double value, bool is_complex = false) {
    GDALDataType eDT_out = GDALFindDataTypeForValue(value, is_complex);
    return GDALGetDataTypeName(eDT_out);
}
