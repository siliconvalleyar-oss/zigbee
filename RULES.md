# RULES.md — Reglas de Colaboración para Agentes de IA

> **Propósito:** Este documento establece las reglas obligatorias para todos los agentes de IA que trabajen en el proyecto Zigbee Mesh, con el fin de evitar conflictos, pisadas de código y garantizar que el código producido sea eficaz, consistente y mantenible.

---

## 1. PREVENCIÓN DE CONFLICTOS ENTRE AGENTES

### 1.1. Archivo de Bloqueo (LOCK)

Cada vez que un agente va a **modificar** un archivo, debe **primero** verificar si existe un archivo `.lock/<ruta-del-archivo>.lock`. Si existe, **no debe modificarlo** hasta que el lock sea liberado. Si no existe, debe crearlo y luego proceder.

Formato del lock:
```
.lock/<path-al-archivo>.lock
```
Ejemplo: `.lock/src/zigbee/zigbee_stack.cpp.lock`

El contenido del lock debe incluir:
- Nombre del agente
- Timestamp de cuando se tomó el lock
- Propósito del cambio

### 1.2. Liberación de Locks

Al terminar de modificar un archivo, el agente debe **eliminar** el archivo de lock correspondiente.

### 1.3. Timeout de Locks

Si un lock tiene más de **30 minutos**, se considera huérfano y cualquier agente puede reclamarlo (sobrescribirlo).

### 1.4. Regla de Oro: No Pisar

- **Nunca** sobrescribas un archivo completo a menos que hayas verificado que es seguro hacerlo (sin locks activos, sin cambios sin commit de otro agente).
- Prefiere siempre `str_replace` para cambios quirúrgicos en lugar de `write_file` para archivos existentes.

---

## 2. COMUNICACIÓN Y COORDINACIÓN

### 2.1. Archivo ACTIVITY.md

Cada agente que realice cambios debe **registrar su actividad** en `ACTIVITY.md` al comenzar y al terminar:

```markdown
## [YYYY-MM-DD HH:MM] - [Agent Name]

- **Archivos modificados:** [lista]
- **Propósito:** [descripción breve]
- **Estado:** [IN PROGRESS | COMPLETED | ROLLED BACK]
```

### 2.2. Archivo TODO.md

Actualiza `TODO.md` después de cada cambio significativo:
- Marca tareas completadas
- Agrega nuevas tareas si es necesario
- **No borres tareas** de otros agentes sin consultar

### 2.3. Mensajes de Commit

Usa **Conventional Commits** (formato en español o inglés, pero consistente):

```
tipo(alcance): descripción breve

Tipos: feat, fix, docs, refactor, test, chore, style
```

---

## 3. CALIDAD DEL CÓDIGO

### 3.1. Estándares

- **C++20** obligatorio (concepts, coroutines, std::span, etc.)
- **Clean Architecture** — respeta las capas (Core → HAL → Drivers → Zigbee Stack → Mesh → Services → CLI)
- **Doxygen** en todas las API públicas
- **Naming conventions** (ver CONTRIBUTING.md):
  - Clases: `PascalCase`
  - Métodos: `camelCase`
  - Variables: `snake_case`
  - Constantes: `UPPER_SNAKE_CASE`

### 3.2. Validación

Después de cualquier cambio de código:
1. El proyecto debe **compilar** sin errores ni warnings
2. Las **pruebas existentes** deben pasar
3. Los **tests nuevos** deben escribirse para funcionalidad nueva

### 3.3. No Dejar Código Muerto

- Elimina variables, funciones y archivos no utilizados
- No dejes stubs sin implementar (`TODO: implement`, `return {};`)
- No dejes comentarios de código comentado

### 3.4. Dependencias

- No agregues una dependencia sin antes verificar que el proyecto ya no la tenga
- Usa `gravity_index` para investigar servicios antes de integrarlos
- Prefiere las bibliotecas ya listadas en `DEPENDENCIES.md`

---

## 4. CONTROL DE VERSIONES

### 4.1. Flujo Git

```bash
# Cada cambio significativo debe seguir:
1. Pull/Rebase con main (si hay remote)
2. Hacer cambios con locks
3. git add <archivos>
4. git commit -m "tipo(alcance): mensaje"
5. git tag v<version>  # si aplica
```

### 4.2. Versionado semántico

Formato: `vMAJOR.MINOR.PATCH` (ver VERSION)

- **MAJOR**: cambios incompatibles en API
- **MINOR**: funcionalidad nueva compatible
- **PATCH**: bug fixes

El archivo `VERSION` debe actualizarse con cada tag.

### 4.3. Tags

Cada push con tag debe corresponder exactamente a la versión en `VERSION`.

---

## 5. ARCHIVOS DEL PROYECTO

### 5.1. Skills

Los skills en `skills/` documentan el comportamiento de cada rol de nodo. Cualquier cambio de rol debe reflejarse en su skill correspondiente.

### 5.2. Documentación

Mantén sincronizados: `README.md`, `ARCHITECTURE.md`, `API.md`, `CHANGELOG.md`.

Incrementa `CHANGELOG.md` bajo `[Unreleased]` al hacer cambios.

---

## 6. EJECUCIÓN

Si un agente detecta que **otro agente está trabajando en el mismo archivo** (lock activo), debe:
1. Esperar hasta 30 segundos
2. Si el lock persiste, registrar una nota en `ACTIVITY.md` y continuar con otra tarea
3. **Nunca** forzar la sobrescritura

---

*Última actualización: 2026-06-24*
*Versión: v0.0.1*
