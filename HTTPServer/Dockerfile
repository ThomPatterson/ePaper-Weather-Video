FROM --platform=linux/amd64 node:lts-alpine3.19

WORKDIR /usr/app
COPY app/ /usr/app

RUN apk update && \
    apk upgrade && \
    apk add tzdata ffmpeg didder imagemagick && \
    npm install && \
    rm -rf /tmp/npm*

ENV CONFIG={\"DATA_DIR\":\"/usr/data/videos\"}

ENV TZ=America/Chicago

VOLUME /usr/data/videos

EXPOSE 8080

CMD ["npm", "start"]