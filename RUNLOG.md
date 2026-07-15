# Plivo Systems Challenge: Experiment Log

## Profile A (Latency Optimized)
* **Command:** `python3 run.py --profile profiles/A.json --delay_ms 60`
* **Result:** VALID
* **Metrics:** * Deadline Misses: < 1.00%
    * Bandwidth Overhead: 1.85x
    * Latency: 60ms

## Profile B (Loss Optimized)
* **Command:** `python3 run.py --profile profiles/B.json --delay_ms 100`
* **Result:** VALID
* **Metrics:** * Deadline Misses: < 1.00%
    * Bandwidth Overhead: 1.22x
    * Latency: 100ms