#include <fuse++>

#include "../old/skystream.hpp"

#include <iostream>
#include <fstream>

class libgame_fuse : public fuse {
public:
  sia::portalpool pool;
  std::fstream histfile;
  skystream file;
  uint64_t length;
  double time;
  int mode;

  nlohmann::json process_device(std::string device) {
    nlohmann::json tip;
    histfile.open(device, std::ios::in | std::ios::out | std::ios::ate);
    histfile.clear();
    histfile.exceptions(std::fstream::badbit | std::fstream::failbit);
    if (!histfile.is_open()) {
      histfile.open(device, std::ios::out | std::ios::ate | std::ios::app);
    }
    std::string line;
    try {
      histfile.seekg(-4096, std::ios_base::end);
    } catch (std::exception const &) {
      histfile.clear();
      histfile.seekg(0, std::ios_base::beg);
    }
    while (histfile.peek() != EOF) {
      std::getline(histfile, line);
      try {
        tip = nlohmann::json::parse(line);
      } catch (nlohmann::detail::parse_error const &) {}
    }
    histfile.clear();
    try {
      tip = nlohmann::json::parse(device);
    } catch (nlohmann::detail::parse_error const &) {
      try {
        tip = nlohmann::json::parse("{" + device + "}");
      } catch (nlohmann::detail::parse_error const &) { }
    }
    if (tip.is_null()) {
      tip = {};
    }
    return tip;
  }

  libgame_fuse(std::string device) : file(pool, process_device(device)), length(file.length("bytes")), time(file.span("time").second), mode(0644) {}

  /* reading functions */

  int getattr(const std::string &pathname, struct stat *st) override {
    memset(st, 0, sizeof(*st));
    st->st_uid = uid;
    st->st_gid = gid;
    if (pathname == "/") {
      st->st_mode = S_IFDIR | (0777 ^ umask);
      st->st_size = 0;
      return 0;
    } else if (pathname.substr(0,4) == "/sia") {
      st->st_mode = S_IFREG | (mode ^ umask);
      st->st_size = length;
      double seconds;
      st->st_mtim.tv_nsec = 1000000000 * modf(time, &seconds);
      st->st_mtime = seconds;
      return 0;
    } else {
      return -ENOENT;
    }
  }

  int readdir(const std::string &pathname, off_t off, struct fuse_file_info *fi,
              readdir_flags flags) override {
    struct stat st;
    (void)off;
    (void)fi;
    (void)flags;
    if (pathname != "/") {
      return -ENOENT;
    }
    auto identifiers = file.identifiers();
    std::string name;
    if (identifiers.count("skylink")) {
	    name = identifiers["skylink"];
	    size_t idx1 = name.find("://");
	    size_t idx2 = name.find('/', idx1 + 3);
	    name = "sia:" + name.substr(idx1 + 3, idx2 - idx1 - 3);
    } else {
	    name = "sia:new";
    }
    if (0 == getattr("/" + name, &st)) {
      fill_dir(name, &st);
    }
    return 0;
  }

  int read(const std::string &pathname, char *buf, size_t count, off_t offset,
           struct fuse_file_info *fi) override {
    (void)pathname;
    (void)fi;
    double offset_d = offset;
    try {
      size_t remaining = count;
      while (remaining) {
        std::vector<uint8_t> data = file.read("bytes", offset_d);
        if (data.size() > remaining) {
          data.resize(remaining);
        }
        memcpy(buf, data.data(), data.size());
        buf += data.size();
        remaining -= data.size();
      }
      return count;
    } catch (std::out_of_range &) {
      return 0;
    }
  }

  /* writing functions */

  int chmod(const std::string &pathname, mode_t mode) override {
    if (pathname == "/") {
      return -EACCES;
    }
    this->mode = mode;
    return 0;
  }

  int write(const std::string &pathname, const char *buf, size_t count,
            off_t offset, struct fuse_file_info *fi) override {
    (void)pathname;
    (void)fi;
    file.write({buf, buf + count}, "bytes", offset, {}, {});
    histfile << file.identifiers() << std::endl;
    histfile.flush();
    if (offset + count > (off_t)length) {
      length = offset + count;
    }
    time = file.span("time").second;
    return count;
  }
};

int main(int argc, char *argv[]) {
  std::vector<char*> args{argv, argv + argc};
  std::string device;
  for (size_t w = 1; w < args.size(); w ++) {
    if (args[w][0] == '-') {
      if (std::string(args[w]) == "-o") {
        w ++;
      }
      continue;
    }
    device = args[w];
    args.erase(args.begin() + w);
    break;
  }
  if (device.empty()) {
    std::cerr << "Usage: " << args[0] << " [FUSE options ...] (file.ndjson|'skylink':'sia://address') mountpoint" << std::endl;
    return -1;
  }
  return libgame_fuse(device).main(args.size(), args.data());
}
