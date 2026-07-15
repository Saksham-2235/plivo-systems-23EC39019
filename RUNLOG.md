# Experiment Log

## Experiment 1: Profile A Tuning
* **Profile:** A
* **Delay:** 60ms
* **Result:** VALID
* **Stats:** Misses: 4 (0.27%), Overhead: 1.55x
* **Changes/Rationale:** Implemented initial Hybrid FEC-80 and ARQ logic. The protocol performed well under low-jitter conditions, meeting the <1% miss rate.

## Experiment 2: Profile A Stability Check
* **Profile:** A
* **Delay:** 80ms
* **Result:** VALID
* **Stats:** Misses: 3 (0.20%), Overhead: 1.55x
* **Changes/Rationale:** Testing increased delay buffer to check for variance. Performance remained stable; no significant changes made to the protocol.

## Experiment 3: Profile B Initial Test
* **Profile:** B
* **Delay:** 60ms
* **Result:** INVALID
* **Stats:** Misses: 399 (26.60%), Overhead: 1.55x
* **Changes/Rationale:** Profile B high-loss environment caused protocol collapse. 60ms buffer is insufficient to retransmit packets lost in burst events.

## Experiment 4: Profile B Buffer Tuning (80ms)
* **Profile:** B
* **Delay:** 80ms
* **Result:** INVALID
* **Stats:** Misses: 43 (2.87%), Overhead: 1.55x
* **Changes/Rationale:** Increased delay to allow more time for NACK retransmissions. Miss rate dropped significantly but remained above the 1% threshold.

## Experiment 5: Profile B Buffer Tuning (90ms)
* **Profile:** B
* **Delay:** 90ms
* **Result:** INVALID
* **Stats:** Misses: 20 (1.33%), Overhead: 1.55x
* **Changes/Rationale:** Marginal improvement, but jitter spikes continue to push packet delivery times past the deadline.

## Experiment 6: Profile B Final Validation
* **Profile:** B
* **Delay:** 100ms
* **Result:** VALID
* **Stats:** Misses: 13 (0.87%), Overhead: 1.55x
* **Changes/Rationale:** 100ms buffer provides enough headroom for retransmission round-trips during burst loss. Protocol stabilized under 1% miss threshold.

## Experiment 7: Profile B Overhead Check
* **Profile:** B
* **Delay:** 150ms
* **Result:** VALID
* **Stats:** Misses: 13 (0.87%), Overhead: 1.55x
* **Changes/Rationale:** Confirmed that increasing delay further does not reduce misses significantly (still 0.87%), validating 100ms as the optimal delay.