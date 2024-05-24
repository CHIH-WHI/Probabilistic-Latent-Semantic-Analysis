# Probabilistic-Latent-Semantic-Analysis

This project implements a Probabilistic Latent Semantic Analysis (PLSA) algorithm. Below are the instructions on how to clone the project and build it inside a Docker container.

## Prerequisites

- Git
- Docker

## Getting Started

### Clone the Repository

First, clone the repository to your local machine:

```sh
git clone --recursive https://github.com/chihwhikuo/Probabilistic-Latent-Semantic-Analysis.git
cd Probabilistic-Latent-Semantic-Analysis
```

### Build the Docker Image

Next, build the Docker image using the provided `Dockerfile`:

```sh
docker build -t plsa-image .
```

### Run the Docker Container

Run the Docker container, mounting the project directory into the container:

```sh
docker run -it -v $(pwd):/app -w /app plsa-image /bin/bash
```

### Compile the Project

Inside the Docker container, create a build directory, compile the project using CMake, and run the executable:

```sh
mkdir -p build
cd build
cmake ..
make
./plsa
```

### Exiting the Container

After running the executable, you can exit the Docker container by typing:

```sh
exit
```
