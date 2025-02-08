/* Exported convenience functions for GDAL data types
   Wrappers of GDAL data type functions in gdal.h

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef SRC_GDAL_DT_H_
#define SRC_GDAL_DT_H_

#include <string>

#include <Rcpp.h>

int dt_size(std::string dt, bool as_bytes);
bool dt_is_complex(std::string dt);
bool dt_is_integer(std::string dt);
bool dt_is_floating(std::string dt);
bool dt_is_signed(std::string dt);

std::string dt_union(std::string dt, std::string dt_other);
std::string dt_union_with_value(std::string dt, double val,
                                bool is_complex);

std::string dt_find(int bits, bool is_signed, bool is_floating,
                    bool is_complex);

std::string dt_find_for_value(double value, bool is_complex);

#endif  // SRC_GDAL_DT_H_
