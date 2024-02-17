#!/bin/bash
set -e

# Build the container, if not built yet
docker build -t fheroes2 .

# Start the container
container_id=$(docker run -d fheroes2)

# Copy the file from the container to the host
docker cp $container_id:/app/build/fheroes2-1.0.12-Linux.deb .

# Stop the container
docker stop $container_id

# Remove the container
docker rm $container_id
