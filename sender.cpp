/* BASELINE SENDER (C) — naive on purpose. Rewrite it (C, C++, Go, or Rust).
 *
 * Ports (all 127.0.0.1):
 *   bind 47010  <- harness source delivers frame i here at t0 + i*20ms
 *                  (format: 4-byte big-endian seq + 160-byte payload)
 *   send 47001  -> relay uplink toward the receiver (YOUR wire format)
 *   bind 47004  <- feedback from your receiver, via the relay (optional)
 *
 * This baseline forwards each frame once, unchanged, and ignores feedback.
 * No redundancy, no retransmission. It cannot pass. That is the point.
 *
 * Env vars available if you want them: T0 (epoch seconds, float),
 * DURATION_S, DELAY_MS. The harness kills this process when the run ends,
 * so a forever-loop is fine.
 *
 * build: make        run: python3 run.py --delay_ms 60
 */
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>

class XorFecSender {
    int in_fd;
    int out_fd;
    struct sockaddr_in relay{};
    bool has_prev = false;
    uint32_t prev_seq = 0;
    std::vector<uint8_t> prev_payload;

public:
    XorFecSender() : prev_payload(160, 0) {
        in_fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in in_addr{};
        in_addr.sin_family = AF_INET;
        in_addr.sin_port = htons(47010);
        in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        bind(in_fd, (struct sockaddr*)&in_addr, sizeof(in_addr));

        out_fd = socket(AF_INET, SOCK_DGRAM, 0);
        relay.sin_family = AF_INET;
        relay.sin_port = htons(47001);
        relay.sin_addr.s_addr = inet_addr("127.0.0.1");
    }

    void execute() {
        uint8_t in_buf[164];
        while (true) {
            if (recvfrom(in_fd, in_buf, 164, 0, nullptr, nullptr) != 164) continue;

            uint32_t raw_seq;
            std::copy(in_buf, in_buf + 4, reinterpret_cast<uint8_t*>(&raw_seq));
            uint32_t seq = ntohl(raw_seq);

            uint8_t data_pkt[165];
            data_pkt[0] = 0;
            std::copy(in_buf, in_buf + 164, data_pkt + 1);
            sendto(out_fd, data_pkt, 165, 0, (struct sockaddr*)&relay, sizeof(relay));

            if (has_prev && (seq == prev_seq + 1) && (seq & 1)) {
                uint8_t fec_pkt[165];
                fec_pkt[0] = 1;
                uint32_t net_prev = htonl(prev_seq);
                std::copy(reinterpret_cast<uint8_t*>(&net_prev), reinterpret_cast<uint8_t*>(&net_prev) + 4, fec_pkt + 1);
                
                for (int i = 0; i < 160; ++i) {
                    fec_pkt[i + 5] = prev_payload[i] ^ in_buf[i + 4];
                }
                sendto(out_fd, fec_pkt, 165, 0, (struct sockaddr*)&relay, sizeof(relay));
                has_prev = false;
            } else {
                std::copy(in_buf + 4, in_buf + 164, prev_payload.begin());
                prev_seq = seq;
                has_prev = true;
            }
        }
    }
};

int main() {
    XorFecSender().execute();
    return 0;
}