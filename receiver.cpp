/* BASELINE RECEIVER (C) — naive on purpose. Rewrite it (C, C++, Go, or Rust).
 *
 * Ports (all 127.0.0.1):
 *   bind 47002  <- media from your sender, via the hostile relay
 *   send 47020  -> harness player. MUST be: 4-byte big-endian seq +
 *                  160-byte payload. Frame i counts only if it arrives
 *                  BEFORE its deadline t0 + DELAY_MS + i*20ms.
 *   send 47003  -> feedback to your sender, via the relay (optional)
 *
 * This baseline forwards whatever arrives straight to the player: lost
 * frames stay lost, late frames stay late, duplicates are re-sent
 * harmlessly. All yours to fix — jitter buffer, reordering, recovery.
 *
 * Env vars available: T0, DURATION_S, DELAY_MS. Harness kills the process
 * at run end; a forever-loop is fine.
 */
#include <iostream>
#include <vector>
#include <unordered_map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <chrono>
#include <cstdlib>
#include <algorithm>

class PlayoutEngine {
    int in_fd;
    int out_fd;
    struct sockaddr_in player_addr{};
    
    std::unordered_map<uint32_t, std::vector<uint8_t>> frames;
    std::unordered_map<uint32_t, std::vector<uint8_t>> parity_blocks;
    
    uint32_t current_seq = 0;
    bool started = false;
    
    long long t0_ms;
    int delay_ms;

    long long get_current_ms() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    void process_recovery(uint32_t seq) {
        uint32_t base = (seq % 2 == 0) ? seq : seq - 1;
        
        // If we don't have the parity block, we can't recover anything
        if (parity_blocks.find(base) == parity_blocks.end()) return;
        
        bool has_even = frames.count(base);
        bool has_odd = frames.count(base + 1);
        
        // XOR Recovery Magic
        if (has_even && !has_odd) {
            std::vector<uint8_t> recovered(160);
            for(int i = 0; i < 160; i++) {
                recovered[i] = frames[base][i] ^ parity_blocks[base][i];
            }
            frames[base + 1] = std::move(recovered);
        } else if (!has_even && has_odd) {
            std::vector<uint8_t> recovered(160);
            for(int i = 0; i < 160; i++) {
                recovered[i] = frames[base + 1][i] ^ parity_blocks[base][i];
            }
            frames[base] = std::move(recovered);
        }
    }

public:
    PlayoutEngine() {
        in_fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in in_addr{};
        in_addr.sin_family = AF_INET;
        in_addr.sin_port = htons(47002);
        in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        bind(in_fd, (struct sockaddr *)&in_addr, sizeof(in_addr));

        out_fd = socket(AF_INET, SOCK_DGRAM, 0);
        player_addr.sin_family = AF_INET;
        player_addr.sin_port = htons(47020);
        player_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        double t0_env_val = 0.0;
        if (const char* env_t0 = std::getenv("T0")) t0_env_val = std::atof(env_t0);
        
        delay_ms = 50; // Fallback default
        if (const char* env_delay = std::getenv("DELAY_MS")) delay_ms = std::atoi(env_delay);

        t0_ms = static_cast<long long>(t0_env_val * 1000.0);
        if (t0_ms == 0) t0_ms = get_current_ms();
    }

    void run() {
        struct pollfd pfd = {in_fd, POLLIN, 0};
        uint8_t rx_buf[165];

        while (true) {
            int p_res = poll(&pfd, 1, 2); 
            
            if (p_res > 0 && (pfd.revents & POLLIN)) {
                if (recvfrom(in_fd, rx_buf, sizeof(rx_buf), 0, nullptr, nullptr) == 165) {
                    uint32_t net_seq;
                    std::copy(rx_buf + 1, rx_buf + 5, reinterpret_cast<uint8_t*>(&net_seq));
                    uint32_t seq = ntohl(net_seq);
                    
                    if (!started) {
                        current_seq = seq;
                        started = true;
                    }

                    uint8_t pkt_type = rx_buf[0];
                    std::vector<uint8_t> payload(rx_buf + 5, rx_buf + 165);

                    if (pkt_type == 0) {
                        frames[seq] = std::move(payload);
                    } else if (pkt_type == 1) {
                        parity_blocks[seq] = std::move(payload);
                    }
                    
                    process_recovery(seq);
                }
            }

            if (started) {
                // 1. Instantly play out anything ready in the buffer
                while (frames.count(current_seq)) {
                    uint8_t tx_buf[164];
                    uint32_t net_seq = htonl(current_seq);
                    std::copy(reinterpret_cast<uint8_t*>(&net_seq), reinterpret_cast<uint8_t*>(&net_seq) + 4, tx_buf);
                    std::copy(frames[current_seq].begin(), frames[current_seq].end(), tx_buf + 4);
                    
                    sendto(out_fd, tx_buf, 164, 0, (struct sockaddr*)&player_addr, sizeof(player_addr));
                    current_seq++;
                }

                // 2. Strict Deadline Enforcement
                long long now = get_current_ms();
                long long deadline = t0_ms + delay_ms + (current_seq * 20);
                
                while (now >= deadline) {
                    current_seq++; // Give up on this frame
                    
                    // Cascade playout in case the blockage is now cleared
                    while (frames.count(current_seq)) {
                        uint8_t tx_buf[164];
                        uint32_t net_seq = htonl(current_seq);
                        std::copy(reinterpret_cast<uint8_t*>(&net_seq), reinterpret_cast<uint8_t*>(&net_seq) + 4, tx_buf);
                        std::copy(frames[current_seq].begin(), frames[current_seq].end(), tx_buf + 4);
                        
                        sendto(out_fd, tx_buf, 164, 0, (struct sockaddr*)&player_addr, sizeof(player_addr));
                        current_seq++;
                    }
                    deadline = t0_ms + delay_ms + (current_seq * 20);
                }

                // 3. Memory Safety (Garbage Collection)
                if (current_seq > 20) {
                    uint32_t obsolete = current_seq - 20;
                    frames.erase(obsolete);
                    parity_blocks.erase(obsolete);
                }
            }
        }
    }
};

int main() {
    PlayoutEngine engine;
    engine.run();
    return 0;
}