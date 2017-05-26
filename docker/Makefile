.PHONY: alpine-img web-img behave clean

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

web-img:
	docker build -t malunas-webserver ./httpd
	docker tag malunas-webserver malunas-webserver:latest

behave: behave-img web-img
	docker-compose up -d
	docker exec -t behave behave --tags=-skip-in-docker
	docker-compose down

clean:
	rm ./build -fr
