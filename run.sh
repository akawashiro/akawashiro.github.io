#!/bin/bash -eux

CONTAINER_NAME=akawashiro_site_container
IMAGE_NAME=akawashiro_site_image

EXITING_CONTAINER_ID=$(sudo podman ps -aqf "name=akawashiro_site_container")
if [ -n "${EXITING_CONTAINER_ID}" ]; then
  sudo podman stop ${EXITING_CONTAINER_ID}
  sudo podman rm ${EXITING_CONTAINER_ID}
fi
sudo podman pull docker.io/library/nginx:latest
sudo podman image build --network host . -t ${IMAGE_NAME}
sudo podman run -d --name ${CONTAINER_NAME} ${IMAGE_NAME}
CONTAINER_IP_ADDR=$(sudo podman inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' ${CONTAINER_NAME})
echo "Access to http://${CONTAINER_IP_ADDR}"
