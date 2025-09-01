#!/bin/bash

docker build -t soar_echo_base .

# Run docker system prune
docker system prune -f

# Start docker-compose services
docker-compose up
