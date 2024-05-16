/* Implementation of class VSIFile
   Encapsulates a VSIVirtualHandle file pointer.
   Chris Toney <chris.toney at usda.gov> */

#include "vsifile.h"
#include "gdalraster.h"

VSIFile::VSIFile() :
            filename_in(""),
            access_in("r"),
            options_in(Rcpp::CharacterVector::create()),
            VSILFILE(nullptr) {}

VSIFile::VSIFile(Rcpp::CharacterVector filename) :
            VSIFile(
                filename,
                "r",
                Rcpp::CharacterVector::create()) {}

VSIFile::VSIFile(Rcpp::CharacterVector filename, std::string access) :
            VSIFile(
                filename,
                access,
                Rcpp::CharacterVector::create()) {}


VSIFile::VSIFile(Rcpp::CharacterVector filename, std::string access,
            Rcpp::CharacterVector options) {

    filename_in = Rcpp::as<std::string>(_check_gdal_filename(filename));
    access_in = access.c_str();
    VSILFILE = nullptr;
    VSILFILE = reinterpret_cast<VSIVirtualHandle *>(
            VSIFOpenL(filename_in.c_str(), access_in));
    if (VSILFILE == nullptr)
        Rcpp::stop("failed to obtain virtual file handle");
}

int VSIFile::close() {
    return VSIFCloseL(VSILFILE);
}


// ****************************************************************************

RCPP_MODULE(mod_VSIFile) {

    Rcpp::class_<VSIFile>("VSIFile")

    .constructor
        ("Default constructor")
    .constructor<Rcpp::CharacterVector>
        ("Usage: new(VSIFile, filename)")
    .constructor<Rcpp::CharacterVector, std::string>
        ("Usage: new(VSIFile, filename, access)")
    .constructor<Rcpp::CharacterVector, std::string, Rcpp::CharacterVector>
        ("Usage: new(VSIFile, filename, access, options)")

    // exposed member functions
    .method("close", &VSIFile::close,
        "Close file")

    ;
}
