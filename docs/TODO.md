# TODO — Zigbee Mesh Multiplatform C++20

> Estado actualizado: 2026-06-24
> Este archivo es la fuente de verdad para el trabajo pendiente.
> Cada tarea tiene un responsable asignado: **AG1** (AI actual) o **AG2** (AI de código/compilación).

---

## Reglas de Colaboración AG1 ↔ AG2

### Principios
1. **No pisarse**: AG1 genera archivos nuevos. AG2 compila, arregla errores y crea git.
2. **Archivos nuevos = AG1**: Solo AG1 crea/reescribe archivos del proyecto.
3. **Compilación y fixes = AG2**: Solo AG2 ejecuta cmake, make, arregla errores de compilación.
4. **Git = AG2**: Solo AG2 inicializa el repo, crea commits y pushes.
5. **No duplicar trabajo**: Si AG2 ya creó un archivo, AG1 no lo reescribe.

### Flujo de trabajo
```
AG1 genera archivos → AG2 compila y arregla → AG2 crea commit → AG1 genera más archivos
```

### Archivos que AG1 NO debe tocar después de que AG2 los compilen
- CMakeLists.txt (AG2 puede ajustar para compilar)
- src/*.cpp que AG2 ya compiló exitosamente
- .github/workflows/ (AG2 crea)

### Archivos que AG2 NO debe tocar (solo AG1)
- include/*.h (headers son sagrados)
- skills/*.skill.md
- docs/*.md (README, ARCHITECTURE, etc.)
- prompt.txt

### Comunicación
- AG2 debe reportar en un archivo `BUILD_STATUS.md` si hay errores pendientes
- AG1 lee `BUILD_STATUS.md` antes de crear archivos nuevos para saber qué necesita ajustes

---

## Fase 1: Archivos Pendientes de Generación (AG1)

- [ ] **AG1** `examples/end_device/end_device.cpp` — Ejemplo de End Device
- [ ] **AG1** `examples/CMakeLists.txt` — Integración de ejemplos en CMake
- [ ] **AG1** `configs/zigbee_mesh.conf` — Configuración por defecto INI
- [ ] **AG1** `skills/coordinator.skill.md` — Skill del coordinador
- [ ] **AG1** `skills/router.skill.md` — Skill del router
- [ ] **AG1** `skills/end_device.skill.md` — Skill del end device
- [ ] **AG1** `skills/commissioning.skill.md` — Skill de commissioning
- [ ] **AG1** `skills/diagnostics.skill.md` — Skill de diagnósticos
- [ ] **AG1** `skills/ota_update.skill.md` — Skill de OTA update
- [ ] **AG1** `skills/mesh_monitor.skill.md` — Skill de monitoreo mesh
- [ ] **AG1** `docs/README.md` — Documentación principal
- [ ] **AG1** `docs/ARCHITECTURE.md` — Arquitectura del proyecto
- [ ] **AG1** `docs/API.md` — Referencia de API pública
- [ ] **AG1** `docs/NETWORK.md` — Documentación de red
- [ ] **AG1** `docs/SECURITY.md` — Documentación de seguridad
- [ ] **AG1** `docs/BUILD.md` — Instrucciones de compilación
- [ ] **AG1** `docs/DEPENDENCIES.md` — Dependencias del proyecto
- [ ] **AG1** `docs/ROADMAP.md` — Roadmap del proyecto
- [ ] **AG1** `docs/CONTRIBUTING.md` — Guía de contribución
- [ ] **AG1** `docs/TODO.md` — (este archivo, moverlo a docs/)
- [ ] **AG1** `docs/CHANGELOG.md` — Changelog
- [ ] **AG1** `LICENSE` — Licencia MIT
- [ ] **AG1** `scripts/build.sh` — Script de build
- [ ] **AG1** `scripts/install.sh` — Script de instalación
- [ ] **AG1** `cmake/FindSQLite3.cmake` — Módulo CMake para SQLite3
- [ ] **AG1** `Doxyfile` — Configuración de Doxygen

## Fase 2: Compilación y Fixes (AG2)

- [ ] **AG2** Inicializar git repo: `git init && git add . && git commit -m "initial"`
- [ ] **AG2** Ejecutar `cmake -B build && cmake --build build` — encontrar errores
- [ ] **AG2** Arreglar todos los errores de compilación
- [ ] **AG2** Crear `.gitignore` apropiado
- [ ] **AG2** Hacer commit con mensaje descriptivo
- [ ] **AG2** Crear `BUILD_STATUS.md` con estado de compilación

## Fase 3: Testing (AG1 + AG2)

- [ ] **AG1** `tests/unit/test_core_types.cpp` — Tests de tipos core
- [ ] **AG1** `tests/unit/test_bytebuffer.cpp` — Tests de ByteBuffer
- [ ] **AG1** `tests/unit/test_config.cpp` — Tests de Config
- [ ] **AG1** `tests/unit/test_routing.cpp` — Tests de routing
- [ ] **AG1** `tests/unit/test_mesh.cpp` — Tests de mesh manager
- [ ] **AG1** `tests/unit/test_storage.cpp` — Tests de storage
- [ ] **AG1** `tests/integration/test_driver_mrf24j40.cpp` — Tests integración driver
- [ ] **AG1** `tests/CMakeLists.txt` — Build de tests con GoogleTest
- [ ] **AG2** Compilar y ejecutar tests: `cmake --build build && ctest --test-dir build`

## Fase 4: CI/CD (AG2)

- [ ] **AG2** `.github/workflows/build.yml` — CI para Ubuntu, macOS, Windows
- [ ] **AG2** `.github/workflows/static-analysis.yml` — clang-tidy, cppcheck
- [ ] **AG2** `.github/workflows/test.yml` — Ejecución de tests

## Fase 5: Optimización y Polish (AG2)

- [ ] **AG2** `tools/` — Utilidades de diagnóstico
- [ ] **AG2** `third_party/` — Instrucciones para dependencias externas
- [ ] **AG2** Verificar build completo limpio sin warnings
- [ ] **AG2** Push a GitHub repo

---

## Prioridad de AG2 (cuando entre)

1. **PRIMERO**: `git init` + `.gitignore` + commit inicial
2. **SEGUNDO**: `cmake -B build && cmake --build build` — identificar TODOS los errores
3. **TERCERO**: Arreglar errores de compilación uno por uno
4. **CUARTO**: Commit con build exitoso
5. **QUINTO**: Crear `.github/workflows/build.yml`

## Prioridad de AG1 (este sesión)

1. Generar los 7 archivos skills/*.skill.md
2. Generar los 11 archivos docs/*.md
3. Generar configs/zigbee_mesh.conf
4. Generar examples/end_device + CMakeLists
5. Generar scripts/ y LICENSE
6. Generar Doxyfile
