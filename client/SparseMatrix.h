#ifndef COURSE_WORK_SOLVE_SYSTEM_OF_SPARSE_LINEAR_EQUATIONS_SPARSEMATRIX_H
#define COURSE_WORK_SOLVE_SYSTEM_OF_SPARSE_LINEAR_EQUATIONS_SPARSEMATRIX_H

#include <string>
#include <fstream>
#include <vector>

class SparseMatrix
{
private:
    int numRows, numCols, numValues;

    std::vector<int> rowIds, colIds;
    std::vector<double> values;
    std::vector<double> b;

    std::ifstream matrix;
public:
    SparseMatrix() = default;
    explicit SparseMatrix(const std::string& pathToMatrix);
    ~SparseMatrix();
    void open(const std::string& pathToMatrix);

    int getMatrixDimension() const { return numRows; }
    int getNumberOfValues() const { return numValues; }
    int* getRowIds() { return rowIds.data(); }
    int* getColIds() { return colIds.data(); }
    double* getValues() { return values.data(); }
    double* getVectorB() { return b.data(); }

    void fillVectorBWithRandomValues(double minValue, double maxValue);
    void fillVectorBFullyWithConcreteValue(double value);
};

#endif //COURSE_WORK_SOLVE_SYSTEM_OF_SPARSE_LINEAR_EQUATIONS_SPARSEMATRIX_H