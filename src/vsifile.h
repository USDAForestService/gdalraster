/* class VSIFile
    Bindings to the GDAL VSIVirtualHandle API. Encapsulates a VSIVirtualHandle.
    Chris Toney <chris.toney at usda.gov>
    Copyright (c) 2023-2025 gdalraster authors

    Requires {bit64} on the R side for its integer64 S3 type, since R does not
    have a native int64 type. Uses RcppInt64 for conversion between R double
    type (passed as Rccp::NumericVector) and C++ int64_t.

    From RcppInt64:
        "It relies on the bit64 package and its s3 type integer64 which use
        a variable stored as 'double' to transport the int64_t type it
        represents, along with proper type casting methods.

        One key aspect is that the 'double' (or in Rcpp parlance the
        'NumericVector' must carry the R class attribute so that the
        payload is taken---and interpreted---as a int64.""

    Use of the integer64 type is only required in order to specify file offsets
    larger than the range of double for representing exact integer (2^53).
    The bit64::integer64 type is signed, so the max file offset that can be
    used with the interface here is 9223372036854775807.
*/

#ifndef SRC_VSIFILE_H_
#define SRC_VSIFILE_H_

#include <cstdint>
#include <string>
#include "Rcpp.h"
#include "RcppInt64"

#include "cpl_vsi.h"

class VSIFile {
 public:
    VSIFile();
    explicit VSIFile(Rcpp::CharacterVector filename);
    VSIFile(Rcpp::CharacterVector filename, std::string access);
    VSIFile(Rcpp::CharacterVector filename, std::string access,
            Rcpp::CharacterVector options);
    ~VSIFile();

    void open();
    int seek(Rcpp::NumericVector offset, std::string origin);
    Rcpp::NumericVector tell() const;
    void rewind();
    SEXP read(Rcpp::NumericVector nbytes);
    Rcpp::NumericVector write(const Rcpp::RawVector& object);
    bool eof() const;
    int truncate(Rcpp::NumericVector offset);
    int flush();
    SEXP ingest(Rcpp::NumericVector max_size);
    int close();
    std::string get_filename() const;
    std::string get_access() const;
    int set_access(std::string access);

    void show() const;

 private:
    std::string m_filename;
    std::string m_access;
    Rcpp::CharacterVector m_options;
    VSILFILE *m_fp;
};

// cppcheck-suppress unknownMacro
RCPP_EXPOSED_CLASS(VSIFile)

#endif  // SRC_VSIFILE_H_
