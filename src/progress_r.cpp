/* GDALTermProgress() works in R on Linux, but not on Windows.
   This was copied from /gdal/port/cpl_progress.cpp and modified to use
   Rprintf() and works on Linux and Windows R. -CT 2023-04-30
*/

 /******************************************************************************
 * Copyright (c) 2013, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <cmath>
#include "gdalraster.h"

/************************************************************************/
/*                          GDALTermProgressR()                          */
/************************************************************************/

/**
 * \fn GDALTermProgressR(double, const char*, void*)
 * \brief Simple progress report to terminal (**modified for R**)
 *
 * This progress reporter prints simple progress report to the
 * terminal window.  The progress report generally looks something like
 * this:

\verbatim
0...10...20...30...40...50...60...70...80...90...100 - done.
\endverbatim

 * Every 2.5% of progress another number or period is emitted.  Note that
 * GDALTermProgress() uses internal static data to keep track of the last
 * percentage reported and will get confused if two terminal based progress
 * reportings are active at the same time.
 *
 * The GDALTermProgress() function maintains an internal memory of the
 * last percentage complete reported in a static variable, and this makes
 * it unsuitable to have multiple GDALTermProgress()'s active either in a
 * single thread or across multiple threads.
 *
 * @param dfComplete completion ratio from 0.0 to 1.0.
 * @param pszMessage optional message.
 * @param pProgressArg ignored callback data argument.
 *
 * @return Always returns TRUE indicating the process should continue.
 */

int CPL_STDCALL GDALTermProgressR(double dfComplete,
                                 CPL_UNUSED const char *pszMessage,
                                 CPL_UNUSED void *pProgressArg)
{
    const int nThisTick =
        std::min(40, std::max(0, static_cast<int>(dfComplete * 40.0)));

    // Have we started a new progress run?
    static int nLastTick = -1;
    if (nThisTick < nLastTick && nLastTick >= 39)
        nLastTick = -1;

    if (nThisTick <= nLastTick)
        return TRUE;

    while (nThisTick > nLastTick)
    {
        ++nLastTick;
        if (nLastTick % 4 == 0)
            Rprintf("%d", (nLastTick / 4) * 10);
        else
            Rprintf(".");
    }

    if (nThisTick == 40)
        Rprintf(" - done.\n");

    return TRUE;
}
