.PHONY: alpine-img web-img behave clean

# ALPINE
# Generates a binary executable
build/malunas.alpine:
	@mkdir -p ./build
	docker build -t malunas-build -f ./Dockerfile.alpine.build ..
	docker run --rm malunas-build tar -c malunas.alpine | tar -x -C build

# Creates alpine image using the binary executable
alpine-img: build/malunas.alpine
	docker build -t malunas:alpine -f ./Dockerfile.alpine .
	docker tag malunas:alpine malunas:latest

alpine-behave-img: alpine-img
	docker build -t malunas-behave:alpine -f ./Dockerfile.behave ..

alpine-behave: alpine-behave-img web-img
	BASE='alpine' docker-compose up -d
	docker exec -t behave behave --tags=-skip-in-docker
	BASE='alpine' docker-compose down

# UBUNTU
# Generates a binary executable
build/malunas.ubuntu:
	@mkdir -p ./build
	docker build -t malunas-ubuntu-build -f ./Dockerfile.ubuntu.build ..
	docker run --rm malunas-ubuntu-build tar -c malunas.ubuntu | tar -x -C build

# Creates alpine image using the binary executable
ubuntu-img: build/malunas.ubuntu
	docker build -t malunas:ubuntu -f ./Dockerfile.ubuntu .
	docker tag malunas:ubuntu malunas:latest

ubuntu-behave-img: ubuntu-img
	docker build -t malunas-behave:ubuntu -f ./Dockerfile.ubuntu.behave ..
	docker tag malunas-behave:ubuntu malunas-behave:latest

ubuntu-behave: ubuntu-behave-img web-img
	BASE='ubuntu' docker-compose up -d
	docker exec -t behave behave
	BASE='ubuntu' docker-compose down 

# Misc.
web-img:
	docker build -t devel-webserver ./httpd
	docker tag devel-webserver devel-webserver:latest

clean:
	rm ./build -fr
