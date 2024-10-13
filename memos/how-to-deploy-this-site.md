---
title: How to deploy this site
layout: default
---

# How to check the result
```sh
$ ./run.sh
```

# How to update contents

1. Update `*.md` files
1. `./run.sh`
1. Add all files under `_site` directory
1. Push

# How to update Mastodon archive
```sh
$ python3 gen_markdown_from_mastodon_archive.py <PATH_TO_THE_ARCHIVE> mastodon_archive.md
```

# TODO

Although this site is deployed both of [https://akawashiro.github.io/](https://akawashiro.github.io/) and [https://akawashiro.com/](https://akawashiro.com/), [https://akawashiro.github.io/](https://akawashiro.github.io/) is using HTML files which Github built. I want to use it to use my build.
