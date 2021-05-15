#include "SparseMatrix.h"


SparseMatrix::SparseMatrix(const std::string& pathToMatrix)
: numRows(0), numCols(0), numValues(0)
{
    open(pathToMatrix);
}

SparseMatrix::~SparseMatrix()
{
    matrix.close();
}

void SparseMatrix::open(const std::string &pathToMatrix)
{
    rowIds.clear();
    colIds.clear();
    values.clear();

    matrix.open(pathToMatrix);
    if (!matrix.is_open())
    {
        throw std::invalid_argument("This file doesn't exist");
    }
    matrix >> numRows >> numCols >> numValues;
    rowIds.resize(numValues);
    colIds.resize(numValues);
    values.resize(numValues);

    for (int i = 0; !matrix.eof(); ++i)
    {
        matrix >> rowIds[i] >> colIds[i] >> values[i];
        rowIds[i]--; colIds[i]--;
    }
}
