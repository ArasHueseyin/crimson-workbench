#pragma once
#include "protocol.h"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <string>
#include <unistd.h>

namespace crimson::mac_items {
inline std::uint64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline bool transfer_all(int fd, void* data, std::size_t size, bool sending) {
    const auto deadline = now_ms()+1500;
    auto bytes = static_cast<unsigned char*>(data);
    while (size) {
        const auto now = now_ms();
        if (now >= deadline) return false;
        pollfd wait{fd, static_cast<short>(sending ? POLLOUT : POLLIN), 0};
        const auto ready = poll(&wait, 1, static_cast<int>(deadline-now));
        if (ready < 0 && errno == EINTR) continue;
        if (ready <= 0 || !(wait.revents & wait.events)) return false;
        const auto count = sending ? send(fd, bytes, size, MSG_DONTWAIT) : recv(fd, bytes, size, MSG_DONTWAIT);
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        if (count <= 0) return false;
        bytes += count;
        size -= count;
    }
    return true;
}
inline bool receive_all(int fd, void* data, std::size_t size) { return transfer_all(fd, data, size, false); }

template<class Selection>
void serve_peer(int peer, crimson::live_items::Queue& queue, Selection selection) {
    uid_t uid;
    gid_t gid;
    int no_sigpipe = 1;
    if (getpeereid(peer, &uid, &gid) != 0 || uid != geteuid()
        || setsockopt(peer, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe)) != 0) return;
    crimson::live_items::Request request;
    if (!receive_all(peer, &request, sizeof(request))) return;
    queue.confirm_selection(now_ms(), selection());
    auto reply = queue.handle(request, now_ms());
    transfer_all(peer, &reply, sizeof(reply), true);
}

// Queue and selection callback live for the game process; no engine pointer enters this thread.
template<class Selection>
bool start_transport(crimson::live_items::Queue* queue, Selection selection) {
    char directory[80], path[104];
    std::snprintf(directory, sizeof(directory), "/tmp/crimson-workbench-%u", geteuid());
    if (mkdir(directory, 0700) != 0 && errno != EEXIST) return false;
    struct stat info{};
    if (lstat(directory, &info) != 0 || !S_ISDIR(info.st_mode)
        || info.st_uid != geteuid() || (info.st_mode & 0777) != 0700) return false;
    std::snprintf(path, sizeof(path), "%s/items-%d.sock", directory, getpid());
    if (lstat(path, &info) == 0) {
        if (!S_ISSOCK(info.st_mode) || info.st_uid != geteuid() || unlink(path) != 0) return false;
    } else if (errno != ENOENT) return false;
    const int listener = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listener < 0) return false;
    fcntl(listener, F_SETFD, FD_CLOEXEC);
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, path, std::strlen(path)+1);
    address.sun_len = sizeof(address);
    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0
        || chmod(path, 0600) != 0 || listen(listener, 4) != 0) {
        close(listener);
        unlink(path);
        return false;
    }
    try {
        std::thread([listener, queue, selection, socket_path = std::string(path)] {
            try {
                while (true) {
                    const int peer = accept(listener, nullptr, nullptr);
                    if (peer < 0) { if (errno == EINTR) continue; break; }
                    fcntl(peer, F_SETFD, FD_CLOEXEC);
                    try { serve_peer(peer, *queue, selection); }
                    catch (...) { close(peer); throw; }
                    close(peer);
                }
            } catch (...) { std::fputs("CW_MAC_ITEMS: local connection stopped after an exception.\n", stderr); }
            close(listener);
            unlink(socket_path.c_str());
        }).detach();
    } catch (...) { close(listener); unlink(path); return false; }
    return true;
}
}
