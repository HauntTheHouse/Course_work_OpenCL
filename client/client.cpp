#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <limits>

#include <TCPSocket.h>
#include "SparseMatrix.h"

template<typename T>
void enterValue(T &choose, T leftLimit, T rightLimit);
void enterValue(std::string &choose);

int main()
{
    std::unique_ptr<SparseMatrix> sparseMatrix;

    system("clear");
    int chooseMenu = 0;
    while (chooseMenu != 5)
    {
        std::cout << "\nChoose what you want to do." << std::endl;
        std::cout << "1. Load sparse matrix from file" << std::endl;
        std::cout << "2. Fill vector b" << std::endl;
        std::cout << "3. Print data about matrix" << std::endl;
        std::cout << "4. Solve linear equation and find vector x" << std::endl;
        std::cout << "5. Exit" << std::endl << std::endl;

        enterValue(chooseMenu, 1, 5);
        system("clear");

        switch (chooseMenu)
        {
            case 1:
            {
                std::cout << "Which file you want to open (example \"data/sparse_matrix_153.txt\")" << std::endl;
                std::string matrixFile;
                enterValue(matrixFile);
                std::cout << std::endl << "Read matrix from " << matrixFile << std::endl;
                try
                {
                    sparseMatrix = std::make_unique<SparseMatrix>(matrixFile);
                }
                catch (const std::invalid_argument &e)
                {
                    std::cerr << e.what() << std::endl;
                    return -2;
                }
                std::cout << "Matrix successfully loaded!" << std::endl;
                std::cout << "Vector b filled with values 0.0f" << std::endl;
                std::cout << "Matrix dimension: " << sparseMatrix->getMatrixDimension() << std::endl;
                std::cout << "Count of not null values: " << sparseMatrix->getNumberOfValues() << std::endl;
                break;
            }
            case 2:
            {
                if (sparseMatrix == nullptr)
                {
                    std::cout << "You can't fill vector b while matrix doesn't exists!" << std::endl;
                    break;
                }
                std::cout << "How you want to fill vector b." << std::endl;
                std::cout << "1. With random values" << std::endl;
                std::cout << "2. With concrete values" << std::endl;
                int chooseFillVector;
                enterValue(chooseFillVector, 1, 2);
                switch (chooseFillVector)
                {
                    case 1:
                        float minValue, maxValue;
                        std::cout << "From: ";
                        enterValue(minValue, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
                        std::cout << "To: ";
                        enterValue(maxValue, minValue, std::numeric_limits<float>::infinity());
                        sparseMatrix->fillVectorBWithRandomValues(minValue, maxValue);
                        break;
                    case 2:
                        float value;
                        std::cout << "Enter value: ";
                        enterValue(value, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
                        sparseMatrix->fillVectorBFullyWithConcreteValue(value);
                        break;
                    default:
                        break;
                }
                break;
            }
            case 3:
                if (sparseMatrix == nullptr)
                {
                    std::cout << "You can't print something that doesn't exist!" << std::endl;
                    break;
                }
                for (int i = 0; i < sparseMatrix->getNumberOfValues(); ++i)
                {
                    std::cout << sparseMatrix->getRowIds()[i] << "\t" << sparseMatrix->getColIds()[i] << "\t" << sparseMatrix->getValues()[i] << std::endl;
                }
                break;
            case 4: {
                if (sparseMatrix == nullptr) {
                    std::cout << "You can't solve linear equation without linear equation!!!" << std::endl;
                    break;
                }

                std::unique_ptr<TCPSocket> clientSocket;
                try {
                    clientSocket = std::make_unique<TCPSocket>(AF_INET, SOCK_STREAM);
                    clientSocket->setSocketAddress(AF_INET, inet_addr("127.0.0.1"), htons(8080));
                    clientSocket->connectToServer();
                }
                catch (const std::invalid_argument &e) {
                    std::cerr << e.what() << std::endl;
                    return -1;
                }

                int countOfPlatforms;
                clientSocket->receiveMessage(countOfPlatforms, sizeof(int));
                std::cout << "Available platforms:" << std::endl;
                for (int i = 0; i < countOfPlatforms; ++i) {
                    int buffSize;
                    clientSocket->receiveMessage(buffSize, sizeof(int));
                    std::string platforms;
                    platforms.resize(buffSize);
                    clientSocket->receiveMessage(*&platforms[0], buffSize);
                    std::cout << platforms << std::endl;
                }

                std::cout << "Choose platform: ";
                enterValue(chooseMenu, 1, countOfPlatforms);
                std::cout << std::endl;
                clientSocket->sendMessage(chooseMenu, sizeof(int));

                clientSocket->sendMessage(sparseMatrix->getMatrixDimension(), sizeof(int));
                clientSocket->sendMessage(sparseMatrix->getNumberOfValues(), sizeof(int));

                clientSocket->sendMessage(*sparseMatrix->getRowIds(), sparseMatrix->getNumberOfValues() * sizeof(int));
                clientSocket->sendMessage(*sparseMatrix->getColIds(), sparseMatrix->getNumberOfValues() * sizeof(int));
                clientSocket->sendMessage(*sparseMatrix->getValues(), sparseMatrix->getNumberOfValues() * sizeof(float));

                clientSocket->sendMessage(*sparseMatrix->getVectorB(), sparseMatrix->getMatrixDimension() * sizeof(float));

                std::vector<float> x(sparseMatrix->getMatrixDimension());
                std::vector<float> result(2);

                int kernelWorkGroupSize;
                clientSocket->receiveMessage(kernelWorkGroupSize, sizeof(int));
                if (sparseMatrix->getMatrixDimension() > kernelWorkGroupSize)
                {
                    std::cout << "Matrix dimension exceeds kernel work group size" << std::endl;
                    break;
                }

                clientSocket->receiveMessage(*x.data(), x.size() * sizeof(float));
                clientSocket->receiveMessage(*result.data(), result.size() * sizeof(float));

                std::cout << "x: ";
                for (auto value : x)
                    std::cout << "{" << value << "}, ";
                std::cout << std::endl << std::endl;
                std::cout << "Iterations: " << static_cast<int>(result[0]) << std::endl;
                std::cout << "Residual length: " << result[1] << std::endl;
                break;
            }
            default:
                break;
        }
    }
}

template<typename T>
void enterValue(T &choose, T leftLimit, T rightLimit)
{
    while (true)
    {
        std::cin >> choose;
        std::cin.clear();
        std::cin.ignore();
        if (choose < leftLimit || choose > rightLimit)
        {
            std::cout << "You enter incorrect value, try again..." << std::endl;
        }
        else
        {
            break;
        }
    }
}

void enterValue(std::string &choose)
{
    std::cin >> choose;
    std::cin.clear();
    std::cin.ignore();
}