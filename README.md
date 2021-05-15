# Course work OpenCL
A program that solves a system of sparse linear equations by conjugate gradient method in OpenCL with client-server architecture

## Requeriments
This program will work only in Linux, mainly due to the fact that the client-server architecture is configured for this OS.

### Debian/Ubuntu
Install OpenCL headers:
``
sudo apt-get install OpenCL headers
``

Install OpenCL drivers according to your parallel computing device vendor:

 - Intel: `sudo apt-get install beignet-dev`
 - AMD: `sudo apt-get install mesa-opencl-icd`
 - Nvidia: `sudo apt-get install nvidia-opencl-dev`

## Usage

To execute the program you need to do the following steps:

```
mkdir build
cd build
cmake ..
make
```
Then the server and client directories will be created, which you need to go to and run executables.
