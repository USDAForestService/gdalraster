# Class for counting unique combinations of integers

`CmbTable` implements a hash table having a vector of integers as the
key, and the count of occurrences of each unique integer combination as
the value. A unique ID is assigned to each unique combination of input
values.

`CmbTable` is a C++ class exposed directly to R (via
`RCPP_EXPOSED_CLASS`). Methods of the class are accessed using the `$`
operator. **Note that all arguments to class methods are required and
must be given in the order documented.** Naming the arguments is
optional but may be preferred for readability.

## Arguments

- keyLen:

  The number of integer values comprising each combination.

- varNames:

  Optional character vector of names for the variables in the
  combination.

## Value

An object of class `CmbTable`. Contains a hash table having a vector of
`keyLen` integers as the key, and the count of occurrences of each
unique integer combination as the value. Class methods that operate on
the hash table are described in Details.

## Usage (see Details)

    ## Constructors
    cmb <- new(CmbTable, keyLen)
    # or, giving the variable names:
    cmb <- new(CmbTable, keyLen, varNames)

    ## Methods
    cmb$update(int_cmb, incr)
    cmb$updateFromMatrix(int_cmbs, incr)
    cmb$updateFromMatrixByRow(int_cmbs, incr)
    cmb$asDataFrame()
    cmb$asMatrix()

## Details

### Constructors

`new(CmbTable, keyLen)`  
Default variable names will be assigned as `V1`, `V2`, .... Returns an
object of class `CmbTable`.

`new(CmbTable, keyLen, varNames)`  
Alternate constructor to specify variable names. Returns an object of
class `CmbTable`.

### Methods

`$update(int_cmb, incr)`  
Updates the hash table for the integer combination in the numeric vector
`int_cmb` (will be coerced to integer by truncation). If this
combination exists in the table, its count will be incremented by
`incr`. If the combination is not found in the table, it will be
inserted with count set to `incr`. The caller should ensure that the
length of the input vector is equal to the key length (`keyLen`) when
using this method. Returns the unique ID assigned to this combination.
Combination IDs are sequential whole numbers starting at `1`.

`$updateFromMatrix(int_cmbs, incr)`  
This method is the same as `$update()` but for a numeric matrix of
integer combinations `int_cmbs` (coerced to integer by truncation). The
matrix is arranged with each column vector forming an integer
combination. For example, the rows of the matrix could be one row each
from a set of `keyLen` rasters all read at the same extent and pixel
resolution (i.e., row-by-row raster overlay). The method calls
`$update()` on each combination (each column of `int_cmbs`),
incrementing count by `incr` for existing combinations, or inserting new
combinations with count set to `incr`. Returns a numeric vector of
length `ncol(int_cmbs)` containing the IDs assigned to the combinations.

`$updateFromMatrixByRow(int_cmbs, incr)`  
This method is the same as `$updateFromMatrix()` above except the
integer combinations are in rows of the matrix `int_cmbs` (columns are
the variables). The method calls `$update()` on each combination (each
row of `int_cmbs`), incrementing count by `incr` for existing
combinations, or inserting new combinations with count set to `incr`.
Returns a numeric vector of length `nrow(int_cmbs)` containing the IDs
assigned to the combinations.

`$asDataFrame()`  
Returns the `CmbTable` as a data frame with column `cmbid` containing
the unique combination IDs, column `count` containing the counts of
occurrences, and `keyLen` columns (with names from `varNames`)
containing the integer values comprising each unique combination.

`$asMatrix()`  
Returns the `CmbTable` as a matrix with column `1` (`cmbid`) containing
the unique combination IDs, column `2` (`count`) containing the counts
of occurrences, and columns `3:keyLen+2` (with names from `varNames`)
containing the integer values comprising each unique combination.

## Examples

``` r
m <- matrix(c(1,2,3,1,2,3,4,5,6,1,3,2,4,5,6,1,1,1), 3, 6, byrow = FALSE)
rownames(m) <- c("layer1", "layer2", "layer3")
print(m)
#>        [,1] [,2] [,3] [,4] [,5] [,6]
#> layer1    1    1    4    1    4    1
#> layer2    2    2    5    3    5    1
#> layer3    3    3    6    2    6    1

cmb <- new(CmbTable, 3, rownames(m))
cmb
#> C++ object of class CmbTable
#>  Columns: cmbid count layer1 layer2 layer3

cmb$updateFromMatrix(m, 1)
#> [1] 1 1 2 3 2 4
cmb$asDataFrame()
#>   cmbid count layer1 layer2 layer3
#> 1     4     1      1      1      1
#> 2     3     1      1      3      2
#> 3     2     2      4      5      6
#> 4     1     2      1      2      3

cmb$update(c(4,5,6), 1)
#> [1] 2
cmb$update(c(1,3,5), 1)
#> [1] 5
cmb$asDataFrame()
#>   cmbid count layer1 layer2 layer3
#> 1     5     1      1      3      5
#> 2     4     1      1      1      1
#> 3     3     1      1      3      2
#> 4     2     3      4      5      6
#> 5     1     2      1      2      3

# same as above but matrix arranged with integer combinations in the rows
m <- matrix(c(1,2,3,1,2,3,4,5,6,1,3,2,4,5,6,1,1,1), 6, 3, byrow = TRUE)
colnames(m) <- c("V1", "V2", "V3")
print(m)
#>      V1 V2 V3
#> [1,]  1  2  3
#> [2,]  1  2  3
#> [3,]  4  5  6
#> [4,]  1  3  2
#> [5,]  4  5  6
#> [6,]  1  1  1

cmb <- new(CmbTable, 3)
cmb$updateFromMatrixByRow(m, 1)
#> [1] 1 1 2 3 2 4
cmb$asDataFrame()
#>   cmbid count V1 V2 V3
#> 1     4     1  1  1  1
#> 2     3     1  1  3  2
#> 3     2     2  4  5  6
#> 4     1     2  1  2  3

cmb$update(c(4,5,6), 1)
#> [1] 2
cmb$update(c(1,3,5), 1)
#> [1] 5
cmb$asDataFrame()
#>   cmbid count V1 V2 V3
#> 1     5     1  1  3  5
#> 2     4     1  1  1  1
#> 3     3     1  1  3  2
#> 4     2     3  4  5  6
#> 5     1     2  1  2  3
```
