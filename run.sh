#!/bin/bash -eux

CONTAINER_NAME=akawashiro_site_container
IMAGE_NAME=akawashiro_site_image

EXITING_CONTAINER_ID=$(podman ps -aqf "name=akawashiro_site_container")
if [ -n "${EXITING_CONTAINER_ID}" ]; then
  podman stop ${EXITING_CONTAINER_ID}
  podman rm ${EXITING_CONTAINER_ID}
fi
podman pull docker.io/library/nginx:latest
podman image build --network host . -t ${IMAGE_NAME}
podman run --publish 8080:80/tcp -d --name ${CONTAINER_NAME} ${IMAGE_NAME}
echo "Access to http://localhost:8080/"
