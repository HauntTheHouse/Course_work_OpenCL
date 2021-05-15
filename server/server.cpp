#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <ctime>

#define CL_HPP_TARGET_OPENCL_VERSION 210
#include <CL/cl2.hpp>

#include <TCPSocket.h>

int main()
{
    std::shared_ptr<TCPSocket> serverSocket;
    std::vector<std::unique_ptr<TCPSocket>> connects;

    try
    {
        serverSocket = std::make_unique<TCPSocket>(AF_INET, SOCK_STREAM);
        serverSocket->setSocketAddress(AF_INET, htonl(INADDR_ANY), htons(8080));
        serverSocket->bindSocket();
        serverSocket->listenForClients(512);

        connects.push_back(std::make_unique<TCPSocket>());
        connects.front()->acceptSocket(serverSocket.get());
    }
    catch(const std::invalid_argument& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    int matrixDimension, numberOfValues;
    connects.front()->receiveMessage(matrixDimension, sizeof(int));
    connects.front()->receiveMessage(numberOfValues, sizeof(int));

    std::vector<int> rowIds(numberOfValues);
    std::vector<int> colIds(numberOfValues);
    std::vector<float> values(numberOfValues);
    std::vector<float> b(matrixDimension);
    connects.front()->receiveMessage(*rowIds.data(), numberOfValues * sizeof(int));
    connects.front()->receiveMessage(*colIds.data(), numberOfValues * sizeof(int));
    connects.front()->receiveMessage(*values.data(), numberOfValues * sizeof(float));
    connects.front()->receiveMessage(*b.data(), matrixDimension * sizeof(float));

//    std::cout << matrixDimension << '\t' << matrixDimension << '\t' << numberOfValues << std::endl;
//    for (int i = 0; i < numberOfValues; ++i)
//    {
//        std::cout << rowIds[i] << '\t' << colIds[i] << '\t' << values[i] << std::endl;
//    }

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
    std::cout << "PLATFORMS:" << std::endl;
    for (auto& platform : platforms)
    {
        std::cout << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

        std::cout << "\tDEVICES:" << std::endl;
        for (auto& dev : devices)
        {
            std::cout << '\t' << dev.getInfo<CL_DEVICE_NAME>() << std::endl;
            std::cout << "\tMAX_WORK_GROUP_SIZE: " << dev.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
            for (auto& itemSize : dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>())
            {
                std::cout << "\t\tMAX_ITEM_SIZE: " << itemSize << std::endl;
            }
            std::cout << "\tMAX_WORK_ITEM_DIMENSIONS: " << dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>() << std::endl;
            std::cout << "\tMAX_COMPUTE_UNITS: " << dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;

//            std::cout << "\tMAX_GLOBAL_VARIABLE_SIZE: " << dev.getInfo<CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE>() << std::endl;

        }
    }

    auto platform = platforms.back();
//    auto platform = platforms.front();

    std::cout << std::endl << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    device = devices.back();
    std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
//    std::ifstream kernel_file("src/steepestDescent.cl");
    std::ifstream kernel_file("conjugateGradient.cl");
    std::stringstream srcStream;
    srcStream << kernel_file.rdbuf();
    std::string src = srcStream.str();

//    cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));
    cl::Program::Sources sources;
    sources.push_back(src);

    context = cl::Context(device);
    program = cl::Program(context, sources);

    auto err = program.build();
    if (err != CL_BUILD_SUCCESS)
    {
        std::cerr << "Error!\nBuild Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device)
                  << "\nBuild Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
        exit(1);
    }

    std::vector<float> x(matrixDimension);
    std::vector<float> result(2);

    cl::Buffer rowsBuf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, numberOfValues * sizeof(int), rowIds.data());
    cl::Buffer colsBuf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, numberOfValues * sizeof(int), colIds.data());
    cl::Buffer valuesBuf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, numberOfValues * sizeof(float), values.data());
    cl::Buffer bBuf(context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR, matrixDimension * sizeof(float), b.data());

    cl::Buffer xBuf(context, CL_MEM_READ_WRITE, matrixDimension * sizeof(float));
    cl::Buffer resultBuf(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, 2 * sizeof(float));

    cl::Kernel kernel(program, "conjugateGradient");
    std::cout << "KERNEL_WORK_GROUP_SIZE: " << kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device) << std::endl;
    kernel.setArg(0, sizeof(int), &matrixDimension);
    kernel.setArg(1, sizeof(int), &numberOfValues);
    kernel.setArg(2, cl::Local(matrixDimension * sizeof(float)));
    kernel.setArg(3, xBuf);
//    kernel.setArg(3, cl::Local(dimension * sizeof(float)));
    kernel.setArg(4, cl::Local(matrixDimension * sizeof(float)));
    kernel.setArg(5, cl::Local(matrixDimension * sizeof(float)));
    kernel.setArg(6, rowsBuf);
    kernel.setArg(7, colsBuf);
    kernel.setArg(8, valuesBuf);
    kernel.setArg(9, bBuf);
    kernel.setArg(10, resultBuf);


    cl::CommandQueue queue(context, device);
    queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(matrixDimension), cl::NDRange(matrixDimension));
    queue.enqueueReadBuffer(xBuf, CL_TRUE, 0, matrixDimension * sizeof(float), x.data());
    queue.enqueueReadBuffer(resultBuf, CL_TRUE, 0, 2 * sizeof(float), result.data());

    connects.front()->sendMessage(*x.data(), matrixDimension * sizeof(float));
    std::cout << "Iterations: " << static_cast<int>(result[0]) << std::endl;
    std::cout << "Residual length: " << result[1] << std::endl;
}