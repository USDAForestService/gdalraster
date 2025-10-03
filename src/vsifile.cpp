/* Implementation of class VSIFile
   Encapsulates a VSIVirtualHandle file pointer.
   Chris Toney <chris.toney at usda.gov>
   Copyright (c) 2023-2025 gdalraster authors
*/

#include "vsifile.h"

#include <cpl_port.h>
#include <cpl_conv.h>

#include <Rcpp.h>
#include <RcppInt64>

#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include "gdalraster.h"
#include "rcpp_util.h"

constexpr uint64_t VSI_L_OFFSET_MAX_R_ = MAX_INTEGER64;

VSIFile::VSIFile()
        : m_filename(""), m_access("r"),
          m_options(Rcpp::CharacterVector::create()), m_fp(nullptr) {}

VSIFile::VSIFile(Rcpp::CharacterVector filename)
        : VSIFile(filename, "r", Rcpp::CharacterVector::create()) {}

VSIFile::VSIFile(Rcpp::CharacterVector filename, std::string access)
        : VSIFile(filename, access, Rcpp::CharacterVector::create()) {}

VSIFile::VSIFile(Rcpp::CharacterVector filename, std::string access,
                 Rcpp::CharacterVector options)
        : m_fp(nullptr) {

    m_filename = Rcpp::as<std::string>(check_gdal_filename(filename));
    if (access.length() > 0 && access.length() < 4)
        m_access = access;
    else
        Rcpp::stop("'access' should be 'r', 'r+', 'w' or 'w+'");

    m_options = options;
    open();
}

VSIFile::~VSIFile() {
    if (m_fp)
        VSIFCloseL(m_fp);
}

void VSIFile::open() {
    if (m_fp != nullptr)
        Rcpp::stop("the file is already open");

    if (m_options.size() > 0) {
        if (gdal_version_num() < 3030000)
            Rcpp::stop("'options' parameter requires GDAL >= 3.3");

        std::vector<const char *> opt_list(m_options.size());
        for (R_xlen_t i = 0; i < m_options.size(); ++i) {
            opt_list[i] = (const char *) (m_options[i]);
        }
        opt_list[m_options.size()] = nullptr;

        m_fp = VSIFOpenEx2L(m_filename.c_str(), m_access.c_str(), TRUE,
                            opt_list.data());
    }
    else {
        m_fp = VSIFOpenExL(m_filename.c_str(), m_access.c_str(), TRUE);
    }

    if (m_fp == nullptr)
        Rcpp::stop("failed to obtain a virtual file handle");

    return;
}

int VSIFile::seek(Rcpp::NumericVector offset, std::string origin) {
    // offset must be an R numeric vector of length 1
    // i.e., a scalar but use NumericVector here since it can carry the class
    // attribute for integer64

    if (m_fp == nullptr)
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

    return VSIFSeekL(m_fp, static_cast<vsi_l_offset>(offset_in),
                     origin_in);
}

Rcpp::NumericVector VSIFile::tell() const {
    if (m_fp == nullptr)
        Rcpp::stop("the file is not open");

    vsi_l_offset offset = VSIFTellL(m_fp);
    if (offset > VSI_L_OFFSET_MAX_R_)
        Rcpp::stop("the current file offset exceeds R integer64 upper limit");

    std::vector<int64_t> ret(1);
    ret[0] = static_cast<int64_t>(offset);
    return Rcpp::wrap(ret);
}

void VSIFile::rewind() {
    if (m_fp == nullptr)
        Rcpp::stop("the file is not open");

    VSIRewindL(m_fp);
    return;
}

SEXP VSIFile::read(Rcpp::NumericVector nbytes) {
    if (m_fp == nullptr)
        Rcpp::stop("the file is not open");

    if (nbytes.size() != 1)
        Rcpp::stop("'nbytes' must be a length-1 numeric vector");
    else if (nbytes[0] <= 0)
        return R_NilValue;

    size_t nbytes_in = 0;

    if (Rcpp::isInteger64(nbytes)) {
        nbytes_in = static_cast<size_t>(Rcpp::fromInteger64(nbytes[0]));
    }
    else {
        if (nbytes[0] > MAX_INT_AS_R_NUMERIC_)
            Rcpp::stop("'nbytes' given as type double is out of range");
        else
            nbytes_in = static_cast<size_t>(nbytes[0]);
    }

    if (nbytes_in == 0)
        return R_NilValue;

    GByte *buf = static_cast<GByte *>(CPLMalloc(nbytes_in));
    size_t nRead = 0;
    nRead = VSIFReadL(buf, 1, nbytes_in, m_fp);
    if (nRead == 0) {
        VSIFree(buf);
        return R_NilValue;
    }

    Rcpp::RawVector raw = Rcpp::no_init(nRead);
    std::memcpy(&raw[0], buf, nRead);
    VSIFree(buf);
    return raw;
}

Rcpp::NumericVector VSIFile::write(const Rcpp::RawVector& object) {
    if (m_fp == nullptr)
        Rcpp::stop("the file is not open");

    std::vector<int64_t> ret(1);
    ret[0] = static_cast<int64_t>(
        VSIFWriteL(&object[0], 1, static_cast<size_t>(object.size()), m_fp));

    return Rcpp::wrap(ret);
}

bool VSIFile::eof() const {
    if (m_fp == nullptr)
        Rcpp::stop("the file is not open");

    int eof = VSIFEofL(m_fp);
    if (eof == 0)
        return false;
    else
        return true;
}

int VSIFile::truncate(Rcpp::NumericVector new_size) {
    if (m_fp == nullptr)
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

    return VSIFTruncateL(m_fp, static_cast<vsi_l_offset>(new_size_in));
}

int VSIFile::flush() {
    if (m_fp == nullptr)
        Rcpp::stop("the file is not open");

    return VSIFFlushL(m_fp);
}

SEXP VSIFile::ingest(Rcpp::NumericVector max_size) {
    // max_size must be an R numeric vector of length 1
    // i.e., a scalar but use NumericVector here since it can carry the class
    // attribute for integer64

    if (m_fp == nullptr)
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

    int result = VSIIngestFile(m_fp, nullptr, &paby, &nSize, max_size_in);

    if (!result) {
        Rcpp::Rcout << "failed to ingest file\n";
        return R_NilValue;
    }

    Rcpp::RawVector raw = Rcpp::no_init(nSize);
    std::memcpy(&raw[0], paby, nSize);
    VSIFree(paby);
    return raw;
}

int VSIFile::close() {
    int ret = -1;
    if (m_fp != nullptr) {
        ret = VSIFCloseL(m_fp);
        if (ret == 0)
            m_fp = nullptr;
    }
    return ret;
}

std::string VSIFile::get_filename() const {
    return m_filename;
}

std::string VSIFile::get_access() const {
    return m_access;
}

int VSIFile::set_access(std::string access) {
    if (m_fp != nullptr) {
        Rcpp::Rcout << "cannot set access while the file is open\n";
        return -1;
    }

    if (access.length() > 0 && access.length() < 4) {
        m_access = access;
        return 0;
    }
    else {
        Rcpp::Rcout << "'access' should be 'r', 'r+', 'w' or 'w+'\n";
        return -1;
    }
}

void VSIFile::show() const {
    Rcpp::Rcout << "C++ object of class VSIFile\n";
    Rcpp::Rcout << " Filename : " << get_filename() << "\n";
    Rcpp::Rcout << " Access   : " << get_access() << "\n";
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
    .const_method("show", &VSIFile::show,
        "S4 show()")

    ;
}
