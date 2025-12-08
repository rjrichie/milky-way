# milky-way

A computer graphics project using OpenGL.

## Prerequisites

- CMake (version 3.0 or higher)
- C++ compiler with C++11 support
- OpenGL development libraries
- GLUT/FreeGLUT

## Building the Project

### Setup

Run the setup script to create the build directory and configure the project:

```bash
./scripts/setup.sh
```

This will:
- Create a `build` directory
- Run CMake to configure the project

### Building

The project is built automatically when you run an assignment/demo (see below).

Alternatively, you can build manually:

```bash
cmake --build ./build --config Release
```

## Running the Application

### Run Demo

To run the demo application:

```bash
./scripts/run.sh demo
```

### Run Assignments

To run a specific assignment:

```bash
./scripts/run.sh <assignment_name>
```

Replace `<assignment_name>` with the name of the assignment you want to run.

## Project Structure

- `demos/` - Demo applications and resources
- `src/` - Source code files
- `ext/` - External dependencies
- `scripts/` - Build and run scripts
- `build/` - Build output directory (created by setup.sh)

## Troubleshooting

### Permission Denied

If you encounter permission errors when running scripts:

```bash
chmod +x ./scripts/setup.sh
chmod +x ./scripts/run.sh
```

### Build Errors

If you encounter build errors, try cleaning the build directory:

```bash
rm -rf build
./scripts/setup.sh
```
