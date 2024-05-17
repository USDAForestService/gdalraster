/* class VSIFile
   Bindings to the GDAL VSIVirtualHandle API. Encapsulates a VSIVirtualHandle.
   Chris Toney <chris.toney at usda.gov> */

#ifndef SRC_VSIFILE_H_
#define SRC_VSIFILE_H_

#include <cstdint>
#include <string>
#include "Rcpp.h"
#include "RcppInt64"

#include "gdal.h"
// #include "cpl_string.h"
#include "cpl_vsi.h"

class VSIFile {
 private:
    std::string filename_in;
    const char* access_in;  // access: "r", "r+", "w", "a"
    Rcpp::CharacterVector options_in;
    VSIVirtualHandle *VSILFILE;
    const uint64_t _R_VSI_L_OFFSET_MAX = 9223372036854775807;

 public:
    VSIFile();
    explicit VSIFile(Rcpp::CharacterVector filename);
    VSIFile(Rcpp::CharacterVector filename, std::string access);
    VSIFile(Rcpp::CharacterVector filename, std::string access,
            Rcpp::CharacterVector options);

    SEXP open();
    SEXP stat(std::string info);
    int seek(Rcpp::NumericVector offset, std::string origin);
    Rcpp::NumericVector tell() const;  // returns integer64 scalar
    void rewind();
    SEXP read(std::size_t count);  // returns RawVector, or NULL
    std::size_t write(const Rcpp::RawVector& buf);
    bool eof() const;
    int truncate(GUIntBig offset);
    int flush();
    int printf(std::string fmt);
    int putc(int c);
    SEXP ingest(Rcpp::NumericVector max_size);  // returns RawVector, or NULL
    int close();
};

RCPP_EXPOSED_CLASS(VSIFile)



#endif  // SRC_VSIFILE_H_
