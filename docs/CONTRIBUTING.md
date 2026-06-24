# Contributing

Thank you for your interest in contributing to Zigbee Mesh!

## Getting Started

1. Fork the repository
2. Clone your fork
3. Create a feature branch
4. Make your changes
5. Submit a pull request

## Development Setup

```bash
# Clone
git clone https://github.com/your-username/zigbee-mesh.git
cd zigbee-mesh

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DZIGBEE_BUILD_TESTS=ON
make -j$(nproc)

# Run tests
ctest --output-on_failure
```

## Code Style

### C++ Standard
- Use C++20 features where appropriate
- Prefer modern C++ idioms
- Use `auto` when type is obvious
- Use `constexpr` for compile-time constants

### Naming Conventions
- **Classes**: PascalCase (`MeshManager`, `RadioDriver`)
- **Functions**: camelCase (`addRoute`, `getTopology`)
- **Variables**: snake_case (`route_table`, `node_count`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_HOPS`, `DEFAULT_CHANNEL`)
- **Namespaces**: snake_case (`zigbee_mesh::core`)

### File Organization
- One class per file
- Header files in `include/`
- Source files in `src/`
- Match header/source directory structure

### Comments
- Use Doxygen-style comments for public APIs
- Avoid obvious comments
- Document complex algorithms
- Keep comments up-to-date

## Commit Messages

Use conventional commits:
```
feat: add new routing algorithm
fix: resolve memory leak in mesh manager
docs: update API reference
test: add unit tests for security service
refactor: improve error handling in CLI
```

## Pull Request Process

1. Update documentation if needed
2. Add tests for new features
3. Ensure all tests pass
4. Follow code style guidelines
5. Request review from maintainers

## Reporting Issues

- Use GitHub Issues
- Include reproduction steps
- Provide environment details
- Attach relevant logs

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
