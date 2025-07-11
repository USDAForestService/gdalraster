% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/gdal_rat.R
\name{buildRAT}
\alias{buildRAT}
\title{Build a GDAL Raster Attribute Table with VALUE, COUNT}
\usage{
buildRAT(
  raster,
  band = 1L,
  col_names = c("VALUE", "COUNT"),
  table_type = "athematic",
  na_value = NULL,
  join_df = NULL,
  quiet = FALSE
)
}
\arguments{
\item{raster}{Either a \code{GDALRaster} object, or a character string containing
the file name of a raster dataset to open.}

\item{band}{Integer scalar, band number to read (default \code{1L}).}

\item{col_names}{Character vector of length two containing names to use for
column 1 (pixel values) and column 2 (pixel counts) in the output data
frame (defaults are \code{c("VALUE", "COUNT")}).}

\item{table_type}{Character string describing the type of the attribute
table. One of either \code{"thematic"}, or \code{"athematic"} for continuous data
(the default).}

\item{na_value}{Numeric scalar. If the set of unique pixel values has an
\code{NA}, it will be recoded to \code{na_value} in the returned data frame.
If \code{NULL} (the default), \code{NA} will not be recoded.}

\item{join_df}{Optional data frame for joining additional attributes. Must
have a column of unique values with the same name as \code{col_names[1]}
(\code{"VALUE"} by default).}

\item{quiet}{Logical scalar. If \verb{TRUE``, a progress bar will not be displayed. Defaults to }FALSE``.}
}
\value{
A data frame with at least two columns containing the set of unique
pixel values and their counts. These columns have attribute \code{"GFU"} set to
\code{"MinMax"} for the values, and \code{"PixelCount"} for the counts. If \code{join_df} is
given, the returned data frame will have additional columns that result from
\code{merge()}. The \code{"GFU"} attribute of the additional columns will be assigned
automatically based on the column names (\emph{case-insensitive} matching, see
Details). The returned data frame has attribute \code{"GDALRATTableType"} set to
\code{table_type}.
}
\description{
\code{buildRAT()} reads all pixels of an input raster to obtain the set of
unique values and their counts. The result is returned as a data frame
suitable for use with the class method \code{GDALRaster$setDefaultRAT()}. The
returned data frame might be further modified before setting as a Raster
Attribute Table in a dataset, for example, by adding columns containing
class names, color values, or other information (see Details).
An optional input data frame containing such attributes may be given, in
which case \code{buildRAT()} will attempt to join the additional columns and
automatically assign the appropriate metadata on the output data frame
(i.e., assign R attributes on the data frame and its columns that define
usage in a GDAL Raster Attribute Table).
}
\details{
A GDAL Raster Attribute Table (or RAT) provides attribute information about
pixel values. Raster attribute tables can be used to represent histograms,
color tables, and classification information. Each row in the table applies
to either a single pixel value or a range of values, and might have
attributes such as the histogram count for that value (or range), the color
that pixels of that value (or range) should be displayed, names of classes,
or various other information.

Each column in a raster attribute table has a name, a type (integer,
double, or string), and a \code{GDALRATFieldUsage}. The usage
distinguishes columns with particular understood purposes (such as color,
histogram count, class name), and columns that have other purposes not
understood by the library (long labels, ancillary attributes, etc).

In the general case, each row has a field indicating the minimum pixel
value falling into that category, and a field indicating the maximum pixel
value. In the GDAL API, these are indicated with usage values of \code{GFU_Min}
and \code{GFU_Max}. In the common case where each row is a discrete pixel value,
a single column with usage \code{GFU_MinMax} would be used instead.
In R, the table is represented as a data frame with column attribute \code{"GFU"}
containing the field usage as a string, e.g., \code{"Max"}, \code{"Min"} or \code{"MinMax"}
(case-sensitive).
The full set of possible field usage descriptors is:
\tabular{lll}{
GFU attr       \tab GDAL enum        \tab Description\cr
\code{"Generic"}    \tab \code{GFU_Generic}    \tab General purpose field\cr
\code{"PixelCount"} \tab \code{GFU_PixelCount} \tab Histogram pixel count\cr
\code{"Name"}       \tab \code{GFU_Name}       \tab Class name\cr
\code{"Min"}        \tab \code{GFU_Min}        \tab Class range minimum\cr
\code{"Max"}        \tab \code{GFU_Max}        \tab Class range maximum\cr
\code{"MinMax"}     \tab \code{GFU_MinMax}     \tab Class value (min=max)\cr
\code{"Red"}        \tab \code{GFU_Red}        \tab Red class color (0-255)\cr
\code{"Green"}      \tab \code{GFU_Green}      \tab Green class color (0-255)\cr
\code{"Blue"}       \tab \code{GFU_Blue}       \tab Blue class color (0-255)\cr
\code{"Alpha"}      \tab \code{GFU_Alpha}      \tab Alpha transparency (0-255)\cr
\code{"RedMin"}     \tab \code{GFU_RedMin}     \tab Color range red minimum\cr
\code{"GreenMin"}   \tab \code{GFU_GreenMin}   \tab Color range green minimum\cr
\code{"BlueMin"}    \tab \code{GFU_BlueMin}    \tab Color range blue minimum\cr
\code{"AlphaMin"}   \tab \code{GFU_AlphaMin}   \tab Color range alpha minimum\cr
\code{"RedMax"}     \tab \code{GFU_RedMax}     \tab Color range red maximum\cr
\code{"GreenMax"}   \tab \code{GFU_GreenMax}   \tab Color range green maximum\cr
\code{"BlueMax"}    \tab \code{GFU_BlueMax}    \tab Color range blue maximum\cr
\code{"AlphaMax"}   \tab \code{GFU_AlphaMax}   \tab Color range alpha maximum
}

\code{buildRAT()} assigns GFU \code{"MinMax"} on the column of pixel values (named
\code{"VALUE"} by default) and GFU \code{"PixelCount"} on the column of counts (named
\code{"COUNT"} by default).
If \code{join_df} is given, the additional columns that result from joining will
have GFU assigned automatically based on the column names (\emph{ignoring case}).
First, the additional column names are checked for containing
the string \code{"name"} (e.g., \code{"classname"}, \code{"TypeName"}, \code{"EVT_NAME"}, etc).
The first matching column (if any) will be assigned a GFU of \code{"Name"}
(=\code{GFU_Name}, the field usage descriptor for class names). Next, columns
named \code{"R"} or \code{"Red"} will be assigned GFU \code{"Red"}, columns named \code{"G"} or
\code{"Green"} will be assigned GFU \code{"Green"}, columns named \code{"B"} or \code{"Blue"}
will be assigned GFU \code{"Blue"}, and columns named \code{"A"} or \code{"Alpha"} will be
assigned GFU \code{"Alpha"}. Finally, any remaining columns that have not been
assigned a GFU will be assigned \code{"Generic"}.

In a variation of RAT, all the categories are of equal size and regularly
spaced, and the categorization can be determined by knowing the value at
which the categories start and the size of a category. This is called
"Linear Binning" and the information is kept specially on the raster
attribute table as a whole. In R, a RAT that uses linear binning would
have the following attributes set on the data frame:
attribute \code{"Row0Min"} = the numeric lower bound (pixel value) of the first
category, and attribute \code{"BinSize"} = the numeric width of each category (in
pixel value units). \code{buildRAT()} does not create tables with linear binning,
but one could be created manually based on the specifications above, and
applied to a raster with the class method \code{GDALRaster$setDefaultRAT()}.

A raster attribute table is thematic or athematic (continuous). In R, this
is defined by an attribute on the data frame named \code{"GDALRATTableType"} with
value of either \code{"thematic"} or \code{"athematic"}.
}
\note{
The full raster will be scanned.

If \code{na_value} is not specified, then an \code{NA} pixel value (if present)
will not be recoded in the output data frame. This may have implications
if joining to other data (\code{NA} will not match), or when using the returned
data frame to set a default RAT on a dataset (\code{NA} will be interpreted
as the value that R uses internally to represent it for the type, e.g.,
-2147483648 for \code{NA_integer_}). In some cases, removing the row in the output
data frame with value \code{NA}, rather than recoding, may be desirable (i.e., by
removing manually or by side effect of joining via \code{merge()}, for example).
Users should consider what is appropriate for a particular case.
}
\examples{
evt_file <- system.file("extdata/storml_evt.tif", package="gdalraster")
# make a copy to modify
f <- file.path(tempdir(), "storml_evt_tmp.tif")
file.copy(evt_file,  f)

ds <- new(GDALRaster, f, read_only=FALSE)
ds$getDefaultRAT(band = 1) # NULL

# get the full attribute table for LANDFIRE EVT from the CSV file
evt_csv <- system.file("extdata/LF20_EVT_220.csv", package="gdalraster")
evt_df <- read.csv(evt_csv)
nrow(evt_df)
head(evt_df)
evt_df <- evt_df[,1:7]

tbl <- buildRAT(ds,
                table_type = "thematic",
                na_value = -9999,
                join_df = evt_df)

nrow(tbl)
head(tbl)

# attributes on the data frame and its columns define usage in a GDAL RAT
attributes(tbl)
attributes(tbl$VALUE)
attributes(tbl$COUNT)
attributes(tbl$EVT_NAME)
attributes(tbl$EVT_LF)
attributes(tbl$EVT_PHYS)
attributes(tbl$R)
attributes(tbl$G)
attributes(tbl$B)

ds$setDefaultRAT(band = 1, tbl)
ds$flushCache()

tbl2 <- ds$getDefaultRAT(band = 1)
nrow(tbl2)
head(tbl2)

ds$close()
\dontshow{deleteDataset(f)}

# Display
evt_gt <- displayRAT(tbl2, title = "Storm Lake EVT Raster Attribute Table")
class(evt_gt)  # an object of class "gt_tbl" from package gt
# To show the table:
# evt_gt
# or simply call `displayRAT()` as above but without assignment
# `vignette("raster-attribute-tables")` has example output
}
\seealso{
\code{\link[=GDALRaster]{GDALRaster$getDefaultRAT()}},
\code{\link[=GDALRaster]{GDALRaster$setDefaultRAT()}},
\code{\link[=displayRAT]{displayRAT()}}

\code{vignette("raster-attribute-tables")}
}
