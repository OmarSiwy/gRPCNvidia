# Build and start services as defined in docker-compose.yml
docker-compose up --build

# Set the container and paths
$CONTAINER_NAME = "grpc_nvidia_app"
$SOURCE_PATH = "/app/build/compile_commands.json"
$DEST_PATH = "$HOME\Software\gRPCNvidia\compile_commands.json"

# Wait for the container to be ready before attempting to copy the file
Write-Host "Waiting for the container to become ready..."
while (-not (docker exec $CONTAINER_NAME ls $SOURCE_PATH 2>$null)) {
    Write-Host "Waiting for $CONTAINER_NAME to generate compile_commands.json..."
    Start-Sleep -Seconds 10
}

# Copy compile_commands.json from the container to the host
Write-Host "Copying compile_commands.json to $DEST_PATH..."
docker cp "${CONTAINER_NAME}:$SOURCE_PATH" "$DEST_PATH"

# Confirm the copy operation
Write-Host "Copied compile_commands.json to $DEST_PATH successfully."