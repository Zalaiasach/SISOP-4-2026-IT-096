FROM ubuntu:latest
RUN apt-get update && apt-get install -y libc6
WORKDIR /app
COPY ./server /app/server
RUN chmod +x /app/server
EXPOSE 9000
CMD ["./server"]