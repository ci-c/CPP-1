# Обработка сигналов ОС (SIGTERM/SIGINT) + переход к daemon-процессу

## Context

Задача: превратить one-shot CLI-утилиту в долгоживущий daemon с корректной обработкой сигналов ОС — по SIGTERM процесс завершает работу (exit 0), SIGINT обрабатывается аналогично (ручной Ctrl+C при отладке). Архитектура: два потока — поток №1 (внутри `SignalWatcher`) принимает сигналы через `sigwait`, поток №2 (`Runner::service_loop`) получает данные из stdin и обрабатывает их существующим конвейером. Обработку сигналов модульными тестами не покрываем — проверяется интеграционно (запуск + kill). После этой задачи процесс ещё не управляется systemd (unit-файл — следующая задача), но уже корректно завершается по сигналу и продолжает работать при EOF stdin. Проверку гонок ThreadSanitizer-ом выполняем после стабилизации всей функциональности (финальная задача).

## Approach

1. Новый класс `SignalWatcher` — поток №1, только сигналы. Файлы `src/signal_watcher.hpp` / `src/signal_watcher.cpp` (аналога в проекте нет). Точный API:
   ```cpp
   class SignalWatcher {
   public:
       SignalWatcher();
       ~SignalWatcher();
       SignalWatcher(const SignalWatcher &) = delete;
       SignalWatcher &operator=(const SignalWatcher &) = delete;
       void start();
       void stop();
       bool shutdown_requested() const;
       bool consume_reload_request();
   private:
       void loop();
       sigset_t watch_set_{};
       std::thread thread_;
       std::atomic<bool> shutdown_{false};
       std::atomic<bool> reload_{false};
       std::atomic<bool> running_{false};
   };
   ```
   Семантика: `start()` сохраняет текущую маску (`pthread_sigmask(SIG_BLOCK, {SIGTERM, SIGINT}, …)` — процесс однопоточен на момент вызова, маска действует процессно), запускает поток с `sigwait(&watch_set_, &sig)`: SIGTERM/SIGINT → `shutdown_ = true`, `running_ = false`, выход из цикла; SIGHUP → `reload_ = true`, продолжать ждать (SIGHUP понадобится в Задаче 2). `stop()` идемпотентен: `shutdown_ = true`; если `running_` — `pthread_kill(native_handle, SIGTERM)` для пробуждения заблокированного `sigwait` (ошибки игнорировать); `join()`, если joinable; восстановить сохранённую маску. Деструктор вызывает `stop()`. SIGHUP здесь можно не блокировать в `start()` — только SIGTERM/SIGINT (перезагрузка кэша добавляется в Задаче 2 вместе с systemd reload).
2. Видимость логов демона: `Logger::set_info_level()` (`src/logger.hpp`, `src/logger.cpp`) — новый статический метод, тело `spdlog::set_level(spdlog::level::info);`. Дефолт `err` в конструкторе (`src/logger.cpp:13`) не трогать (one-shot и тесты без изменений). Демон вызывает один раз на старте, иначе `info`-строки не видны.
3. Конвейер в `Runner` + диспетч в `main` (зависит от шагов 1–2). Роли потоков фиксированы: главный поток — координатор (создание/останов потоков, join), поток №1 — сигналы (`SignalWatcher::loop`), поток №2 — данные (`Runner::service_loop`). Из тела `try` в `Runner::run` (`src/runner.cpp:23-32`) вырезать стадии в новый метод `std::string process_one(const std::string &input)` (`src/runner.hpp`: добавить `#include <string>`, новая структура и объявления):
   ```cpp
   struct RequestResult {
       int status = 1;      // 0 = OK, 1 = ошибка
       std::string payload; // OK: число; ошибка: e.what()
   };
   int process_one(const std::string &input);        // вызывает handle_one, логирует, возвращает status
   RequestResult handle_one(const std::string &input); // для тестов
   int run_service();                                 // координатор (main-поток)
   void service_loop(SignalWatcher &watcher);         // тело потока №2
   ```
   `handle_one` тело: в начале сбросить состояние `task` (`task = Task{};` — обязательно: `is_cached`/`result` от прошлого запроса иначе протухают), затем `parser.exec(input); checker.exec(); cache_reader.exec(); if (!task.is_cached) { calc.exec(); cache_writer.exec(); }` и `return {0, std::to_string(task.result)};`; всё тело обернуть в `try/catch (const std::exception &e) → return {1, e.what()}`. `process_one` вызывает `handle_one`, при `status != 0` пишет `Logger::error("error: " + payload)` (тот же формат, что сегодня в `Runner::run:32-34`), возвращает `status`. `run()` сводится к проверке `argc` и `return process_one(argv[1])` — внешний one-shot CLI не меняется.
4. Тело потока №2 `void service_loop(SignalWatcher &watcher)` (читает `std::cin`; заголовки `<poll.h>`, `<unistd.h>`):
   ```
   struct pollfd pfd{STDIN_FILENO, POLLIN, 0};
   std::string line;
   bool stdin_eof = false;
   while (!watcher.shutdown_requested()) {
       const bool have_data = !stdin_eof && (std::cin.rdbuf()->in_avail() > 0 || ::poll(&pfd, 1, 100) > 0);
       if (watcher.shutdown_requested()) break;      // re-check после poll
       if (have_data) {
           if (!std::getline(std::cin, line)) { stdin_eof = true; continue; }
           if (!line.empty()) process_one(line);      // ошибки залогированы внутри
       } else {
           std::this_thread::sleep_for(std::chrono::milliseconds(10));
       }
   }
   ```
   `poll` с таймаутом 100 мс + `sleep_for(10)` обязательны: сигналы заблокированы и `sigwait` их съедает, блокирующий `getline` не проснулся бы на SIGTERM; после EOF stdin демон НЕ завершается, а переходит в idle-сон до сигнала (иначе systemd-демон с закрытым stdin умирал бы мгновенно). `in_avail() > 0` перед `poll` обязателен: буфер `cin` может держать строки при пустом fd.
5. Координатор `int run_service()` (главный поток):
   ```
   Logger::set_info_level();
   SignalWatcher watcher;
   watcher.start();
   std::thread worker(&Runner::service_loop, this, std::ref(watcher));
   worker.join();          // ждёт SIGTERM/SIGINT
   watcher.stop();
   Logger::getInstance().info("service stopped");
   return 0;
   ```
   `Task`/`Database`/все воркеры трогает только поток №2. Конструктор `Runner` при недоступной БД бросает как сегодня (не ловить — поведение one-shot сохраняется). `src/main.cpp`: в начале `main` проигнорировать SIGPIPE через `sigaction` (не `signal`), затем `if (argc == 2 && std::string(argv[1]) == "--daemon") return Runner().run_service();` — точный литерал флага `--daemon`. SIGPIPE ловим здесь, потому что в Задаче 3 ответы уйдут в сокет, а запись в закрытый сокет убила бы процесс.
6. CMake (`CMakeLists.txt`): `find_package(Threads REQUIRED)`; `signal_watcher.cpp` добавить в `APP_SOURCES`, `signal_watcher.hpp` в `APP_HEADERS` (строки 72-99); `Threads::Threads` в оба `target_link_libraries` (111-139). Тесты не меняются (обработку сигналов покрываем интеграционной проверкой из Verification, а не модульными тестами).

## Critical files & anchors

- `src/runner.cpp:17-38` — вырезать стадии в `handle_one`/`process_one`; добавить `service_loop` и `run_service`.
- `src/signal_watcher.hpp` — новый API шага 1; поток №1 (sigwait).
- `src/task.hpp:3-9` — сброс `task = Task{}` в начале `handle_one`.
- `src/logger.cpp:13` — дефолт `err`; `set_info_level()` не трогает конструктор.
- `CMakeLists.txt:72-139` — списки источников и линки обоих таргетов.

## Verification

Каталог `/home/c/Documents/Project/CPP-1`, Postgres доступен по `src/db.hpp:26` (без него бинарные проверки невозможны — `Database` бросает в конструкторе; пропустить их и опереться на `./build/tests`):

- `cmake -S . -B build && cmake --build build && ./build/tests` — зелёный, без новых упавших тестов.
- One-shot не сломан: `./build/iusearchbtw '{"operation":"*","first_number":7,"second_number":6}'` → `42`.
- Демон и сигнал: `./build/iusearchbtw --daemon < /dev/null & pid=$!; sleep 0.5; kill -TERM $pid; wait $pid; echo "exit=$?"` → `exit=0`, в stderr `service stopped`. Аналогично `kill -INT` → `exit=0`.
- Построчная обработка: `printf '%s\n' '{"operation":"+","first_number":2,"second_number":3}' '{"operation":"!","first_number":5}' > /tmp/jobs.txt; ./build/iusearchbtw --daemon < /tmp/jobs.txt & pid=$!; sleep 1; kill -TERM $pid; wait $pid` → stdout `5` и `120` (каждый со своей строки), exit 0.
- Битый ввод не убивает: `printf '%s\n' 'not json' '{"operation":"+","first_number":1,"second_number":1}' > /tmp/bad.txt; ./build/iusearchbtw --daemon < /tmp/bad.txt & pid=$!; sleep 1; kill -TERM $pid; wait $pid` → stderr `error: …`, stdout `2`, exit 0.

## Assumptions & contingencies

- Потоковая модель: ровно 3 потока — main (координатор), sigwait (внутри `SignalWatcher`), `service_loop`. `Task`/`Database`/воркеры — только `service_loop`; связь через атомики `SignalWatcher`. Маску сигналов ставить в `SignalWatcher::start()` до создания `service_loop`.
- Если после сборки окажется, что `process_one`/`handle_one` конфликтуют с существующими именами (не должны — таких символов нет), переименовать только эти два метода, не трогая публичный CLI.
