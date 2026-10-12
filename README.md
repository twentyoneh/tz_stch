# Каталог пользователей: Qt TCP-сервер и клиент

## Требования

- CMake 3.25 или новее, C++-компилятор и Make.
- Qt 6.5 или новее с модулями Core, Widgets, Network, Sql и драйвером QPSQL.
- Docker с Docker Compose, поддерживающим `up --wait --wait-timeout`.
- Запущенный Docker Engine; на macOS предварительно запустите Docker Desktop.
- Файл `.env` в корне проекта. Если его нет, скопируйте `.env.example` в `.env`
  и задайте настройки PostgreSQL.

## Сборка одной командой

Из корня проекта:

```sh
cmake --workflow --preset default
```

Команда настраивает CMake в `build`, запускает сервис `db` через Compose и
ждёт успешного healthcheck до 90 секунд, затем собирает сервер и клиент.
При ошибке запуска БД сборка приложений не начинается. Первоначальная загрузка
образа Docker может занять дополнительное время.

Если Qt установлен вне стандартных путей, перед командой задайте
`CMAKE_PREFIX_PATH` с путём к комплекту Qt.

После первоначальной настройки для повторной сборки:

```sh
cmake --build build --parallel
```

При каждой сборке Compose проверяет/запускает БД; существующий контейнер с теми же
настройками используется повторно. После сборки PostgreSQL остаётся запущенным.
Приложения собираются, но автоматически не запускаются.

## Результат

```text
build/
├── server/
│   └── server
└── client/
    └── client.app/       # macOS; на Linux — исполняемый файл client
```

На macOS исполняемый файл клиента находится в
`build/client/client.app/Contents/MacOS/client`.

## Запуск приложений

Сначала сервер:

```sh
./build/server/server
```

В другом терминале, на macOS:

```sh
open build/client/client.app
```

На Linux:

```sh
./build/client/client
```

Сервер ищет `.env` в рабочей директории, затем в директории исполняемого файла
и её родительских директориях. Можно явно указать конфигурацию:

```sh
./build/server/server --env /absolute/path/to/.env
```

## Остановка БД

```sh
cmake --build build --target database_stop
```

Останавливает контейнер, сохраняя данные PostgreSQL. Следующая обычная сборка
запустит его снова.

## Сборка без Docker

Для сборки только приложений, например в CI:

```sh
cmake -S . -B build -DSTART_DATABASE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

`cmake --workflow --preset default` снова включает автоматический запуск БД.
Проекты `server` и `client` также можно по-прежнему открывать и собирать отдельно
в Qt Creator. Для единой сборки откройте корневой `CMakeLists.txt`.
