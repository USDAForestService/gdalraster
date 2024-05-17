/* Implementation of class VSIFile
   Encapsulates a VSIVirtualHandle file pointer.
   Chris Toney <chris.toney at usda.gov> */

#include <cstdlib>
#include <vector>

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
            Rcpp::CharacterVector options) :

            VSILFILE(nullptr) {

    filename_in = Rcpp::as<std::string>(_check_gdal_filename(filename));
    access_in = access.c_str();
    VSILFILE = reinterpret_cast<VSIVirtualHandle *>(
            VSIFOpenL(filename_in.c_str(), access_in));
    if (VSILFILE == nullptr)
        Rcpp::stop("failed to obtain virtual file handle");
}

int VSIFile::seek(Rcpp::NumericVector offset, std::string origin) {
    // offset must be an R numeric vector of length 1
    // i.e., a scalar but use NumericVector here since it can carry the class
    // attribute for integer64
    //
    // allowed values of origin:
    // "SEEK_SET" seeking from beginning of the file
    // "SEEK_CUR" seeking from the current file position
    // "SEEK_END" seeking from end of the file

    if (VSILFILE == nullptr)
        Rcpp::stop("the file is not open");

    if (offset.size() != 1)
        Rcpp::stop("'offset' must be a length-1 numeric vector");

    int64_t offset_in = -1;

    if (Rcpp::isInteger64(offset)) {
        offset_in = Rcpp::fromInteger64(offset[0]);
    }
    else {
        double tmp = Rcpp::as<double>(offset);
        offset_in = static_cast<int64_t>(tmp);
    }

    Rcpp::Rcout << offset_in;

    if (offset_in < 0)
        Rcpp::stop("'offset cannot be a negative number");

    int origin_in = -1;
    if (EQUALN(origin.c_str(), "SEEK_SET", 8))
        origin_in = SEEK_SET;
    else if (EQUALN(origin.c_str(), "SEEK_CUR", 8))
        origin_in = SEEK_CUR;
    else if (EQUALN(origin.c_str(), "SEEK_END", 8))
        origin_in = SEEK_END;
    else
        Rcpp::stop("'origin' is invalid");

    return VSIFSeekL(VSILFILE, offset_in, origin_in);
}

Rcpp::NumericVector VSIFile::tell() const {
    if (VSILFILE == nullptr)
        Rcpp::stop("the file is not open");

    vsi_l_offset offset = VSIFTellL(VSILFILE);
    if (offset > _R_VSI_L_OFFSET_MAX)
        Rcpp::stop("the current file offset exceeds R integer64 upper limit");

    int64_t ret = static_cast<int64_t>(offset);
    return Rcpp::wrap(ret);
}

void VSIFile::rewind() {
    if (VSILFILE == nullptr)
        Rcpp::stop("the file is not open");

    VSIRewindL(VSILFILE);
}

SEXP VSIFile::read(std::size_t count) {
    if (VSILFILE == nullptr)
        Rcpp::stop("the file is not open");

    void *buf = VSIMalloc(count);
    size_t nRead = 0;
    nRead = VSIFReadL(buf, 1, count, VSILFILE);
    if (nRead == 0)
        return R_NilValue;

    Rcpp::RawVector raw(nRead);
    std::memcpy(&raw[0], buf, nRead);
    VSIFree(buf);
    return raw;
}

Rcpp::NumericVector VSIFile::write(const Rcpp::RawVector& buf) {
    if (VSILFILE == nullptr)
        Rcpp::stop("the file is not open");

    size_t nSize = 1;
    size_t nCount = buf.size();
    int64_t ret = static_cast<int64_t>(
            VSIFWriteL(&buf[0], nSize, nCount, VSILFILE));

    return Rcpp::wrap(ret);
}

bool VSIFile::eof() const {
    if (VSILFILE == nullptr)
        Rcpp::stop("the file is not open");

    int eof = VSIFEofL(VSILFILE);
    if (eof == 0)
        return false;
    else
        return true;
}

SEXP VSIFile::ingest(Rcpp::NumericVector max_size) {
    // max_size must be an R numeric vector of length 1
    // i.e., a scalar but use NumericVector here since it can carry the class
    // attribute for integer64
    if (VSILFILE == nullptr)
        Rcpp::stop("the file is not open");

    if (max_size.size() != 1)
        Rcpp::stop("'max_size' must be a length-1 numeric vector (integer64)");

    int64_t max_size_in;
    if (Rcpp::isInteger64(max_size))
        max_size_in = Rcpp::fromInteger64(max_size[0]);
    else
        max_size_in = static_cast<int64_t>(max_size[0]);

    GByte *paby = nullptr;
    vsi_l_offset nSize = 0;

    int result = VSIIngestFile(VSILFILE, nullptr, &paby, &nSize,
                               (GIntBig) max_size_in);

    if (!result) {
        Rcpp::Rcerr << "failed to ingest file\n";
        return R_NilValue;
    }

    Rcpp::RawVector raw(nSize);
    std::memcpy(&raw[0], paby, nSize);
    VSIFree(paby);
    return raw;
}

int VSIFile::close() {
    int ret = -1;
    if (VSILFILE != nullptr) {
        ret = VSIFCloseL(VSILFILE);
        if (ret == 0)
            VSILFILE = nullptr;
    }
    else {
        Rcpp::Rcout << "VSIVirtualHandle is NULL so VSIFCloseL() not called\n";
    }
    return ret;
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
    .method("seek", &VSIFile::seek,
        "Seek to requested offset")
    .const_method("tell", &VSIFile::tell,
        "Tell current file offset")
    .method("rewind", &VSIFile::rewind,
        "Rewind the file pointer to the beginning of the file")
    .method("read", &VSIFile::read,
        "Read bytes from file")
    .method("write", &VSIFile::write,
        "Write bytes to file")
    .const_method("eof", &VSIFile::eof,
        "Test for end of file")
    .method("ingest", &VSIFile::ingest,
        "Ingest file into memory and return as R raw vector")
    .method("close", &VSIFile::close,
        "Close file")

    ;
}
