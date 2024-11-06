check the number of NUMA nodes, CPU kernels and cache sizes:
```bash
lstopo-no-graphics
```
```
Machine (7845MB total)
  Package L#0
    NUMANode L#0 (P#0 7845MB)
    L3 L#0 (4096KB)
      L2 L#0 (256KB) + L1d L#0 (32KB) + L1i L#0 (32KB) + Core L#0
        PU L#0 (P#0)
        PU L#1 (P#2)
      L2 L#1 (256KB) + L1d L#1 (32KB) + L1i L#1 (32KB) + Core L#1
        PU L#2 (P#1)
        PU L#3 (P#3)
  HostBridge
    PCI 00:02.0 (VGA)
    PCIBridge
      PCI 03:00.0 (Network)
        Net "wlp3s0"
    PCIBridge
      PCI 04:00.0 (SATA)
        Block(Disk) "sda"
```

```bash
gcc numa -o numa.c -pthread -lhwloc -g
```

```bash
gdb ./numa
```

```bash
run 0 0 4 1024 spread
```
segfault because there's only ONE MUNA node on this machine