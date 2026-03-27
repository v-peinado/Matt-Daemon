# Dockerfile para Matt_daemon
# Testing environment - Ubuntu 24.04

FROM ubuntu:24.04

# Instalar dependencias mínimas
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    netcat-openbsd \
    procps \
    && rm -rf /var/lib/apt/lists/*

# Directorio de trabajo
WORKDIR /app

# Copiar solo archivos necesarios
COPY Makefile .
COPY client/ client/
COPY server/ server/

# Compilar automáticamente
RUN make re

# Exponer puerto 4242
EXPOSE 4242

# Ejecutar daemon por defecto
# Para modo interactivo: docker run -it matt_daemon /bin/bash
CMD ["./Matt_daemon"]