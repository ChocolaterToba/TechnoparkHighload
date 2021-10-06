$ mkdir build \
$ cd build \
$ cmake .. \
$ cmake build . \
$ sudo ./../output/server \


OR if you wanna use Docker

$ docker build -t highload-server .
$ docker run --init -p 80:80 --name highload-server highload-server

For testing:
$ docker run --init -p 80:80 -v /etc/httpd.conf:/etc/httpd.conf:ro -v /home/mikhail/Documents/Technopark/http-test-suite:/var/www/html:ro --name highload-server highload-server
