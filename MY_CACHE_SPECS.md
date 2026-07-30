# M4 Pro Cache / Hardware

Cache line: 128 B (x86 is 64 B)

P-core: L1d 128 KB (2^17), L1i 192 KB, L2 16 MB (2^24, shared per cluster)
E-core: L1d 64 KB (2^16), L1i 128 KB, L2 4 MB (shared per cluster)

Memory: unified LPDDR5X, ~273 GB/s. No L3, SLC sits before DRAM.

## Expected sweep steps (P-core via QoS)

Under 128 KB -> L1, ~1-2 ns
128 KB to 16 MB -> L2, ~5-15 ns
Over 16 MB -> DRAM, ~80-120 ns
SLC may add a shelf past L2.

## Other Notes

- alignas(128) for node padding, false sharing
- QoS class steers P vs E core, moves the L1 step
- L2 shared per cluster, other threads eat capacity

macOS doesn't let me pin a thread to a specific core directly so we need
to declare a Quality of Service class for my thread and the scheduler
uses it to decide core placement

// CODE: pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
The QOS tells the scheduler to run on P-cores (latency critical)
QOS_CLASS_BACKROUND gives me E-Cores
