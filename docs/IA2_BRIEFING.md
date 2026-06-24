# Briefing para IA-2 (Infraestructura / Documentación / DevOps)

## Tu rol
Eres la IA de infraestructura. Tu trabajo es preparar el proyecto para que sea profesional, documentado, versionado y listo para distribución.

## Lo que YA existe
- 30+ archivos C++ generados en `src/` e `include/`
- CMakeLists.txt funcional
- El código tiene errores de compilación que IA-1 está arreglando

## Tu trabajo (en orden de prioridad)

### 1. Git (AHORA)
```bash
cd /mnt/disk/src/raspberry_src/zigbee
git init
# Crear .gitignore
git add .gitignore
git commit -m "Initial project structure"
```

### 2. Documentación (después de Git)
Los .md ya existen en `docs/`. Solo necesitas mejorarlos/actualizarlos.
Ver `TODO.md` FASE 1 para tareas pendientes.

### 3. Config + Skills
Crear `configs/zigbee_mesh.conf` y los 7 archivos en `skills/`. Ver `TODO.md` FASE 2.

### 4. CI/CD
Crear `.github/workflows/build.yml`. Ver `TODO.md` FASE 3.

## REGLAS IMPORTANTES
1. **NO tocar** archivos en `src/` o `include/` — esos son de IA-1
2. **NO commitear** archivos de IA-1 sin coordinating
3. **Leer** `docs/RULES.md` para la división completa
4. **Leer** `docs/TODO.md` para ver tareas pendientes
5. Código y docs en **inglés**
6. **Documentación** vive en `docs/` (excepto README.md y LICENSE en raíz)

## Archivos compartidos (requieren coordinación)
- `CMakeLists.txt` — solo si agregas targets de infraestructura
- `configs/zigbee_mesh.conf` — tú creas, IA-1 sugiere cambios
