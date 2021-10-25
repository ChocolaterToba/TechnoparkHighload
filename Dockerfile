# Сборка ---------------------------------------

# В качестве базового образа для сборки используем gcc:latest
FROM gcc:latest as build

# Установим рабочую директорию для сборки GoogleTest
WORKDIR /gtest_build

# Скачаем все необходимые пакеты и выполним сборку GoogleTest
# Такая длинная команда обусловлена тем, что
# Docker на каждый RUN порождает отдельный слой,
# Влекущий за собой, в данном случае, ненужный оверхед
RUN apt-get update && \
    apt-get install cmake -y && \
    apt-get install libgtest-dev -y && \
    cd /usr/src/googletest/googletest && \
    mkdir build && \
    cd build && \
    cmake .. && \
    cmake --build . && \
    cp lib/libgtest* /usr/lib/ && \
    cd .. && \
    rm -rf build && \
    mkdir /usr/local/lib/googletest && \
    ln -s /usr/lib/libgtest.a /usr/local/lib/googletest/libgtest.a && \
    ln -s /usr/lib/libgtest_main.a /usr/local/lib/googletest/libgtest_main.a

# Устновим libevent - нужен для работы с сокетами
RUN apt-get install libevent-dev

# Скопируем директорию /src в контейнер
COPY http_server /app/src/http_server
COPY linters /app/src/linters
COPY CPPLINT.cfg /app/src
COPY CMakeLists.txt /app/src

# Установим рабочую директорию для сборки проекта
WORKDIR /app/build

# Выполним сборку нашего проекта, а также его тестирование
RUN cmake ../src -DENABLE_TESTING=ON && \
    cmake --build . && \
    ctest

# Запуск ---------------------------------------

# В качестве базового образа используем gcc:latest
FROM gcc:latest

RUN apt-get update && apt-get install -y valgrind

# Добавим пользователя, потому как в Docker по умолчанию используется root
# Запускать незнакомое приложение под root'ом неприлично :)
RUN groupadd -r sample && useradd -r -g sample sample
USER sample

# Установим рабочую директорию нашего приложения
WORKDIR /app

# Скопируем приложение со сборочного контейнера в рабочую директорию
COPY --from=build /app/build/result .

# Установим точку входа
CMD ["valgrind", "./server"]