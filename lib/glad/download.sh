#!/bin/bash

# Create directories
mkdir -p include/glad
mkdir -p src

# Download GLAD files
curl -L \
     -H "Accept: text/plain" \
     -H "User-Agent: Mozilla/5.0" \
     -o include/glad/glad.h \
     https://raw.githubusercontent.com/Dav1dde/glad/master/include/glad/glad.h

curl -L \
     -H "Accept: text/plain" \
     -H "User-Agent: Mozilla/5.0" \
     -o src/glad.c \
     https://raw.githubusercontent.com/Dav1dde/glad/master/src/glad.c

# Verify downloads
if [ ! -s include/glad/glad.h ] || [ ! -s src/glad.c ]; then
    echo "Error: Failed to download GLAD files"
    exit 1
fi

# Remove any existing gl.h if it exists
rm -f include/glad/gl.h

echo "GLAD files downloaded successfully!" 