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
    std::vector<float> values;
    std::vector<float> b;

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
    float* getValues() { return values.data(); }
    float* getVectorB() { return b.data(); }

    void fillVectorBWithRandomValues(float minValue, float maxValue);
    void fillVectorBFullyWithConcreteValue(float value);
};

#endif //COURSE_WORK_SOLVE_SYSTEM_OF_SPARSE_LINEAR_EQUATIONS_SPARSEMATRIX_H