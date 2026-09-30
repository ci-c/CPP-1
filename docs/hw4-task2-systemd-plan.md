# Конфигурация systemd (unit-файл)

## Context

Задача: сделать daemon управляемым systemd-сервисом — команды `systemctl start/restart/stop`. Приложение уже умеет: режим `--daemon` с двумя потоками и завершением по SIGTERM/SIGINT (предыдущая задача). Эта задача добавляет уведомления готовности через `sd_notify` (Type=notify) и unit-файл `iusearchbtw.service`. Обработку сигналов тестами не покрываем; гонки потоков проверяются ThreadSanitizer-ом в финальной задаче (после стабилизации кода).

## Approach

1. Уведомления systemd в `Runner::run_service` (`src/runner.cpp`, объявления уже есть после Задачи 1 — добавить вызовы, сигнатуры не менять). Заголовок `<systemd/sd-daemon.h>`. Точные правки внутри `run_service` (вставить до/после существующих строк):
   - после `SignalWatcher watcher; watcher.start();` — `sd_notify(0, "READY=1");`
   - после `Logger::getInstance().info("service ready");` ничего не добавлять (строка уже есть из Задачи 1); если её нет — добавить `Logger::getInstance().info("service ready");` сразу после `sd_notify(0, "READY=1");`
   - перед `return 0;` — `sd_notify(0, "STOPPING=1");` сразу после `watcher.stop();`
   Семантика: без переменной окружения `NOTIFY_SOCKET` `sd_notify` — no-op, тесты и one-shot не ломаются.
2. Зависимость сборки. В `CMakeLists.txt` (корень CPP-1): `find_package(PkgConfig REQUIRED)` + `pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)`; линк `PkgConfig::SYSTEMD` в оба таргета — `iusearchbtw` и `tests` (строки 115-139): в `tests` обязательно, потому что `TEST_SOURCES` включает `APP_SOURCES` с `runner.cpp`, где теперь `sd-daemon.h`. Обязательный `REQUIRED`, без фолбэка: молчаливый демон нарушил бы контракт `Type=notify`. Если `pkg_check_modules` не найдёт libsystemd — остановиться и сообщить (нужен пакет `libsystemd-dev`/`systemd-devel`, на Arch уже установлен — проверено `pkg-config` на этой машине).
3. Unit-файл. Новый файл `iusearchbtw.service` в корне CPP-1, содержимое дословно:
   ```ini
   [Unit]
   Description=iusearchbtw calculator daemon
   After=network.target postgresql.service
   Wants=postgresql.service

   [Service]
   Type=notify
   NotifyAccess=main
   ExecStart=/usr/local/bin/iusearchbtw --daemon
   Restart=on-failure
   RestartSec=5s
   TimeoutStopSec=10s
   StandardOutput=journal
   StandardError=journal

   [Install]
   WantedBy=multi-user.target
   ```
   Путь `/usr/local/bin` = дефолтный префикс `cmake --install`. `Type=notify` + `NotifyAccess=main`: готовность подтверждает главный поток через `sd_notify` из шага 1. `Restart=on-failure`: падение по недоступному Postgres перезапустит сервис. Деплой — ручное копирование `cp iusearchbtw.service /etc/systemd/system/ && systemctl daemon-reload` (автоустановку unit-файла через `install()` не добавлять — путь абсолютный и зависит от дистрибутива; автоматическая установка появится в deb-пакете, Задача 4).

## Critical files & anchors

- `src/runner.cpp` (`run_service`) — три вставки `sd_notify`: READY=1, STOPPING=1 (+ service ready info при отсутствии).
- `CMakeLists.txt:111-139` — линк `PkgConfig::SYSTEMD` в оба таргета.
- `iusearchbtw.service` — новый файл, содержимое дословно из шага 3; обязательный минимум — `start`/`restart`/`stop`.

## Verification

Каталог `/home/c/Documents/Project/CPP-1`, Postgres доступен по `src/db.hpp:26`. Выполняется с правами sudo:

- `cmake -S . -B build && cmake --build build && ./build/tests` — зелёный (линк `PkgConfig::SYSTEMD` не сломал таргеты).
- Деплой: `cmake --install build` (по умолчанию в `/usr/local`), `sudo cp iusearchbtw.service /etc/systemd/system/`, `sudo systemctl daemon-reload`.
- `sudo systemctl start iusearchbtw` → `systemctl is-active iusearchbtw` → `active` (доказательство READY=1 при Type=notify; без него статус был бы activating/failed).
- `sudo systemctl restart iusearchbtw` → `systemctl show -p MainPID iusearchbtw` до и после → PID изменился, `is-active` → `active`.
- `sudo systemctl stop iusearchbtw` → `systemctl is-active iusearchbtw` → `inactive` (exit 0 без SIGKILL — `TimeoutStopSec` не выбран), в journal (`journalctl -u iusearchbtw -e`) последняя строка `service stopped`.

## Assumptions & contingencies

- libsystemd доступен на хосте сборки (проверено). Если контур сборки без него — не делать опциональный фолбэк, а остановить конфигурацию с сообщением об отсутствующем пакете.
- Порт/транспорт не затрагивается — сервис пока читает stdin (сетевой ввод появится в Задаче 3); unit-файл менять не придётся.
- Потоковая модель не меняется: три потока из Задачи 1 (main, sigwait, service_loop); `sd_notify` вызывается только из main-потока в `run_service`.
