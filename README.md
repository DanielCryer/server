# Биржа (сервер)

Тестовое задание NTProgress.

## Установка

Используйте cmake для установки.

```bash
cmake -DCMAKE_PREFIX_PATH=<qt_directory> <sources_path>
cmake --build .
```
Указание -DCMAKE_PREFIX_PATH может и не понадобиться, но мне пригодилось.

## Запуск

Запустите приложение server через терминал. Принимает 2 необязательных параметра: адрес и порт для запуска сервера. По умолчанию 127.0.0.1:7070.

```bash
./server 127.0.0.1 7070
```
После запуска создает в исходной директории файл базы данных. Если файл уже существует, но не подходит для работы сервера (другие таблицы), будет выдано сообщение об этом и приложение не запустится.

## Комментарий

Если честно, когда увидел задание, сразу забыл про предоставленный каркас приложения. А когда вспомнил, было уже поздно...

Изначально собирался реализовать сетевое взаимодействие на TCP-сокетах, но после пришел к выводу, что http-запросы будут значительно удобнее. Да и применяются в сетевых приложениях они чаще, хотя могу и ошибаться.

Разбираясь в вариантах реализации http-сервера наткнулся на библиотеку [cpp-httplib](https://github.com/yhirose/cpp-httplib). Показалось, что она весьма неплохо подходит для подобного задания. 

С точки зрения архитектуры и расширяемости это была ошибка. Да, она поддерживает параллельную обработку запросов, однако гибкости ей не хватает. Некоторые процессы можно было бы разделить на разные потоки, однако необходимость обработки запроса одной функцией этого не позволяет. Кроме того, в основном приложение было реализовано с использованием Qt. В результате возникали некоторые проблемы при взаимодействии с данной библиотекой и ограничивалась возможность использования событий и слотов Qt.

В случае теоретической дальнейшей разработки данного приложения в первую очередь необходимо будет убирать эту библиотеку. И тогда хотя бы спокойно вынести работу с БД в отдельный поток. Возможно, стоит загрузить активные заявки в память и обрабатывать их напрямую, а не через запросы БД. Но тут тоже не уверен, опыта не хватает.

И, дополнительно, вопрос безопасности. Так как в задании про это ничего не сказано, я реализовал только Basic авторизацию. Это по факту особой защиты не дает, учитывая, что используется http протокол, но лучше, чем ничего).
