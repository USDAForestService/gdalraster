/* Exported convenience functions for GDAL data types
   Wrappers of GDAL data type functions in gdal.h

   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#ifndef GDAL_DT_H_
#define GDAL_DT_H_

#include <Rcpp.h>

#include <string>


int dt_size(const std::string &dt, bool as_bytes);
bool dt_is_complex(const std::string &dt);
bool dt_is_integer(const std::string &dt);
bool dt_is_floating(const std::string &dt);
bool dt_is_signed(const std::string &dt);

std::string dt_union(const std::string &dt, const std::string &dt_other);
std::string dt_union_with_value(const std::string &dt, double val,
                                bool is_complex);

std::string dt_find(int bits, bool is_signed, bool is_floating,
                    bool is_complex);

std::string dt_find_for_value(double value, bool is_complex);

#endif  // GDAL_DT_H_
