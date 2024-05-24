# Use an official Ubuntu as a parent image
FROM ubuntu:24.04

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git

# Set the working directory inside the container
WORKDIR /app

# Specify the command to run when the container starts
CMD ["bash"]
