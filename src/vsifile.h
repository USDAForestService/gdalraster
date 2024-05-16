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

// define R_VSI_L_OFFSET_MAX = lim.integer64()[2]

class VSIFile {
 private:
    std::string filename_in;
    const char* access_in;  // access: "r", "r+", "w", "a"
    Rcpp::CharacterVector options_in;
    VSIVirtualHandle *VSILFILE;

 public:
    VSIFile();
    explicit VSIFile(Rcpp::CharacterVector filename);
    VSIFile(Rcpp::CharacterVector filename, std::string access);
    VSIFile(Rcpp::CharacterVector filename, std::string access,
            Rcpp::CharacterVector options);

    SEXP open();
    int close();
    SEXP stat(std::string info);
    int seek(int64_t offset, int origin);
    int64_t tell() const;
    void rewind();
    Rcpp::RawVector read(std::size_t size, std::size_t count);
    std::size_t write(const Rcpp::RawVector& buf);
    bool eof() const;
    int truncate(GUIntBig offset);
    int flush();
    int printf(std::string fmt);
    int putc(int c);
    Rcpp::RawVector ingest(int64_t max_size);

};

RCPP_EXPOSED_CLASS(VSIFile)

#endif  // SRC_VSIFILE_H_
