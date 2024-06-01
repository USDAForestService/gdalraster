/* Implementation of class VSIFile
   Encapsulates a VSIVirtualHandle file pointer.
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2024 gdalraster authors
*/

#include <cstdlib>
#include <vector>

#include "cpl_conv.h"
#include "cpl_port.h"
#include "gdalraster.h"
#include "vsifile.h"

VSIFile::VSIFile() :
            filename_in(""),
            access_in("r"),
            options_in(Rcpp::CharacterVector::create()),
            fp(nullptr) {}

VSIFile::VSIFile(Rcpp::CharacterVector filename) :
            VSIFile(filename, "r", Rcpp::CharacterVector::create()) {}

VSIFile::VSIFile(Rcpp::CharacterVector filename, std::string access) :
            VSIFile(filename, access, Rcpp::CharacterVector::create()) {}

VSIFile::VSIFile(Rcpp::CharacterVector filename, std::string access,
                 Rcpp::CharacterVector options) : fp(nullptr) {

    filename_in = Rcpp::as<std::string>(_check_gdal_filename(filename));
    if (access.length() > 0 && access.length() < 4)
        access_in = access;
    else
        Rcpp::stop("'access' should be 'r', 'r+' or 'w'");
    options_in = options;
    open();
}

VSIFile::~VSIFile() {
    close();
}

void VSIFile::open() {
    if (fp != nullptr)
        Rcpp::stop("the file is already open");

    if (options_in.size() > 0) {
        if (_gdal_version_num() < 3030000)
            Rcpp::stop("'options' parameter requires GDAL >= 3.3");

        std::vector<const char *> opt_list(options_in.size());
        for (R_xlen_t i = 0; i < options_in.size(); ++i) {
            opt_list[i] = (const char *) (options_in[i]);
        }
        opt_list[options_in.size()] = nullptr;

        fp = VSIFOpenEx2L(filename_in.c_str(), access_in.c_str(), TRUE,
                          opt_list.data());
    }
    else {
        fp = VSIFOpenExL(filename_in.c_str(), access_in.c_str(), TRUE);
    }

    if (fp == nullptr)
        Rcpp::stop("failed to obtain a virtual file handle");

    return;
}

int VSIFile::seek(Rcpp::NumericVector offset, std::string origin) {
    // offset must be an R numeric vector of length 1
    // i.e., a scalar but use NumericVector here since it can carry the class
    // attribute for integer64

    if (fp == nullptr)
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

    if (offset_in < 0)
        Rcpp::stop("'offset' cannot be a negative number");

    int origin_in = -1;
    if (EQUALN(origin.c_str(), "SEEK_SET", 8))
        origin_in = SEEK_SET;
    else if (EQUALN(origin.c_str(), "SEEK_CUR", 8))
        origin_in = SEEK_CUR;
    else if (EQUALN(origin.c_str(), "SEEK_END", 8))
        origin_in = SEEK_END;
    else
        Rcpp::stop("'origin' is invalid");

    return VSIFSeekL(fp, static_cast<vsi_l_offset>(offset_in),
                     origin_in);
}

Rcpp::NumericVector VSIFile::tell() const {
    if (fp == nullptr)
        Rcpp::stop("the file is not open");

    vsi_l_offset offset = VSIFTellL(fp);
    if (offset > _R_VSI_L_OFFSET_MAX)
        Rcpp::stop("the current file offset exceeds R integer64 upper limit");

    std::vector<int64_t> ret(1);
    ret[0] = static_cast<int64_t>(offset);
    return Rcpp::wrap(ret);
}

void VSIFile::rewind() {
    if (fp == nullptr)
        Rcpp::stop("the file is not open");

    VSIRewindL(fp);
    return;
}

SEXP VSIFile::read(std::size_t nbytes) {
    if (fp == nullptr)
        Rcpp::stop("the file is not open");

    if (nbytes <= 0)
        return R_NilValue;

    GByte *buf = static_cast<GByte *>(CPLMalloc(nbytes));
    size_t nRead = 0;
    nRead = VSIFReadL(buf, 1, nbytes, fp);
    if (nRead == 0) {
        VSIFree(buf);
        return R_NilValue;
    }

    Rcpp::RawVector raw(nRead);
    std::memcpy(&raw[0], buf, nRead);
    VSIFree(buf);
    return raw;
}

Rcpp::NumericVector VSIFile::write(const Rcpp::RawVector& object) {
    if (fp == nullptr)
        Rcpp::stop("the file is not open");

    std::vector<int64_t> ret(1);
    ret[0] = static_cast<int64_t>(
            VSIFWriteL(&object[0], 1, static_cast<size_t>(object.size()), fp));

    return Rcpp::wrap(ret);
}

bool VSIFile::eof() const {
    if (fp == nullptr)
        Rcpp::stop("the file is not open");

    int eof = VSIFEofL(fp);
    if (eof == 0)
        return false;
    else
        return true;
}

int VSIFile::truncate(Rcpp::NumericVector new_size) {
    if (fp == nullptr)
        Rcpp::stop("the file is not open");

    if (new_size.size() != 1)
        Rcpp::stop("'new_size' must be a length-1 numeric vector");

    int64_t new_size_in = -1;

    if (Rcpp::isInteger64(new_size)) {
        new_size_in = Rcpp::fromInteger64(new_size[0]);
    }
    else {
        double tmp = Rcpp::as<double>(new_size);
        new_size_in = static_cast<int64_t>(tmp);
    }

    if (new_size_in < 0)
        Rcpp::stop("'offset' cannot be a negative number");

    return VSIFTruncateL(fp, static_cast<vsi_l_offset>(new_size_in));
}

int VSIFile::flush() {
    if (fp == nullptr)
        Rcpp::stop("the file is not open");

    return VSIFFlushL(fp);
}

SEXP VSIFile::ingest(Rcpp::NumericVector max_size) {
    // max_size must be an R numeric vector of length 1
    // i.e., a scalar but use NumericVector here since it can carry the class
    // attribute for integer64

    if (fp == nullptr)
        Rcpp::stop("the file is not open");

    if (max_size.size() != 1)
        Rcpp::stop("'max_size' must be a length-1 numeric vector (integer64)");

    int64_t max_size_in;

    if (Rcpp::isInteger64(max_size)) {
        max_size_in = Rcpp::fromInteger64(max_size[0]);
    }
    else {
        double tmp = Rcpp::as<double>(max_size);
        max_size_in = static_cast<int64_t>(tmp);
    }

    GByte *paby = nullptr;
    vsi_l_offset nSize = 0;

    int result = VSIIngestFile(fp, nullptr, &paby, &nSize, max_size_in);

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
    if (fp != nullptr) {
        ret = VSIFCloseL(fp);
        if (ret == 0)
            fp = nullptr;
    }
    return ret;
}

std::string VSIFile::get_filename() const {
    return filename_in;
}

std::string VSIFile::get_access() const {
    return access_in;
}

int VSIFile::set_access(std::string access) {
    if (fp != nullptr) {
        Rcpp::Rcerr << "cannot set access while the file is open\n";
        return -1;
    }

    if (access.length() > 0 && access.length() < 4) {
        access_in = access;
        return 0;
    }
    else {
        Rcpp::Rcerr << "'access' should be 'r', 'r+' or 'w'\n";
        return -1;
    }
}

RCPP_MODULE(mod_VSIFile) {
    Rcpp::class_<VSIFile>("VSIFile")

    .constructor
        ("Default constructor, no file handle opened")
    .constructor<Rcpp::CharacterVector>
        ("Usage: new(VSIFile, filename)")
    .constructor<Rcpp::CharacterVector, std::string>
        ("Usage: new(VSIFile, filename, access)")
    .constructor<Rcpp::CharacterVector, std::string, Rcpp::CharacterVector>
        ("Usage: new(VSIFile, filename, access, options)")

    // exposed member functions
    .method("open", &VSIFile::open,
        "Open file")
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
    .method("truncate", &VSIFile::truncate,
        "Truncate/expand the file to the specified size")
    .method("flush", &VSIFile::flush,
        "Flush pending writes to disk")
    .method("ingest", &VSIFile::ingest,
        "Ingest file into memory and return as raw vector")
    .method("close", &VSIFile::close,
        "Close file")
    .const_method("get_filename", &VSIFile::get_filename,
        "Return the filename")
    .const_method("get_access", &VSIFile::get_access,
        "Return the access")
    .method("set_access", &VSIFile::set_access,
        "Set the access if the file is closed")

    ;
}
