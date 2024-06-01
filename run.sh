#!/bin/bash -eux

CONTAINER_NAME=akawashiro_site_container
IMAGE_NAME=akawashiro_site_image

EXITING_CONTAINER_ID=$(sudo docker ps -aqf "name=akawashiro_site_container")
if [ -n "${EXITING_CONTAINER_ID}" ]; then
  sudo docker stop ${EXITING_CONTAINER_ID}
  sudo docker rm ${EXITING_CONTAINER_ID}
fi
sudo docker image build --network host . -t ${IMAGE_NAME}
sudo docker run -d --name ${CONTAINER_NAME} ${IMAGE_NAME}
CONTAINER_IP_ADDR=$(sudo docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' ${CONTAINER_NAME})
echo "Access to http://${CONTAINER_IP_ADDR}"
