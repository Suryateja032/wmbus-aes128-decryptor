# WMBus AES128 Decryptor

## Overview

This project implements AES-128 decryption for Wireless M-Bus telegrams according to OMS Volume 2 standard. It parses encrypted meter data packets and decrypts them for analysis.

## Project Structure

- `src/` - Main source code
- `alt/` - Alternative AES implementation
- `scripts/` - Build scripts for Linux/macOS and Windows
- `docs/` - Documentation and assignment summary PDF
- `CMakeLists.txt` - CMake build configuration
- `.gitignore` - File exclusions

## Prerequisites

- C++14 compatible compiler
- OpenSSL library
- CMake build system

## Build Instructions

### Linux/macOS

