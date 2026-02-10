FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    netcat-openbsd \
    procps \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY Makefile .
COPY includes/ includes/
COPY src/ src/

RUN make re

EXPOSE 4242

CMD ["./Matt_daemon"]