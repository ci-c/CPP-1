# Сборка deb-пакета (CPack) + финальная проверка ThreadSanitizer

## Context

Задача: собрать deb-пакет daemon-а через CPack; в пакет — бинарник и конфигурационные файлы (unit-файл systemd). Это финальная задача вехи: сразу после неё обязательна проверка ThreadSanitizer на гонки данных и deadlock для итоговой версии (проверять незаконченный код бессмысленно). К моменту старта уже готовы: обработка сигналов с двумя потоками (задача 1), unit-файл + sd_notify (задача 2), TCP-сервер на Asio и интеграционный тест с проверкой результата (задача 3). Обработку сигналов тестами не покрываем.

## Approach

1. CPack-конфигурация в конце корневого `CMakeLists.txt` (CPACK должен идти после всех `install()`, иначе правила не попадут в пакет). Точный блок:
   ```cmake
   set(CPACK_GENERATOR "DEB")
   set(CPACK_PACKAGE_NAME "iusearchbtw")
   set(CPACK_PACKAGE_VERSION "1.0.0")
   set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ops <ops@example.com>")
   set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "iusearchbtw calculator daemon")
   set(CPACK_DEBIAN_PACKAGE_DEPENDS "libpq5, libsystemd0")
   set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
       "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/debian/postrm")
   include(CPack)
   ```
   `libpq5`/`libsystemd0` — рантайм-зависимости (libpq и sd_notify линкуются динамически). Блок `include(CPack)` — последней строкой файла.
2. Правила установки бинарника и unit-файла (перед CPACK-блоком):
   ```cmake
   install(TARGETS iusearchbtw RUNTIME DESTINATION bin)
   install(FILES iusearchbtw.service DESTINATION lib/systemd/system)
   ```
   Относительный `lib/systemd/system` — CPack складывает в `/lib/systemd/system` при установке пакета; для Debian/Ubuntu это корректный путь unit-файлов.
3. Скрипты maintainer-скриптов. Новые файлы `debian/postinst` и `debian/postrm` (режим 0755 — CPack упакует с ним):
   - `debian/postinst`:
     ```sh
     #!/bin/sh
     set -e
     if ! getent group iusearchbtw >/dev/null; then
         groupadd --system iusearchbtw
     fi
     if ! getent passwd iusearchbtw >/dev/null; then
         useradd --system --no-create-home --shell /usr/sbin/nologin -g iusearchbtw iusearchbtw
     fi
     ```
   - `debian/postrm`:
     ```sh
     #!/bin/sh
     set -e
     if [ "$1" = "purge" ]; then
         userdel iusearchbtw 2>/dev/null || true
         groupdel iusearchbtw 2>/dev/null || true
     fi
     ```
   Запуск отдельного системного юзера — стандартная практика для production-демонов, и unit-файл позже можно усилить директивой `User=iusearchbtw` без правки пакета.
4. Обновление unit-файла: в секцию `[Service]` добавить строку `User=iusearchbtw` (после `NotifyAccess=main`) — сервис не работает рутом.
5. Сборка пакета: `cmake -S . -B build && cmake --build build && (cd build && cpack -G DEB)` → артефакт `build/iusearchbtw_1.0.0_amd64.deb`. Проверка содержимого: `dpkg-deb -c build/iusearchbtw_1.0.0_amd64.deb` показывает `./usr/bin/iusearchbtw`, `./lib/systemd/system/iusearchbtw.service`, `./usr/share/doc/iusearchbtw/*`; `dpkg-deb -I … | grep Depends` → `libpq5, libsystemd0`.
6. Финальная проверка ThreadSanitizer (критична для многопоточного daemon-а — делается ПОСЛЕ всех пунктов). Отдельная сборка с санитайзером — чтобы не ломать основную:
   ```bash
   cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug \
       -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
   cmake --build build-tsan
   ```
   Запуск сервиса и нагрузка: Postgres поднят, `./build-tsan/iusearchbtw --daemon & pid=$!`, busy-цикл подключений (как в tcp_tests) шлющий ≥100 запросов (`{"operation":"+","first_number":2,"second_number":3}` и др.), затем `kill -TERM $pid; wait $pid`. Критерий: stderr пуст от предупреждений TSan (`WARNING: ThreadSanitizer`); любое предупреждение — баг, чинить до сдачи. `./build-tsan/tcp_tests` тоже запустить под TSan (интеграционный тест сам форкает демон — важно, чтобы весь путь обработки был под санитайзером).

## Critical files & anchors

- `CMakeLists.txt` (конец файла) — блок `include(CPack)` и CPACK-переменные (шаг 1), `install()` (шаг 2).
- `debian/postinst`, `debian/postrm` — новые maintainer-скрипты (шаг 3), mode 0755.
- `iusearchbtw.service` — добавить `User=iusearchbtw` (шаг 4).
- `build/` и `build-tsan/` — артефакты cpack/проверки (не коммитить).

## Verification

Каталог `/home/c/Documents/Project/CPP-1`, Postgres доступен по `src/db.hpp:26`, права sudo для установки пакета:

- `cd build && cpack -G DEB` → `iusearchbtw_1.0.0_amd64.deb` без ошибок; `dpkg-deb -c` показывает бинарник + unit-файл (шаги 1-2).
- `sudo dpkg -i build/iusearchbtw_1.0.0_amd64.deb` → `getent passwd iusearchbtw` возвращает запись (postinst отработал).
- `sudo systemctl daemon-reload && sudo systemctl start iusearchbtw` → `systemctl is-active` → `active`; `systemctl show -p User iusearchbtw` → `User=iusearchbtw`.
- Smoke-запрос через сеть: `printf '%s\n' '{"operation":"+","first_number":2,"second_number":3}' | timeout 2 nc 127.0.0.1 50321` → `5` (или python-socket однострочник, см. Задачу 3).
- `sudo systemctl restart iusearchbtw` → `active`; `sudo systemctl stop iusearchbtw` → `inactive`, в journal последняя строка `service stopped`.
- TSan: `./build-tsan/iusearchbtw --daemon` + ≥100 запросов + SIGTERM → stderr без `WARNING: ThreadSanitizer`; `./build-tsan/tcp_tests` зелёный.
- Полный regression: `./build/tests && ./build/tcp_tests` зелёные.

## Assumptions & contingencies

- Версия пакета `1.0.0` — стартовая; тегов в git-репо нет — версия фиксируется литералом в CMakeLists.
- `CMAKE_CXX_FLAGS` для TSan перекрывает флаги по умолчанию CMake — для Debug-сборки это допустимо; релизная сборка без санитайзера собирается отдельным каталогом `build/`.
- Если `dpkg-deb` недоступен (не Debian-система), проверять состав пакета через `cpack -G DEB` + `ar x`/`tar tf` на control/data-архивах — но на Arch хосте достаточно `dpkg-deb -c` из пакета `dpkg`, при его отсутствии проверка состава опускается и остаётся `dpkg -i` на целевой Debian-машине.
- Занятость порта 50321 при TSan-прогоне: прогонять последовательно, не параллельно с работающим systemd-сервисом.
