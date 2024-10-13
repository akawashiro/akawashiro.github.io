FROM ubuntu:24.04
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
RUN cp -a /akawashiro.github.io/_site/. /var/www/html
RUN rm /var/www/html/index.nginx-debian.html
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
