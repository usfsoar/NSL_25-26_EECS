# Define the script actions
param (
    [string]$dockerComposeArguments
)

# Build the Docker image
docker build -t soar_echo_base .
echo "Build done"

# Run docker system prune
docker system prune -f | Out-Null
echo "Prune done"

# Start docker-compose services
# docker-compose up
docker-compose run soar_echo_base $dockerComposeArguments
