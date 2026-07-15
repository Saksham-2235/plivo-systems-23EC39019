# Architectural Notes

To meet the 60ms latency constraint of Profile A while surviving the 11% packet loss environment of Profile B, I implemented a hybrid transport protocol utilizing **Fractional FEC** and **Gap-Based ARQ**.

### 1. Fractional Forward Error Correction (FEC)
I implemented an XOR-based parity scheme where redundant data (the previous payload) is attached to 80% of packets (omitting parity on every 5th frame). This keeps bandwidth overhead under 2.00x (achieving ~1.85x) while allowing instant recovery for the majority of single-packet losses without incurring round-trip latency.

### 2. Gap-Based ARQ (NACKs)
For scenarios where FEC cannot recover lost packets (e.g., burst loss), the system falls back to a NACK mechanism.
* **Detection:** The receiver sweeps the buffer every 2ms to detect sequence gaps.
* **Triple-Tap NACKs:** To ensure feedback reaches the sender through high-loss channels, NACKs are sent in bursts of 3.
* **Cooldowns:** Both sender and receiver utilize strict 15-20ms cooldowns to prevent congestion collapse and race conditions.