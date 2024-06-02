import argparse
import json
import logging
import tarfile
import tempfile
from pathlib import Path

_logger = logging.getLogger(__name__)
_logger.setLevel(logging.DEBUG)
_logger.addHandler(logging.StreamHandler())


def main():
    parser = argparse.ArgumentParser(
        description="Generate markdown from Mastodon archive"
    )
    parser.add_argument("archive", type=str, help="Path to Mastodon archive")
    parser.add_argument("output", type=str, help="Output markdown file")
    args = parser.parse_args()

    archive_tar_gz = Path(args.archive)
    if not archive_tar_gz.exists():
        _logger.error(f"archive {archive_tar_gz} does not exist")
        return
    if not str(archive_tar_gz).endswith(".tar.gz"):
        _logger.error(f"archive must be a .tar.gz file")
        return

    with tempfile.TemporaryDirectory() as tmpdirname:
        _logger.debug(f"created temporary directory {tmpdirname}")
        with tarfile.open(archive_tar_gz, "r:gz") as tar:
            tar.extractall(tmpdirname)
        for p in Path(tmpdirname).rglob("*.json"):
            _logger.debug(f"found json file {p}")
        outbox_json = Path(tmpdirname) / "outbox.json"
        if not outbox_json.exists():
            _logger.error(f"outbox.json does not exist")
            return
        with open(outbox_json, "r") as f:
            _logger.debug(f"reading {outbox_json}")
            outbox_text = f.read()
        outbox = json.loads(outbox_text)
        if not "orderedItems" in outbox:
            _logger.error(f"orderedItems not found in outbox.json")
            return
        with open(args.output, "w") as f:
            f.write("---\n")
            f.write("title: Mastodon Archive\n")
            f.write("layout: default\n")
            f.write("---\n\n")
        for item in reversed(outbox["orderedItems"]):
            if not "object" in item:
                _logger.error(f"object not found in item")
                continue
            obj = item["object"]
            if not "content" in obj:
                _logger.error(f"content not found in object")
                continue
            content = obj["content"]
            if not "published" in obj:
                _logger.error(f"published not found in object")
                continue
            published = obj["published"]
            with open(args.output, "a") as f:
                f.write(f"## {published}\n")
                f.write(f"{content}\n")


if __name__ == "__main__":
    main()
