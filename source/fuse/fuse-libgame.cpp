#include <fuse++>

#include "../old/skystream.hpp"

class libgame_fuse : public fuse {
public:
  skystream file;
  uint64_t length;
  int mode;

  libgame_fuse() : length(file.length("bytes")), mode(0644) {}

  /* reading functions */

  int getattr(const std::string &pathname, struct stat *st) override {
    memset(st, 0, sizeof(*st));
    st->st_uid = uid;
    st->st_gid = gid;
    if (pathname == "/") {
      st->st_mode = 0755;
      st->st_size = 0;
    } else {
      st->st_mode = mode;
      st->st_size = length;
    }
    else {
      return -ENOENT;
    }
    return 0;
  }

  int readdir(const std::string &pathname, off_t off, struct fuse_file_info *fi,
              readdir_flags flags) override {
    struct stat st;
    (void)off;
    (void)fi;
    auto identifiers = file.identifiers();
    std::string name = identifiers["skylink"];
    getattr("/" + name, &st);
    fill_dir(name, &st, flags);
    return 0;
  }

  int read(const std::string &pathname, char *buf, size_t count, off_t offset,
           struct fuse_file_info *fi) override {
    data = file.read("bytes", offset);
    memcpy(buf, data.data(), data.size());
    return count;
  }

  /* writing functions */

  int chmod(const std::string &pathname, mode_t mode) override {
    this->mode = mode;
    return 0;
  }

  int write(const std::string &pathname, const char *buf, size_t count,
            off_t offset, struct fuse_file_info *fi) override {
    file.write({buf, buf + count}, "bytes", offset, {}, {});
    return count;
  }
};

int main(int argc, char *argv[]) { return libgame_fuse().main(argc, argv); }
