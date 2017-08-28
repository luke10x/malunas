.PHONY: alpine-img web-img behave clean

# ALPINE
# Generates a binary executable
build/malunas.alpine:
	@mkdir -p ./build
	docker build -t malunas-build -f ./Dockerfile.alpine.build ..
	docker run malunas-build tar -c malunas.alpine | tar -x -C build

# Creates alpine image using the binary executable
alpine-img: build/malunas.alpine
	docker build -t malunas:alpine -f ./Dockerfile.alpine .

behave-img: alpine-img
	docker build -t malunas-behave -f ./Dockerfile.behave ..
	docker tag malunas-behave malunas-behave:latest

behave: behave-img web-img
	docker-compose up -d
	docker exec -t behave behave --tags=-skip-in-docker
	docker-compose down

# UBUNTU
# Generates a binary executable
build/malunas.ubuntu:
	@mkdir -p ./build
	docker build -t malunas-ubuntu-build -f ./Dockerfile.ubuntu.build ..
	docker run malunas-ubuntu-build tar -c malunas.ubuntu | tar -x -C build

# Creates alpine image using the binary executable
ubuntu-img: build/malunas.ubuntu
	docker build -t malunas:ubuntu -f ./Dockerfile.ubuntu .

behave-ubuntu-img: ubuntu-img
	docker build -t malunas-ubuntu-behave -f ./Dockerfile.ubuntu.behave ..
	docker tag malunas-ubuntu-behave malunas-behave:latest

ubuntu-behave: behave-ubuntu-img web-img
	docker-compose up -d
	docker exec -t behave behave
	docker-compose down

# Misc.
web-img:
	docker build -t malunas-webserver ./httpd
	docker tag malunas-webserver malunas-webserver:latest

clean:
	rm ./build -fr
