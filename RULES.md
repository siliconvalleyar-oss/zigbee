# Reglas de Coordinación entre IAs

## Contexto
Dos IAs trabajan en paralelo en el proyecto Zigbee Mesh C++20.
La división evita conflictos de archivos y permite trabajo concurrente.

---

## División de Responsabilidades

### IA-1 (Arquitecto / Generador de Código)
**Enfoque**: Generación de código fuente C++

**Archivos que PUEDE modificar/crear**:
- `src/**/*.cpp` — Implementaciones
- `include/**/*.h` — Headers
- `include/**/*.hpp` — Headers alternativos
- `examples/**/*.cpp` — Ejemplos
- `tests/**/*.cpp` — Tests unitarios
- `cmake/**` — Módulos CMake custom

**NO TOCAR**:
- `README.md`, `ARCHITECTURE.md`, `API.md`, `BUILD.md`, `DEPENDENCIES.md`
- `NETWORK.md`, `SECURITY.md`, `ROADMAP.md`, `TODO.md`, `CONTRIBUTING.md`
- `.github/workflows/**` — CI/CD
- `skills/**` — Skill definitions
- `configs/**` — Configuración por defecto
- `CMakeLists.txt` — Solo tocar si agrega nuevos targets de código

### IA-2 (Infraestructura / Documentación / DevOps)
**Enfoque**: Git, documentación, CI/CD, configuración, skills

**Archivos que PUEDE modificar/crear**:
- `README.md`, `ARCHITECTURE.md`, `API.md`, `BUILD.md`, `DEPENDENCIES.md`
- `NETWORK.md`, `SECURITY.md`, `ROADMAP.md`, `TODO.md`, `CONTRIBUTING.md`
- `CHANGELOG.md`, `LICENSE`
- `.github/workflows/**` — CI/CD
- `skills/**` — Skill definitions
- `configs/**` — Configuración
- `scripts/**` — Scripts de build/install
- `tools/**` — Utilidades
- `third_party/**` — Dependencias externas
- `docs/**` — Documentación adicional
- `CMakeLists.txt` — Solo tocar si agrega targets de infraestructura
- Git operations: init, add, commit, push, tag

**NO TOCAR**:
- `src/**/*.cpp` — No modificar implementaciones
- `include/**/*.h` — No modificar headers
- `examples/**/*.cpp` — No modificar ejemplos
- `tests/**/*.cpp` — No modificar tests

---

## Reglas Generales

1. **Comunicación**: Usar este archivo RULES.md para coordinar. Actualizar §Estado cuando se complete una tarea.
2. **Commits**: Cada IA hace commit solo de SUS archivos. Nunca commitear archivos del otro dominio.
3. **Merge conflicts**: Si hay conflicto, revisar RULES.md para determinar quién tiene prioridad.
4. **Build verification**: Antes de commit, ejecutar `cmake -B build && cmake --build build` para verificar que el código compila.
5. **No pseudocódigo**: Solo código C++20 real y compilable (regla de prompt.txt).
6. **Idioma**: Código y docs en inglés. Comentarios en español si el usuario lo pide.
7. **Archivos compartidos** (requieren coordinación):
   - `CMakeLists.txt` — Coordinar cambios
   - `configs/zigbee_mesh.conf` — IA-2 crea, IA-1 puede sugerir cambios

---

### ⚜️ REGLA DE ORO: NUNCA ELIMINAR ARCHIVOS

**Está terminantemente prohibido eliminar archivos del proyecto.**

En lugar de eliminar, **mueve el archivo a la carpeta `olds/`** manteniendo la misma estructura de directorios:

```
# MAL ❌ — No hacer nunca
rm src/zigbee/ieee802154.cpp

# BIEN ✅ — Mover a olds/ conservando la ruta relativa
mv src/zigbee/ieee802154.cpp olds/src/zigbee/ieee802154.cpp
```

**Reglas específicas:**
- La carpeta `olds/` replica la estructura del proyecto para facilitar búsquedas
- `olds/` está en `.gitignore` — los archivos viejos **no se commitean**
- Si un archivo ya no sirve pero tiene código que podría ser útil después, va a `olds/`
- Si se reemplaza un archivo por una implementación nueva, el viejo va a `olds/`
- Si se renombra un archivo, el nombre viejo queda en `olds/` como referencia
- **Nunca** eliminar archivos aunque parezcan redundantes
- Siempre que veas `rm`, `delete`, `remove` en código o comandos, **detente y usa `mv` a `olds/`**

---

## §Estado de Coordinación

### Tareas completadas por IA-1:
- [x] Core types, result, event, logger, config, timer, bytebuffer (headers)
- [x] Drivers: HAL, backend, MRF24J40, CC2530, XBee (headers + src)
- [x] Zigbee stack: ieee802154, security_manager, zdo_layer, zigbee_stack (headers + src)
- [x] Routing table + route discovery (headers + src)
- [x] Mesh manager (headers + src)
- [x] Services: network_manager, mesh, diagnostics, OTA (headers + src)
- [x] Security service (headers + src)
- [x] Storage manager (headers + src)
- [x] CLI (headers + src)
- [x] Main application entry point

### Tareas pendientes para IA-1:
- [ ] **CRÍTICO**: Fix compilation errors (ver §Errores conocidos)
- [ ] Verificar build con `cmake -B build && cmake --build build`
- [ ] Tests unitarios completos
- [ ] Tests de integración
- [ ] Tests de mesh
- [ ] Tests de routing
- [ ] Tests de seguridad
- [ ] Ejemplo end_device

### Tareas completadas por IA-2:
- (Ninguna aún — IA-2 empieza ahora)

### Tareas pendientes para IA-2:
- [ ] Git init + primer commit
- [ ] README.md
- [ ] ARCHITECTURE.md
- [ ] API.md
- [ ] BUILD.md
- [ ] DEPENDENCIES.md
- [ ] NETWORK.md
- [ ] SECURITY.md
- [ ] ROADMAP.md
- [ ] TODO.md (actualizado)
- [ ] CONTRIBUTING.md
- [ ] CHANGELOG.md
- [ ] LICENSE (MIT)
- [ ] .github/workflows/build.yml
- [ ] skills/ (7 archivos)
- [ ] configs/zigbee_mesh.conf
- [ ] scripts/ (build.sh, install.sh)
- [ ] .gitignore
- [ ] Doxygen config

---

## §Errores conocidos (para IA-1)

El código tiene errores de compilación que deben corregirse ANTES de que IA-2 haga el primer commit:

1. **`src/zigbee/security_manager.cpp`**: `mbedtlsCcmEncrypt` — firma no coincide con el header. El header declara 7 parámetros pero el .cpp llama con 6.

2. **`src/zigbee/zigbee_stack.cpp`**: `DeviceEvent` y `EventType` deben calificarse como `core::DeviceEvent` y `core::EventType`.

3. **`src/zigbee/zdo_layer.cpp`**: `TimerManager` debe ser `core::TimerManager`.

4. **`include/drivers/backend.h`**: `SPIBackend` — métodos `transfer`, `readRegister`, etc. deben ser `const` o `fd_` debe ser `mutable`.

5. **`include/core/logger.h`**: `mutex_` debe ser `mutable` para `getLevel() const`.

6. **OpenSSL deprecado**: `AES_set_encrypt_key` y `AES_encrypt` están deprecados en OpenSSL 3.0. Usar `EVP_*` API.
