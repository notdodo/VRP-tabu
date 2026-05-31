FROM alpine:3.21 AS build

RUN apk add --no-cache \
      build-base \
      cmake \
      ca-certificates

WORKDIR /src
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel

FROM alpine:3.21 AS runtime

RUN apk add --no-cache libstdc++

WORKDIR /app
COPY --from=build /src/build/VRP /usr/local/bin/VRP

ENTRYPOINT ["/usr/local/bin/VRP"]
