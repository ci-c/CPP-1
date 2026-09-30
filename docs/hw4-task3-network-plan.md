# Сетевой ввод (Boost.Asio TCP) + интеграционный тест

## Context

Задача: daemon получает задания на расчёт по сети от клиентов и возвращает результат; сетевое взаимодействие — Boost.Asio. Для тестов пишем тестовый клиент, который шлёт данные серверу и проверяет ПОЛУЧЕННЫЙ РЕЗУЛЬТАТ расчёта; в тестах не используем sleep/sleep_for — только busy-циклы с ограничением по времени (стабильность CI). Одновременные подключения пока не тестируем — считаем, что к серверу подключается один клиент за раз (поддержка нескольких — отдельная задача). Протокол: строка JSON `\n` → строка-ответ `\n`. Формат ответа (точные литералы): успех — число (`42`); ошибка — JSON `{"error":"<e.what()>"}`. Сервер слушает `127.0.0.1`, фиксированный порт 50321 (тесты знают его). Обработку сигналов тестами не покрываем; TSan — финальная задача. Boost на хосте установлен частично (boost-libs без заголовков, `boost/asio.hpp` отсутствует), поэтому Asio берём отдельной зависимостью (standalone Asio — тот же API, без сборки Boost).

## Approach

1. Зависимость Asio в `CMakeLists.txt`. Аналога сетевого кода в проекте нет. Добавить в корневой `CMakeLists.txt`:
   ```cmake
   FetchContent_Declare(
       asio
       GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
       GIT_TAG asio-1-8-1
       GIT_SHALLOW TRUE
       SOURCE_SUBDIR asio
   )
   FetchContent_MakeAvailable(asio)
   ```
   Это standalone Asio (`asio::asio`, header-only, тянет ~1 заголовок `<asio.hpp>`) — то же API, что у `boost::asio`, без необходимости собирать Boost. Линк `asio::asio` + `Threads::Threads` в оба таргета (`iusearchbtw`, `tests`).
2. TCP-сервер — новые файлы `src/tcp_server.hpp` / `src/tcp_server.cpp` (аналога нет). Синхронный Asio (один клиент за раз — достаточно для текущей нагрузки). Точный API:
   ```cpp
   #include <asio.hpp>
   #include <string>
   #include <functional>

   class TcpServer {
   public:
       TcpServer(short port, std::function<std::string(const std::string &)> handler);
       void run(std::function<bool()> should_stop); // accept/read/write цикл
   private:
       asio::io_context io_;
       asio::ip::tcp::acceptor acceptor_;
       std::function<std::string(const std::string &)> handler_;
   };
   ```
   Семантика `run`: цикл `while (!should_stop()) { acceptor_.accept(socket); asio::read_until(socket, buf, '\n'); std::string req{istreams…}; auto resp = handler_(req); asio::write(socket, asio::buffer(resp + "\n")); }`. При исключении `system_error` (клиент оборвался, EOF) — ловить `error` в `Logger::error`, продолжать цикл. `acceptor_` конструируется `tcp::acceptor(io_, tcp::endpoint(tcp::v4(), port))`. Перед возвратом из `run` — `acceptor_.close()`.
3. Интеграция в `Runner::service_loop` (`src/runner.cpp`, сигнатура уже есть из Задачи 1: `void service_loop(SignalWatcher &watcher)`). Тело меняется на TCP-цикл:
   ```cpp
   TcpServer server(port_, [this](const std::string &req) {
       const auto r = handle_one(req);
       if (r.status == 0) return r.payload;
       return std::string("{\"error\":\"") + r.payload + "\"}";
   });
   server.run([&watcher] { return watcher.shutdown_requested(); });
   ```
   В `src/runner.hpp`: поле `unsigned short port_ = 50321;` и `#include "tcp_server.hpp"`. `process_one` (логирующая обёртка) больше не нужна в service-пути; оставить её для `run()` one-shot, но внутри переиспользовать `handle_one` (она уже есть). SIGHUP-обработку (перезагрузка кэша) пока не добавлять — появится вместе с deb-пакетом (задача 4), если понадобится; текущая веха её не требует.
4. Сброс состояния — уже есть в `handle_one` из Задачи 1 (`task = Task{};` в начале). Проверить, что он на месте: без него `is_cached`/`result` протухали бы между сетевыми запросами.
5. Тестовый клиент и тест — файл `src/tcp_test.cpp`. Тестовый клиент (внутри этого же файла, отдельный таргет не нужен — клиент существует только ради тестов):
   ```cpp
   static std::string send_request(short port, const std::string &req) {
       asio::io_context io;
       asio::ip::tcp::socket s(io);
       s.connect(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port));
       asio::write(s, asio::buffer(req + "\n"));
       asio::streambuf buf;
       asio::read_until(s, buf, '\n');
       std::istream is(&buf);
       std::string resp; std::getline(is, resp);
       return resp;
   }
   ```
   Тест `TcpIntegrationTest.EndToEnd`:
   - занять порт: `send_request(50321, …)` должен падать (connection refused) — предварительная проверка `EXPECT_ANY_THROW`.
   - форк демона: `fork()`, в дочке `execl("./build/iusearchbtw", "iusearchbtw", "--daemon", nullptr)` (если путь другой — использовать `argv[0]`/`/proc/self/exe` для вычисления); в родителе busy-цикл до 5 с шагом 5 мс пытаться `connect` до 50321 — успех = сервер поднялся. Никаких `sleep_for`/`sleep` — только busy-цикл (исключаем флаки в CI).
   - `EXPECT_EQ(send_request(50321, R"({"operation":"+","first_number":2,"second_number":3})"), "5");`
   - `EXPECT_EQ(send_request(50321, R"({"operation":"!","first_number":5})"), "120");`
   - `EXPECT_EQ(send_request(50321, R"({"operation":"^","first_number":2,"second_number":3})"), "8");`
   - `EXPECT_EQ(send_request(50321, "not json"), R"({"error":"parse error"})");` — точное содержимое e.what() у nlohmann::json зависит от версии; поэтому проверять через `HasSubstr("error")` (gtest matcher), не точную строку.
   - `kill(pid, SIGTERM)`; busy-цикл до 5 с с `waitpid(pid, &st, WNOHANG)` пока не вернулся → `EXPECT_TRUE(WIFEXITED(st) && WEXITSTATUS(st) == 0)`.
   CMake: новый таргет `add_executable(tcp_tests src/tcp_test.cpp)` + `target_link_libraries(tcp_tests PRIVATE asio::asio Threads::Threads)`; `add_test(NAME tcp_integration COMMAND tcp_tests)`.
6. `src/main.cpp` без изменений (флаг `--daemon` уже из Задачи 1). One-shot `iusearchbtw '<json>'` продолжает работать через `run()`.

## Critical files & anchors

- `src/tcp_server.hpp/.cpp` — новый синхронный Asio-сервер, API шага 2.
- `src/runner.cpp` (`service_loop`) — замена stdin-цикла на `TcpServer::run` с лямбдой-обёрткой над `handle_one`.
- `src/tcp_test.cpp` — новый: тестовый клиент `send_request` + `TcpIntegrationTest.EndToEnd` (busy-циклы, без sleep).
- `CMakeLists.txt` — `FetchContent` Asio (шаг 1), таргет `tcp_tests` (шаг 5).
- `src/runner.cpp` `handle_one` — должен начинаться с `task = Task{};` (Задача 1).

## Verification

Каталог `/home/c/Documents/Project/CPP-1`, Postgres доступен по `src/db.hpp:26`, сетевой доступ для git (FetchContent):

- `cmake -S . -B build && cmake --build build` — конфигурация тянет Asio, оба таргета линкуются.
- `./build/tests` — старые тесты зелёные; `./build/tcp_tests` — интеграционный тест зелёный (5 проверок результата + корректный shutdown).
- `ctest` (из build) — оба теста (`calculator_tests`, `tcp_integration`) проходят.
- Ручной smoke: `./build/iusearchbtw --daemon & sleep 0.5; printf '%s\n' '{"operation":"*","first_number":7,"second_number":6}' | timeout 2 nc 127.0.0.1 50321` → `42`; `kill -TERM %1`. Если `nc` недоступен — использовать `python3 -c "import socket; s=socket.create_connection(('127.0.0.1',50321)); s.sendall(b'{\"operation\":\"-\",\"first_number\":10,\"second_number\":4}\n'); print(s.makefile().readline().strip())"` → `6`.

## Assumptions & contingencies

- Порт 50321 свободен в тестовом окружении; если занят — поменять литерал в `port_` и в тесте одновременно (одна константа в одном месте не получится: тест — отдельный процесс).
- `fork()` в тесте использует тот же бинарь: путь вычислять через `/proc/self/exe` (Linux-специфично, допустимо — проект Linux-only под systemd).
- Заголовки Boost на хосте отсутствуют — поэтому standalone Asio, а не `boost::asio`; API идентичен, префикс `asio::` вместо `boost::asio::`.
- Если `FetchContent` для Asio недоступен (нет сети), остановиться и сообщить — вендорить заголовок в репо не планируем.
