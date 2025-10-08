#!/bin/bash

# Glide Glide Test Script
# This script tests the Glide functionality between server and client

echo "=== Glide Glide Test Script ==="
echo "This script will build and test the Glide functionality"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found. Please run this script from the Glide project directory."
    exit 1
fi

# Create build directory
print_status "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
print_status "Configuring project with CMake..."
if cmake .. -DCMAKE_BUILD_TYPE=Debug; then
    print_success "CMake configuration successful"
else
    print_error "CMake configuration failed"
    exit 1
fi

# Build the project
print_status "Building project..."
if make -j$(nproc); then
    print_success "Build successful"
else
    print_error "Build failed"
    exit 1
fi

# Check if executables were created
if [ -f "server" ] && [ -f "client" ]; then
    print_success "Server and client executables created successfully"
else
    print_error "Failed to create executables"
    exit 1
fi

# Get local IP address
LOCAL_IP=$(hostname -I | awk '{print $1}')
print_status "Local IP address: $LOCAL_IP"

echo
echo "=== Build Complete ==="
echo
echo "To test the Glide functionality:"
echo
echo "1. On the SERVER machine, run:"
echo "   ./server $LOCAL_IP"
echo
echo "2. On the CLIENT machine, run:"
echo "   ./client $LOCAL_IP"
echo
echo "3. Test the Glide functionality:"
echo "   - Move your mouse to the right edge of the server screen"
echo "   - The cursor should disappear from server and appear on client"
echo "   - You should be able to control the client with server's mouse/keyboard"
echo "   - Move mouse to left edge of client to return control to server"
echo
echo "Expected behavior:"
echo "- Server: Mouse at right edge -> cursor hides, control transfers to client"
echo "- Client: Receives control, cursor shows, can use mouse/keyboard"
echo "- Client: Mouse at left edge -> control returns to server"
echo "- Server: Cursor shows again, regains control"
echo
echo "Logs will show:"
echo "- Key exchange completion messages"
echo "- Screen transition messages"
echo "- Encryption status"
echo
print_success "Test setup complete!"

# Optional: Run a quick connectivity test
echo
read -p "Do you want to run a quick connectivity test? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_status "Starting connectivity test..."
    
    # Start server in background
    print_status "Starting server..."
    timeout 10s ./server $LOCAL_IP &
    SERVER_PID=$!
    sleep 2
    
    # Start client in background
    print_status "Starting client..."
    timeout 8s ./client $LOCAL_IP &
    CLIENT_PID=$!
    sleep 5
    
    # Check if processes are still running
    if kill -0 $SERVER_PID 2>/dev/null; then
        print_success "Server is running"
    else
        print_warning "Server process ended"
    fi
    
    if kill -0 $CLIENT_PID 2>/dev/null; then
        print_success "Client is running"
    else
        print_warning "Client process ended"
    fi
    
    # Clean up
    print_status "Cleaning up test processes..."
    kill $SERVER_PID 2>/dev/null
    kill $CLIENT_PID 2>/dev/null
    wait 2>/dev/null
    
    print_success "Connectivity test complete"
fi

echo
echo "=== Test Script Complete ==="
echo "The Glide system is ready for testing!"
