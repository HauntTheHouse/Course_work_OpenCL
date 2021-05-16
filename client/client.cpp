#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
//#include <string>

#include <TCPSocket.h>
#include "SparseMatrix.h"

void enterValue(int &choose, int leftLimit, int rightLimit);

int main()
{
    std::unique_ptr<TCPSocket> clientSocket;
    std::unique_ptr<SparseMatrix> sparseMatrix;
    try
    {
        clientSocket = std::make_unique<TCPSocket>(AF_INET, SOCK_STREAM);
        clientSocket->setSocketAddress(AF_INET, inet_addr("127.0.0.1"), htons(8080));
        clientSocket->connectToServer();
//        sparseMatrix = std::make_unique<SparseMatrix>("data/pll_post_short_A.txt");
        sparseMatrix = std::make_unique<SparseMatrix>("data/sparse_matrix.txt");
//        sparseMatrix = std::make_unique<SparseMatrix>("data/bcsstk01.txt");
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    std::cout << "Available platforms:\n";
    int countOfPlatforms;
    clientSocket->receiveMessage(countOfPlatforms, sizeof(int));
    for (int i = 0; i < countOfPlatforms; ++i)
    {
        int buffSize;
        std::string platforms;
        clientSocket->receiveMessage(buffSize, sizeof(int));
        platforms.resize(buffSize);
        clientSocket->receiveMessage(*&platforms[0], buffSize);
        std::cout << platforms << std::endl;
    }

    int choose;
    std::cout << "Choose platform: " << std::endl;
    enterValue(choose, 1, countOfPlatforms);
    clientSocket->sendMessage(choose, sizeof(int));

    clientSocket->sendMessage(sparseMatrix->getMatrixDimension(), sizeof(int));
    clientSocket->sendMessage(sparseMatrix->getNumberOfValues(), sizeof(int));

    clientSocket->sendMessage(*sparseMatrix->getRowIds(), sparseMatrix->getNumberOfValues() * sizeof(int));
    clientSocket->sendMessage(*sparseMatrix->getColIds(), sparseMatrix->getNumberOfValues() * sizeof(int));
    clientSocket->sendMessage(*sparseMatrix->getValues(), sparseMatrix->getNumberOfValues() * sizeof(float));

    std::vector<float> b(sparseMatrix->getMatrixDimension());
    std::generate(b.begin(), b.end(), []{ return 250.0f; });
    clientSocket->sendMessage(*b.data(), sparseMatrix->getNumberOfValues() * sizeof(float));

    std::vector<float> x(sparseMatrix->getMatrixDimension());
    clientSocket->receiveMessage(*x.data(), sparseMatrix->getNumberOfValues() * sizeof(float));

    std::cout << "x: ";
    for (auto value : x)
    {
        std::cout << "{" << value << "}, ";
    }
    std::cout << std::endl;
}

void enterValue(int &choose, int leftLimit, int rightLimit)
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