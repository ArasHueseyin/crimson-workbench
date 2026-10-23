#include "../src/transport.h"
#include <cassert>
#include <thread>

int main() {
    using namespace crimson::live_items;
    using namespace crimson::mac_items;
    Queue queue(getpid(), 7);
    queue.observe(now_ms(), 42);
    int pair[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    std::thread server([&] { serve_peer(pair[1], queue, [] { return 42U; }); close(pair[1]); });
    Request request;
    request.operation = Operation::grant;
    request.id[0] = 1;
    request.key = 710001;
    request.quantity = 1;
    request.epoch = 7;
    assert(write(pair[0], &request, 13) == 13);
    assert(write(pair[0], reinterpret_cast<const char*>(&request)+13, sizeof(request)-13) == sizeof(request)-13);
    Reply reply;
    assert(receive_all(pair[0], &reply, sizeof(reply)));
    assert(reply.state == State::queued && reply.pid == static_cast<unsigned>(getpid()) && reply.epoch == 7);
    assert(reply.id == request.id && reply.key == 710001 && reply.quantity == 1);
    close(pair[0]);
    server.join();
    auto taken = queue.take(now_ms(), 42);
    assert(taken && taken->id == request.id);
    queue.finish(*taken, State::applied, Error::none);
    assert(!queue.take(now_ms(), 42));
    assert(queue.handle(request, now_ms()).state == State::applied);
}
