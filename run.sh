#!/bin/bash -eux

sudo docker compose down
sudo docker image build --network host . -t akawashiro_site
sudo docker compose up -d
