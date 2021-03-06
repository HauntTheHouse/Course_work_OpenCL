#define CL_HPP_TARGET_OPENCL_VERSION 210
#include <CL/cl2.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <ctime>

#include <TCPSocket.h>

void solveLinearEquation(TCPSocket* connect);

int main()
{
    std::shared_ptr<TCPSocket> serverSocket;
    std::shared_ptr<TCPSocket> connect;
    try
    {
        serverSocket = std::make_unique<TCPSocket>(AF_INET, SOCK_STREAM);
        serverSocket->setSocketAddress(AF_INET, htonl(INADDR_ANY), htons(8080));
        serverSocket->bindSocket();
        serverSocket->listenForClients(16);
        while (true)
        {
            connect = std::make_unique<TCPSocket>();
            connect->acceptSocket(serverSocket.get());
            solveLinearEquation(connect.get());
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}

void solveLinearEquation(TCPSocket* connect)
{
    cl::Program program;
    cl::Context context;
    cl::Device device;

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.empty())
    {
        std::cerr << "No platforms found!" << std::endl;
        exit(1);
    }

    connect->sendMessage(platforms.size(), sizeof(int));
    for (int i = 0; i < platforms.size(); ++i)
    {
        std::stringstream platformsOutStream;
        platformsOutStream << i + 1 << ". " << platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;

        std::vector<cl::Device> devices;
        platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);

        platformsOutStream << "\tDevices:" << std::endl;
        for (auto &dev : devices)
        {
            platformsOutStream << '\t' << dev.getInfo<CL_DEVICE_NAME>() << std::endl;
            platformsOutStream << "\tMAX_WORK_GROUP_SIZE: " << dev.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
            for (auto &itemSize : dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>())
            {
                platformsOutStream << "\t\tMAX_ITEM_SIZE: " << itemSize << std::endl;
            }
            platformsOutStream << "\tMAX_WORK_ITEM_DIMENSIONS: " << dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>() << std::endl;
            platformsOutStream << "\tMAX_COMPUTE_UNITS: " << dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
        }
        connect->sendMessage(platformsOutStream.str().size(), sizeof(int));
        connect->sendMessage(*&platformsOutStream.str()[0], platformsOutStream.str().size());
    }

    int choose;
    connect->receiveMessage(choose, sizeof(int));
    auto platform = platforms[choose - 1];
    std::cout << std::endl << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    device = devices.back();
    std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

    int matrixDimension, numberOfValues;
    connect->receiveMessage(matrixDimension, sizeof(int));
    connect->receiveMessage(numberOfValues, sizeof(int));

    std::vector<int> rowIds(numberOfValues);
    std::vector<int> colIds(numberOfValues);
    std::vector<double> values(numberOfValues);
    std::vector<double> b(matrixDimension);
    connect->receiveMessage(*values.data(), numberOfValues * sizeof(double));
    connect->receiveMessage(*rowIds.data(), numberOfValues * sizeof(int));
    connect->receiveMessage(*colIds.data(), numberOfValues * sizeof(int));
    connect->receiveMessage(*b.data(), matrixDimension * sizeof(double));

    std::string method = "conjugateGradient";
//    std::string method = "steepestDescent";
    std::cout << "Algorithm that solves linear equation: " << method << std::endl;
    std::ifstream kernel_file("kernels/" + method + ".cl");
    std::stringstream srcStream;
    srcStream << kernel_file.rdbuf();
    std::string src = srcStream.str();

    cl::Program::Sources sources;
    sources.push_back(src);

    context = cl::Context(device);
    program = cl::Program(context, sources);

    auto err = program.build();
    if (err != CL_BUILD_SUCCESS)
    {
        std::cerr << "Error!\nBuild Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device)
                  << "\nBuild Log:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
        exit(1);
    }

    std::vector<double> x(matrixDimension);
    std::vector<double> result(2);
    clock_t start = clock();
    {
        cl::Buffer rowsBuf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, numberOfValues * sizeof(int), rowIds.data());
        cl::Buffer colsBuf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, numberOfValues * sizeof(int), colIds.data());
        cl::Buffer valuesBuf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, numberOfValues * sizeof(double), values.data());
        cl::Buffer bBuf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, matrixDimension * sizeof(double), b.data());
        cl::Buffer xBuf(context, CL_MEM_READ_WRITE, matrixDimension * sizeof(double));
        cl::Buffer resultBuf(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, 2 * sizeof(double));

        cl::Kernel kernel(program, "conjugateGradient");
//        cl::Kernel kernel(program, "steepestDescent");

        int kernelWorkGroupSize = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
        std::cout << "KERNEL_WORK_GROUP_SIZE: " << kernelWorkGroupSize << std::endl;
        connect->sendMessage(kernelWorkGroupSize, sizeof(int));
        if (matrixDimension > kernelWorkGroupSize)
            return;

        kernel.setArg(0, sizeof(int), &matrixDimension);
        kernel.setArg(1, sizeof(int), &numberOfValues);
        kernel.setArg(2, cl::Local(matrixDimension * sizeof(double)));
        kernel.setArg(3, xBuf);
        kernel.setArg(4, cl::Local(matrixDimension * sizeof(double)));
        kernel.setArg(5, cl::Local(matrixDimension * sizeof(double)));
        kernel.setArg(6, rowsBuf);
        kernel.setArg(7, colsBuf);
        kernel.setArg(8, valuesBuf);
        kernel.setArg(9, bBuf);
        kernel.setArg(10, resultBuf);

        cl::CommandQueue queue(context, device);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(matrixDimension), cl::NDRange(matrixDimension));
        queue.enqueueReadBuffer(xBuf, CL_TRUE, 0, x.size() * sizeof(double), x.data());
        queue.enqueueReadBuffer(resultBuf, CL_TRUE, 0, result.size() * sizeof(double), result.data());
    }
    clock_t end = clock();
    double computeTime = (1000.0 * (end - start)) / CLOCKS_PER_SEC;

    connect->sendMessage(*x.data(), x.size() * sizeof(double));
    connect->sendMessage(*result.data(), result.size() * sizeof(double));
    connect->sendMessage(computeTime, sizeof(double));
}