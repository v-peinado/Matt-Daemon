# Dockerfile - Matt_daemon
FROM ubuntu:24.04

# Dependencias básicas
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    netcat-openbsd \
    procps \
    && rm -rf /var/lib/apt/lists/*

# Copiar código
WORKDIR /app
COPY Makefile .
COPY client/ client/
COPY server/ server/

# Compilar
RUN make bonus

# Crear directorios
RUN mkdir -p /var/log/matt_daemon /var/lock

# Exponer puerto
EXPOSE 4242

# Shell por defecto
CMD ["/bin/bash"]