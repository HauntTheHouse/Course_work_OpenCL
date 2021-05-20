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

int main() {
    std::shared_ptr<TCPSocket> serverSocket;
    std::vector<std::unique_ptr<TCPSocket>> connects;

    try {
        serverSocket = std::make_unique<TCPSocket>(AF_INET, SOCK_STREAM);
        serverSocket->setSocketAddress(AF_INET, htonl(INADDR_ANY), htons(8080));
        serverSocket->bindSocket();
        serverSocket->listenForClients(512);

        connects.push_back(std::make_unique<TCPSocket>());
        connects.front()->acceptSocket(serverSocket.get());
    }
    catch (const std::invalid_argument &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    while (true)
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

        connects.front()->sendMessage(platforms.size(), sizeof(int)); //1
        for (int i = 0; i < platforms.size(); ++i)
        {
            std::stringstream platformsOutStream;
            platformsOutStream << i + 1 << ". " << platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;

            std::vector<cl::Device> devices;
            platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);

            platformsOutStream << "\tDevices:" << std::endl;
            for (auto &dev : devices) {
                platformsOutStream << '\t' << dev.getInfo<CL_DEVICE_NAME>() << std::endl;
                platformsOutStream << "\tMAX_WORK_GROUP_SIZE: " << dev.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
                for (auto &itemSize : dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()) {
                    platformsOutStream << "\t\tMAX_ITEM_SIZE: " << itemSize << std::endl;
                }
                platformsOutStream << "\tMAX_WORK_ITEM_DIMENSIONS: " << dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>() << std::endl;
                platformsOutStream << "\tMAX_COMPUTE_UNITS: " << dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
            }
            connects.front()->sendMessage(platformsOutStream.str().size(), sizeof(int)); //2
            connects.front()->sendMessage(*&platformsOutStream.str()[0], platformsOutStream.str().size()); //3
        }

        int choose;
        connects.front()->receiveMessage(choose, sizeof(int)); //4
        auto platform = platforms[choose - 1];
        std::cout << std::endl << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
        device = devices.back();
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

        int matrixDimension, numberOfValues;
        connects.front()->receiveMessage(matrixDimension, sizeof(int)); //5
        connects.front()->receiveMessage(numberOfValues, sizeof(int)); //6

        std::vector<int> rowIds(numberOfValues);
        std::vector<int> colIds(numberOfValues);
        std::vector<float> values(numberOfValues);
        std::vector<float> b(matrixDimension);
        connects.front()->receiveMessage(*rowIds.data(), numberOfValues * sizeof(int)); //7
        connects.front()->receiveMessage(*colIds.data(), numberOfValues * sizeof(int)); //8
        connects.front()->receiveMessage(*values.data(), numberOfValues * sizeof(float)); //9
        connects.front()->receiveMessage(*b.data(), matrixDimension * sizeof(float)); //10

        std::cout << "Algorithm that solves linear equation: conjugate gradient" << std::endl;
//    std::ifstream kernel_file("kernels/steepestDescent.cl");
        std::ifstream kernel_file("kernels/conjugateGradient.cl");
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
                      << "\nBuild Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
            exit(1);
        }

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

        std::vector<float> x(matrixDimension);
        std::vector<float> result(2);

        cl::CommandQueue queue(context, device);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(matrixDimension), cl::NDRange(matrixDimension));
        queue.enqueueReadBuffer(xBuf, CL_TRUE, 0, x.size() * sizeof(float), x.data());
        queue.enqueueReadBuffer(resultBuf, CL_TRUE, 0, result.size() * sizeof(float), result.data());

        std::cout << "x: ";
        for (auto value : x)
            std::cout << "{" << value << "}, ";
        std::cout << std::endl << std::endl;

        std::cout << "Iterations: " << static_cast<int>(result[0]) << std::endl;
        std::cout << "Residual length: " << result[1] << std::endl;

        connects.front()->sendMessage(*x.data(), x.size() * sizeof(float));
        connects.front()->sendMessage(*result.data(), result.size() * sizeof(float));
    }
}