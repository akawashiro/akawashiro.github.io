FROM ubuntu:24.04 as builder

RUN apt update
RUN apt-get install -y \
    git \
    ruby \
    rubygems \
    build-essential \
    ruby-dev \
    nginx
RUN gem install jekyll bundler
COPY . /akawashiro.github.io
WORKDIR /akawashiro.github.io
RUN bundle update
RUN bundle install
RUN bundle exec jekyll build

FROM nginx:latest
COPY --from=builder /akawashiro.github.io/_site /usr/share/nginx/html
COPY nginx.conf /etc/nginx/nginx.conf
